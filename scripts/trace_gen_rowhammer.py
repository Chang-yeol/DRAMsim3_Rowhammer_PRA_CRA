#!/usr/bin/env python3

import argparse
import configparser
import os
import numpy as np
import copy

def util_format(addr):
    return f'channel: {addr["ch"]}, rank: {addr["ra"]}, bankgroup: {addr["bg"]}, bank: {addr["ba"]}, row: {addr["ro"]}'

def calculate_ranks(config, field_range):
    # code modified fron configuration.cc

    # calculate rank and re-calculate channel_size
    bus_width = int(config['system']['bus_width'])
    device_width = int(config['dram_structure']['device_width'])
    devices_per_rank = bus_width / device_width
    columns = int(config['dram_structure']['columns'])
    page_size = columns * device_width / 8  # page size in bytes
    megs_per_bank = page_size * (field_range["ro"] / 1024) / 1024
    banks = field_range["ba"]
    megs_per_rank = megs_per_bank * banks * devices_per_rank

    channel_size = int(config['system']['channel_size'])
    ranks=0
    if (megs_per_rank > channel_size) :
        ranks = 1
        #channel_size = megs_per_rank
    else :
        ranks = channel_size / megs_per_rank
        #channel_size = ranks * megs_per_rank
    return int(ranks)

def AddressInverseMapping(config, field_range, addr) :
    hex_addr=0

    field_widths=dict()
    field_vals=dict()
    field_widths["ch"] = np.log2(field_range["ch"])
    field_widths["ra"] = np.log2(field_range["ra"])
    field_widths["bg"] = np.log2(field_range["bg"])
    field_widths["ba"] = np.log2(field_range["ba"])
    field_widths["ro"] = np.log2(field_range["ro"])
    field_widths["co"] = np.log2(field_range["co"])

    field_vals["ch"] = addr["ch"]
    field_vals["ra"] = addr["ra"]
    field_vals["bg"] = addr["bg"]
    field_vals["ba"] = addr["ba"]
    field_vals["ro"] = addr["ro"]
    field_vals["co"] = addr["co"]

    fields=list()
    address_mapping = config["system"]["address_mapping"]
    for i in range(0,len(address_mapping),2) :
        token = address_mapping[i:i+2]
        fields.append(token)
    
    for token in fields:
        hex_addr <<= int(field_widths[token])
        hex_addr |= int(field_vals[token])

    bus_width = int(config['system']['bus_width'])
    BL = int(config["dram_structure"]["BL"])

    request_size_bytes = bus_width / 8 * BL
    shift_bits = int(np.log2(request_size_bytes))

    hex_addr <<= shift_bits

    return hex(hex_addr)

def main(args):
    
    config = configparser.ConfigParser()
    config.read(args.config_file)

    inter_arrival=args.interarrival
    if inter_arrival==0:
        inter_arrival = int(config['timing']['tRAS'])+int(config['timing']['tRP'])+2
    print(f'Config: {args.config_file} Output-dir: {args.output_dir} #requests: {args.num_reqs} inter-arrival time: {inter_arrival}')

    field_range = dict()
    field_range["ch"]=int(config['system']['channels'])
    #field_range["ra"] is defined later
    field_range["bg"]=int(config['dram_structure']['bankgroups'])
    field_range["ba"]=field_range["bg"]*int(config['dram_structure']['banks_per_group'])
    field_range["ro"]=int(config['dram_structure']['rows'])
    BL = int(config["dram_structure"]["BL"])
    field_range["co"]=int(int(config['dram_structure']['columns']) / BL)
    
    field_range["ra"] = calculate_ranks(config, field_range)

    # generate victim row
    victim = dict()
    for field in field_range:
        # select random number from [0,field_range[field])
        victim[field]=np.random.randint(field_range[field])

    # generate aggressor row
    aggressor = copy.deepcopy(victim)
    aggressor["ro"]=victim["ro"]+1
    if victim["ro"]+1 == field_range["ro"]:
        aggressor["ro"]=victim["ro"]-1
    
    # generate row for avoid row hits
    avoid_row_hit = copy.deepcopy(victim)
    while (True):
        avoid_row_hit["ro"]=np.random.randint(field_range["ro"])
        if avoid_row_hit["ro"]!=victim["ro"] and avoid_row_hit["ro"]!=aggressor["ro"]:
            break
    
    victim_addr = AddressInverseMapping(config, field_range, victim)
    aggressor_addr = AddressInverseMapping(config, field_range, aggressor)
    avoid_row_hit_addr = AddressInverseMapping(config, field_range, avoid_row_hit)
    print("Victim: \t", util_format(victim), "\thex addr: ", victim_addr) 
    print("Aggressor: \t", util_format(aggressor), "\thex addr: ", aggressor_addr)
    print("Avoid_row_hit: \t", util_format(avoid_row_hit), "\thex addr: ",avoid_row_hit_addr)
    
    config_name = args.config_file.split('.')[0].split('/')[-1]
    file_name = f'trace_{config_name}'
    print("Write to file: \t", file_name, "(note that there is no extension)")
    file = open(os.path.join(args.output_dir,file_name),'w')

    coin_toss = True
    clk = 0
    op="READ"
    for i in range(args.num_reqs):
        if coin_toss:
            file.write(f'{aggressor_addr} {op} {clk}\n')
            coin_toss = False
        else :
            file.write(f'{avoid_row_hit_addr} {op} {clk}\n')
            coin_toss=True
        clk += inter_arrival

                
    file.close()

    #print(victim)
    
    


def check_dir_exists(dir, option):
    if not os.path.exists(dir):
        print(f'Directory for the {option} "{dir}" does not exists.')
        exit(1)
    return

def check_value_validity(val, option):
    if val <= 0:
        print(f'The value for the {option} "{val}" is invalid.')
        exit(1)
    # if value is too large,
    # there might be some compatibility issues 
    # with c++ code
    return

if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description="Rowhammered Trace Generator with for dramsim3",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter
    )
    
    parser.add_argument('config_file',
                        type=str, 
                        help="input path to the config_file for generating trace. \
                              e.g., PATH_TO_CONFIG/DDR3_8Gb_x16_1866.ini"
                       )
    # parser.add_argument('--rowhammer',
    #                     help="Generate victim row if set.",
    #                     action=argparse.BooleanOptionalAction,
    #                     required=True
    #                     )
    parser.add_argument('-n','--num-reqs',
                        type=int,
                        help="Total number of requests. \
                              The default value is 10^7.",
                        default=10**7)
    parser.add_argument('-i', '--interarrival',
                        type=int,
                        help='Inter-arrival time in cycles. \
                              The default value is "row cycle time"+ 2 = tRC + 2 (=tRAS+tRP+2) in order to avoid row buffer hits. \
                              Ignore default message -->',
                        default=0)
    # parser.add_argument('-r', '--ratio',
    #                     type=float,
    #                     help='Read to write(1) ratio. \
    #                           The default value is 2.0.',
    #                     default=2.0)
    parser.add_argument("-o", "--output-dir",
                        type=str,
                        help="output directory. \
                              The default dir is '.'", default=".")

    args = parser.parse_args()

    check_dir_exists(args.config_file, "config file")
    check_dir_exists(args.output_dir, "output folder")

    check_value_validity(args.num_reqs, "number of requests")
    check_value_validity(args.interarrival, "inter-arrival time")
    # check_value_validity(args.ratio, "read-write ratio")
    
    main(args)