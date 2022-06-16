#include <iostream>
#include "./../ext/headers/args.hxx"
#include "cpu.h"
#include "rowhammer.h"

using namespace dramsim3;

int main(int argc, const char **argv) {
    args::ArgumentParser parser(
        "DRAM Simulator.",
        "Examples: \n"
        "dramsim3main ../configs/DDR3_8Gb_x16_1866.ini -c 100000000 -t ../sample_trace -r CRA\n"
        );
    args::HelpFlag help(parser, "help", "Display the help menu", {'h', "help"});
    args::ValueFlag<uint64_t> num_cycles_arg(parser, "num_cycles",
                                             "Number of cycles to simulate",
                                             {'c', "cycles"}, 100000000);
    args::ValueFlag<std::string> output_dir_arg(
        parser, "output_dir", "Output directory for stats files",
        {'o', "output-dir"}, ".");
    args::ValueFlag<std::string> stream_arg(
        parser, "stream_type", "address stream generator - (random), stream",
        {'s', "stream"}, "");
    args::ValueFlag<std::string> trace_file_arg(
        parser, "trace",
        "Trace file, setting this option will ignore -s option",
        {'t', "trace"});
    args::ValueFlag<std::string> rowhammer_arg(
        parser, "rowhammer",
        "Rowhammer protection, option: X (not applied, default), PRA (Probablistic Row Activation), CRA (Counter-based Row Activation)",
        {'r', "rowhammer"}, "X");
    args::Positional<std::string> config_arg(
        parser, "config", "The config file name (mandatory)");

    try {
        parser.ParseCLI(argc, argv);
    } catch (args::Help) {
        std::cout << parser;
        return 0;
    } catch (args::ParseError e) {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        return 1;
    }

    std::string config_file = args::get(config_arg);
    if (config_file.empty()) {
        std::cerr << parser;
        return 1;
    }

    uint64_t cycles = args::get(num_cycles_arg);
    std::string output_dir = args::get(output_dir_arg);
    std::string trace_file = args::get(trace_file_arg);
    std::string stream_type = args::get(stream_arg);
    std::string rowhammer_type = args::get(rowhammer_arg);

    CPU *cpu;
    if (!trace_file.empty()) {
        bool is_cra = true;
        if (rowhammer_type.compare("PRA") == 0) {
            // probability = 0.001
            PRA pra(config_file,output_dir,trace_file, 0.001);
            is_cra = false;
            trace_file = pra.convertedTrace(is_cra);
        }
        else if (rowhammer_type.compare("CRA") == 0) {
            // threshold = 55555
            // bit flip occur when consecutive 55555 attacks
            CRA cra(config_file,output_dir,trace_file,55555);
            trace_file = cra.convertedTrace(is_cra);
        }
        else {
            if (rowhammer_type.compare("X") != 0){
                std::cout << "Undefined Row Hammering Scheme" << std::endl;
                return 0;
            }
        }
        cpu = new TraceBasedCPU(config_file, output_dir, trace_file);
    } else {
        if (stream_type == "stream" || stream_type == "s") {
            cpu = new StreamCPU(config_file, output_dir);
        } else {
            cpu = new RandomCPU(config_file, output_dir);
        }
    }
    std::cout << "simulating trace file ";
    for (uint64_t clk = 0; clk < cycles; clk++) {
        if (clk % 10000000==0) std::cout<< "-"<<std::flush;
        cpu->ClockTick();

    }
    std::cout << " done" << std::endl;
    cpu->PrintStats();

    std::cout<<"check the output files in directory ./" << output_dir << std::endl;
    delete cpu;

    return 0;
}
