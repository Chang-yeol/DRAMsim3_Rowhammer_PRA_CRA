#ifndef __ROWHAMMER_H
#define __ROWHAMMER_H

#include <string>
#include <sstream>
#include <random>

#include "configuration.h"

namespace dramsim3 {

class Rowhammer {
    public:
        Rowhammer(std::string config_file, std::string output_file, std::string trace_file, std::string method);
        ~Rowhammer();

        std::string new_trace_file;
        
        std::string convertedTrace();
        virtual bool isInsertionRequired(){return false;}
        virtual void updateInfo(int row){}
        void addTrace(int row, uint64_t cycle);
        std::string getAddrOf(int row);
    protected:
        Config config;
        std::ifstream trace;
        std::ofstream new_trace;
};

class PRA : public Rowhammer {
    public:
        PRA(std::string config_file, std::string output_file, std::string trace_file, float probability);
        ~PRA();
        bool isInsertionRequired() override;
        void updateInfo(int row) override;
    private:
        std::random_device rd;
        std::mt19937_64 gen;
        std::uniform_int_distribution<int> distribution;
        float p;
};

class CRA : public Rowhammer {
    public:
        CRA(std::string config_file, std::string output_file, std::string trace_file, int threshold);
        ~CRA();
        bool isInsertionRequired() override;
        void updateInfo(int row) override;
    private:
        const int thd;
        int * counter_table;
        int recent_row;
};

}

#endif