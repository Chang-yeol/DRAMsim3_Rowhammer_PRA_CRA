#include "rowhammer.h"
#include <queue>
namespace dramsim3{

void print_addr(Address addr) {
    std::cout << "channel: " << addr.channel << " "
              << "rank: " << addr.rank << " "
              << "bankgroup: " << addr.bankgroup << " "
              << "bank: " << addr.bank << " "
              << "row: " << addr.row << std::endl;
}

Rowhammer::Rowhammer(std::string config_file, std::string output_file, std::string trace_file, std::string method)
    : config(config_file, output_file),
      trace(trace_file),
      new_trace(trace_file+"_"+method+"_applied")
    {
        new_trace_file = trace_file + "_"+method+"_applied";
    }

Rowhammer::~Rowhammer() {
    trace.close();
    new_trace.close();
}

std::string Rowhammer::convertedTrace(bool is_cra) {
    std::cout << "generating new trace file ";
    std::string str_addr, trans_type; 
    uint64_t cycle;
    int util_progress = 1;
    bool util_print_flag = true;
    std::vector<string> util_aggressor;
    while (trace >> str_addr >> trans_type >> cycle) {
        if (util_print_flag && util_progress++ % 1000000 == 0) std::cout<<"-"<<std::flush;
        new_trace << str_addr << " " << trans_type << " " << cycle << std::endl;
        
        uint64_t hex_addr;
        std::stringstream tmp;
        tmp << std::hex << str_addr;
        tmp >> hex_addr;
        Address addr = config.AddressMapping(hex_addr);

        updateInfo(addr); // only for CRA
        if (isInsertionRequired()){
            if (util_print_flag) {
                std::cout<<"\n";
                util_print_flag = false;
            }
            if (is_cra && std::find(util_aggressor.begin(), 
                          util_aggressor.end(), 
                          str_addr) 
                        == util_aggressor.end()) {
                util_aggressor.push_back(str_addr);
                std::cout<< "Aggressor detected - "<<str_addr <<" "; print_addr(addr);
                // std::cout<< "Add activation trace for neighbor rows" << std::endl;
            }
            addTrace(addr, cycle);
        }
    }
    if (util_print_flag) std::cout << " done" << std::endl;
    return new_trace_file;
}

void Rowhammer::addTrace(Address addr, uint64_t cycle) {
    // assuming the procedure determining additional
    // activations can be done in one cycle
    int t=0;
    if (addr.row>0) {
        t++;
        Address tmp_addr(addr);
        tmp_addr.row -= 1;
        new_trace << AddressInverseMapping(tmp_addr) << " NEI_ACT " << cycle+t << std::endl;
    }
    if (addr.row<config.rows) {
        t++;
        Address tmp_addr(addr);
        tmp_addr.row += 1;
        new_trace << AddressInverseMapping(tmp_addr) << " NEI_ACT " << cycle+t << std::endl;
    }
}

PRA::PRA(std::string config_file, std::string output_file, std::string trace_file, float probability)
    : Rowhammer(config_file, output_file, trace_file, "PRA"),
      distribution(1,1/probability)
    {
        p = probability;
    }

PRA::~PRA() {}

CRA::CRA(std::string config_file, std::string output_file, std::string trace_file, int threshold)
    : Rowhammer(config_file, output_file, trace_file, "CRA"),
      thd(threshold)
    {
        std::cout<<"Initializing counter table (with 2^" 
                 << LogBase2(config.channels*config.ranks*config.bankgroups*config.banks*config.rows)
                 << " number of entries)" << std::endl;
        counter_table = new int****[config.channels];
        for(int c=0; c<config.channels; ++c) {
            counter_table[c]=new int***[config.ranks];
            for(int r=0; r<config.ranks; ++r) {
                counter_table[c][r]=new int**[config.bankgroups];
                for (int bg=0; bg<config.bankgroups; ++bg) {
                    counter_table[c][r][bg]=new int*[config.banks];
                    for (int b=0;b<config.banks; ++b) {
                        counter_table[c][r][bg][b]=new int[config.rows];
                        for(int row=0; row<config.rows; ++row)
                            counter_table[c][r][bg][b][row]=0;
                    }
                }
            }
        }
        
    }

CRA::~CRA() {
    for(int c=0; c<config.channels; ++c) {
        for(int r=0; r<config.ranks; ++r) {
            for (int bg=0; bg<config.bankgroups; ++bg) {
                for (int b=0;b<config.banks; ++b) {
                    delete counter_table[c][r][bg][b];
                }
                delete counter_table[c][r][bg];
            }
            delete counter_table[c][r];
        }
        delete counter_table[c];
    }
    delete counter_table;
}

void PRA::updateInfo(Address addr) {}
void CRA::updateInfo(Address addr) {
    recent_addr = addr;
    int counter = counterFunc(addr);
    if (counter==thd) {
        // previously, this row was aggressor.
        // We already handled this.
        // Reset the counter.
        counter = 0;
    }
    // Increment the counter
    counter_table[addr.channel][addr.rank][addr.bankgroup][addr.bank][addr.row] = counter+1;
}

bool PRA::isInsertionRequired() {
    // selected with probability p
    return distribution(gen)==1;
}

bool CRA::isInsertionRequired() {
    // if the row is aggressor,
    // it must be the recent_row
    return counterFunc(recent_addr) == thd;
}

int CRA::counterFunc(Address addr){
    return counter_table[addr.channel][addr.rank][addr.bankgroup][addr.bank][addr.row];
}

std::string Rowhammer::AddressInverseMapping(Address addr) {
    uint64_t hex_addr=0;

    std::map<std::string, int> field_widths, field_vals;
    field_widths["ch"] = LogBase2(config.channels);
    field_widths["ra"] = LogBase2(config.ranks);
    field_widths["bg"] = LogBase2(config.bankgroups);
    field_widths["ba"] = LogBase2(config.banks_per_group);
    field_widths["ro"] = LogBase2(config.rows);
    field_widths["co"] = LogBase2(config.columns) - LogBase2(config.BL);

    field_vals["ch"] = addr.channel;
    field_vals["ra"] = addr.rank;
    field_vals["bg"] = addr.bankgroup;
    field_vals["ba"] = addr.bank;
    field_vals["ro"] = addr.row;
    field_vals["co"] = addr.column; // any value (within valid range) is possible

    std::queue<std::string> fields;
    std::string address_mapping = config.address_mapping;
    for (size_t i = 0; i < address_mapping.size(); i += 2) {
        std::string token = address_mapping.substr(i, 2);
        fields.push(token);
    }
    while (!fields.empty()) {
        auto token = fields.front();
        fields.pop();
        // push "width" amount of address 
        // and make room for the value
        // corresponding to the token
        hex_addr <<= field_widths[token];
        // write value corresponding to the token
        hex_addr |= field_vals[token];
    }
    hex_addr <<= config.shift_bits;

    // convert it into string
    std::string str_addr;
    std::stringstream tmp;
    tmp << std::hex << hex_addr;
    tmp >> str_addr;
    return "0x"+str_addr;
}

}

