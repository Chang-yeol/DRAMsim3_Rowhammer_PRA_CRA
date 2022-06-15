#include "rowhammer.h"

namespace dramsim3{

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

std::string Rowhammer::convertedTrace() {
    std::cout << "generating new trace file...";
    std::string str_addr, trans_type; 
    uint64_t cycle;
    while (trace >> str_addr >> trans_type >> cycle) {
        new_trace << str_addr << " " << trans_type << " " << cycle << std::endl;
        
        uint64_t hex_addr;
        std::stringstream tmp;
        tmp << std::hex << str_addr;
        tmp >> hex_addr;
        Address addr = config.AddressMapping(hex_addr);

        updateInfo(addr.row); // only for CRA
        if (isInsertionRequired()){
            addTrace(addr.row, cycle);
            // std::cout << "new trace added.. neighbor of row: " << addr.row << std::endl;
        }
    }
    std::cout << "DONE" << std::endl;
    return new_trace_file;
}

void Rowhammer::addTrace(int row, uint64_t cycle) {
    // assuming the procedure determining additional
    // activations can be done in one cycle
    int t=0;
    if (row>0) {
        t++;
        new_trace << "0x" << getAddrOf(row-1) << " NEI_ACT " << cycle+t << std::endl;
    }
    if (row<config.rows) {
        t++;
        new_trace << "0x" << getAddrOf(row+1) << " NEI_ACT " << cycle+t << std::endl;
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
        counter_table = new int[config.rows];
        for(int r=0; r<config.rows; ++r) counter_table[r]=0;
    }

CRA::~CRA() {
    delete counter_table;
}

void PRA::updateInfo(int row) {}
void CRA::updateInfo(int row) {
    recent_row = row;
    int counter = counter_table[row];
    if (counter==thd) {
        // previously, this row was aggressor.
        // We already handled this.
        // Reset the counter and increment it.
        counter = 1;
    }
    counter_table[row] = counter+1;
}

bool PRA::isInsertionRequired() {
    // selected with probability p
    return distribution(gen)==1;
}

bool CRA::isInsertionRequired() {
    // if the row is aggressor,
    // it must be the recent_row
    return counter_table[recent_row] == thd;
}

std::string Rowhammer::getAddrOf(int row) {
    std::string str_addr;
    std::stringstream tmp;
    tmp << std::hex << ((uint64_t) row << (config.shift_bits + config.ro_pos));
    tmp >> str_addr;
    return str_addr;
}

}

