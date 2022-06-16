[![Build Status](https://travis-ci.com/umd-memsys/DRAMsim3.svg?branch=master)](https://travis-ci.com/umd-memsys/DRAMsim3)

# About DRAMsim3_Rowhammer_PRA_CRA

On top of [DRAMsim3](https://github.com/umd-memsys/DRAMsim3) model, we implemented the PRA (probablistic row activation) and CRA (counter-based row activation) scheme for the protection from row hammering based on the following.

*Dae-Hyun Kim, Prashant J. Nair, and Moinuddin K. Qureshi. 2015. Architectural Support for Mitigating Row Hammering in DRAM Memories. IEEE Comput. Archit. Lett. 14, 1 (Jan.-June 2015), 9–12.* [Link](https://doi.org/10.1109/LCA.2014.2332177)

It is worth noting that it is **_not_** the actual realization of PRA and CRA. For PRA, we need additional coin tossing module with (pre)defined probability. For CRA, wee need additional dram space for row-activation-counter and caching mechanism for accelerating the counting proceduer. These systems are not fully implemented in the configuration of a memory system. <br/>
Therefore, in terms of energy consumption, the result of the simulator does not reflect the required energy for these systems. However, in terms of timing, by assuming the timing (related to these systems) can be hidden behind the trace handling procedure (e.g., by assuming coin tossing can be done in 1 cycle and assuming counting procedure can be done parallely), I concluded that it is not critical. Please consider this when interpreting the results.

For more information on DRAMsim3 itself, please refer to the section [About DRAMsim3](#about-dramsim3) below.

## Generating row hammering trace

```scripts/trace_gen_rowhammer.py``` produces a trace file consists of only ```READ``` operation but has row hammering attack. According to the input configuration file (inside the folder ```configs```), it first decides a victim row, and declare one aggressor row and one row for avoiding row hits. (Note that the latter row is also an aggressor in dram's view.)

You can run the code by the following (requires `numpy`):

```bash
# help
python scripts/trace_gen_rowhammer.py -h

# running example
python scripts/trace_gen_rowhammer.py configs/DDR3_8Gb_x16_1866.ini
```

The name of the output file is ```trace_[CONFIG]``` (without extension), e.g., ```trace_DDR3_8Gb_x16_1866```. <br/>
The output can be directed to another directory by `-o` option. The default is set to the current directory. (Please check the other option using `-h` flag.)

## Building & running the simulator

This section is essensially equivalent to the corresponding original explanation, but for the sake of completeness (whatever it means), it is restated.

### Building the simulator

CMake 3.0+ is required to build the simulator.
It is recommended to make another directory for the build files.

```bash
# cmake out of source build
mkdir build
cd build
# assume using MinGW compiler
cmake .. -G "MinGW Makefiles"

# Build dramsim3 library and executables
make -j4
```

The build process creates `dramsim3main` and executables in the (current) `build` directory.

### Running the simulater

When rowhammer protection scheme is applied (by setting `-r` flag), ```src/rowhammer.cc``` generates a new trace file (on the same directory with the input trace file) with protection applied, e.g., ```trace_DDR3_8Gb_x16_1866_CRA_applied``` (without extension). The new trace file has additional traces with operation code ```NEI_ACT```, which stands for Neighbor Activation.

This new operation is treated same as ```refresh``` in terms of energy consumption. After the new trace is generated, the simulator simulates it for the input cycles. While simulating, it calculates how many number of ```NEI_ACT``` operation is handled and how much enery did it consume. The result of the simulation can be checked in the files in the output directory (which can be defined by setting `-o` flag).

The description assumes that we are still in the `build` directory. <br/>
It is recommended to create `output` folder inside the `build` directory before running the simulator.

```bash
# help
dramsim3main -h

# Running a trace file with PRA scheme
dramsim3main ../configs/DDR3_8Gb_x16_1866.ini -c 100000000 -t ../trace_DDR3_8Gb_x16_1866 -o output -r PRA
# PRA with different probability (default 0.001)
dramsim3main ../configs/DDR3_8Gb_x16_1866.ini -c 100000000 -t ../trace_DDR3_8Gb_x16_1866 -o output -r PRA -p 0.0005

# Running a trace file with CRA scheme
dramsim3main ../configs/DDR3_8Gb_x16_1866.ini -c 100000000 -t ../trace_DDR3_8Gb_x16_1866 -o output -r CRA
# CRA with different counter threshold (default 55555)
dramsim3main ../configs/DDR3_8Gb_x16_1866.ini -c 100000000 -t ../trace_DDR3_8Gb_x16_1866 -o output -r CRA --thd 56789

```

# About DRAMsim3

DRAMsim3 models the timing paramaters and memory controller behavior for several DRAM protocols such as DDR3, DDR4, LPDDR3, LPDDR4, GDDR5, GDDR6, HBM, HMC, STT-MRAM. It is implemented in C++ as an objected oriented model that includes a parameterized DRAM bank model, DRAM controllers, command queues and system-level interfaces to interact with a CPU simulator (GEM5, ZSim) or trace workloads. It is designed to be accurate, portable and parallel.
    
If you use this simulator in your work, please consider cite:

[1] S. Li, Z. Yang, D. Reddy, A. Srivastava and B. Jacob, "DRAMsim3: a Cycle-accurate, Thermal-Capable DRAM Simulator," in IEEE Computer Architecture Letters. [Link](https://ieeexplore.ieee.org/document/8999595)

See [Related Work](#related-work) for more work done with this simulator.


## Building and running the simulator

This simulator by default uses a CMake based build system.
The advantage in using a CMake based build system is portability and dependency management.
We require CMake 3.0+ to build this simulator.
If `cmake-3.0` is not available,
we also supply a Makefile to build the most basic version of the simulator.

### Building

Doing out of source builds with CMake is recommended to avoid the build files cluttering the main directory.

```bash
# cmake out of source build
mkdir build
cd build
cmake ..

# Build dramsim3 library and executables
make -j4

# Alternatively, build with thermal module enabled
cmake .. -DTHERMAL=1

```

The build process creates `dramsim3main` and executables in the `build` directory.
By default, it also creates `libdramsim3.so` shared library in the project root directory.

### Running

```bash
# help
./build/dramsim3main -h

# Running random stream with a config file
./build/dramsim3main configs/DDR4_8Gb_x8_3200.ini --stream random -c 100000 

# Running a trace file
./build/dramsim3main configs/DDR4_8Gb_x8_3200.ini -c 100000 -t sample_trace.txt

# Running with gem5
--mem-type=dramsim3 --dramsim3-ini=configs/DDR4_4Gb_x4_2133.ini

```

The output can be directed to another directory by `-o` option
or can be configured in the config file.
You can control the verbosity in the config file as well.

### Output Visualization

`scripts/plot_stats.py` can visualize some of the output (requires `matplotlib`):

```bash
# generate histograms from overall output
python3 scripts/plot_stats dramsim3.json

# or
# generate time series for a variety stats from epoch outputs
python3 scripts/plot_stats dramsim3epoch.json
```

Currently stats from all channels are squashed together for cleaner plotting.

### Integration with other simulators

**Gem5** integration: works with a forked Gem5 version, see https://github.com/umd-memsys/gem5 at `dramsim3` branch for reference.

**SST** integration: see http://git.ece.umd.edu/shangli/sst-elements/tree/dramsim3 for reference. We will try to merge to official SST repo.

**ZSim** integration: see http://git.ece.umd.edu/shangli/zsim/tree/master for reference.

## Simulator Design

### Code Structure

```
├── configs                 # Configs of various protocols that describe timing constraints and power consumption.
├── ext                     # 
├── scripts                 # Tools and utilities
├── src                     # DRAMsim3 source files
├── tests                   # Tests of each model, includes a short example trace
├── CMakeLists.txt
├── Makefile
├── LICENSE
└── README.md

├── src  
    bankstate.cc: Records and manages DRAM bank timings and states which is modeled as a state machine.
    channelstate.cc: Records and manages channel timings and states.
    command_queue.cc: Maintains per-bank or per-rank FIFO queueing structures, determine which commands in the queues can be issued in this cycle.
    configuration.cc: Initiates, manages system and DRAM parameters, including protocol, DRAM timings, address mapping policy and power parameters.
    controller.cc: Maintains the per-channel controller, which manages a queue of pending memory transactions and issues corresponding DRAM commands, 
                   follows FR-FCFS policy.
    cpu.cc: Implements 3 types of simple CPU: 
            1. Random, can handle random CPU requests at full speed, the entire parallelism of DRAM protocol can be exploited without limits from address mapping and scheduling pocilies. 
            2. Stream, provides a streaming prototype that is able to provide enough buffer hits.
            3. Trace-based, consumes traces of workloads, feed the fetched transactions into the memory system.
    dram_system.cc:  Initiates JEDEC or ideal DRAM system, registers the supplied callback function to let the front end driver know that the request is finished. 
    hmc.cc: Implements HMC system and interface, HMC requests are translates to DRAM requests here and a crossbar interconnect between the high-speed links and the memory controllers is modeled.
    main.cc: Handles the main program loop that reads in simulation arguments, DRAM configurations and tick cycle forward.
    memory_system.cc: A wrapper of dram_system and hmc.
    refresh.cc: Raises refresh request based on per-rank refresh or per-bank refresh.
    timing.cc: Initiate timing constraints.
```

## Experiments

### Verilog Validation

First we generate a DRAM command trace.
There is a `CMD_TRACE` macro and by default it's disabled.
Use `cmake .. -DCMD_TRACE=1` to enable the command trace output build and then
whenever a simulation is performed the command trace file will be generated.

Next, `scripts/validation.py` helps generate a Verilog workbench for Micron's Verilog model
from the command trace file.
Currently DDR3, DDR4, and LPDDR configs are supported by this script.

Run

```bash
./script/validataion.py DDR4.ini cmd.trace
```

To generage Verilog workbench.
Our workbench format is compatible with ModelSim Verilog simulator,
other Verilog simulators may require a slightly different format.


## Related Work

[1] Li, S., Yang, Z., Reddy D., Srivastava, A. and Jacob, B., (2020) DRAMsim3: a Cycle-accurate, Thermal-Capable DRAM Simulator, IEEE Computer Architecture Letters.

[2] Jagasivamani, M., Walden, C., Singh, D., Kang, L., Li, S., Asnaashari, M., ... & Yeung, D. (2019). Analyzing the Monolithic Integration of a ReRAM-Based Main Memory Into a CPU's Die. IEEE Micro, 39(6), 64-72.

[3] Li, S., Reddy, D., & Jacob, B. (2018, October). A performance & power comparison of modern high-speed DRAM architectures. In Proceedings of the International Symposium on Memory Systems (pp. 341-353).

[4] Li, S., Verdejo, R. S., Radojković, P., & Jacob, B. (2019, September). Rethinking cycle accurate DRAM simulation. In Proceedings of the International Symposium on Memory Systems (pp. 184-191).

[5] Li, S., & Jacob, B. (2019, September). Statistical DRAM modeling. In Proceedings of the International Symposium on Memory Systems (pp. 521-530).

[6] Li, S. (2019). Scalable and Accurate Memory System Simulation (Doctoral dissertation).

