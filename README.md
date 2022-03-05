# MSMT

## Configuration
Create a **general configuration file** (e.g. *In/msmt.cfg*) with the parameters:
* RUN_NAME: name of run used to identify the output folder
* NUM_THREAS: a number representing the total amount of threads the MSMT supports
* WINDOW_SIZE: a number representing the instruction window size of each thread
* ORDERING: IN ORDER for in-order execution or OOO for out-of-order execution
* THREAD_CFG: path to threads configuration file
* SCHEDULING: RR for Round Robin, ICNT for Instruction Count, CCNT for Cycle Count
* TYPE_EXE_UNITS: HOM for homogeneous or HET for heterogeneous
* EXE_UNITS_CFG: path to execution units configuration file
* CREATE_TRACES: TRUE or FALSE

Create a **threads configuration file** (e.g. *In/Threads/threads.cfg*):
* Each line is in the format **\<Number of Threads\> \<Path to Trace File\>**
* Can have multiple lines one for each trace file
* The total amount of number of threads must match that specified in the general configuration file

Create an **execution units configuration file** (e.g. *In/ExecutionUnits/hetExeUnits.cfg* or *In/ExecutionUnits/homExeUnits.cfg*):
* Each line is in the foram **\<Number of Units\> \<Unit Type\> \<Latency\>**
* If TYPE_EXE_UNITS is HOM - one line with Unit Type ALL
* if TYPE_EXE_UNITS is HET - five lines for ALU, FP, MEM, JUMP_BRANCH, MISC

## Build & Run
To build the MSMT Simulator simply run:
```bash
make
```
This will create the execution file **msmtSim**

To run the simulator simply pass the general configuration file, for example:
```bash
./msmtSim In/msmt.cfg
```

## Outputs
All outputs will be generated into the folder **Out/\<RUN_NAME\>**
1. general_stats.csv - Includes statistics regrading general IPC and utilization along with utilization per execution unit type.
2. thread_stats.csv - Includes per thread statistics including single thread IPC.
3. Traces/execution_units.log - Details cycles where execution units were fully utilized.
4. Traces/scheduler.log - Cycle accurate log detailing information on which instructions from which threads were arbitrated by the scheduler and sent to execute.
5. Traces/Threads/thread_#.trc - Trace file per thread detailing the instructions parsed with information like type (e.g. ALU/MEM), registers read from and written to, and dependencies on previous instructions within the instruction window.
6. Traces/Threads/thread_window_#.log - Log file per thread detailing the instruction window state per cycle with information like the instructions in the window, their type (e.g. ALU/MEM), their state (e.g. Fetched/Running).

Outputs 3-7 are created if CREATE_TRACES is set to TRUE, while outputs 1-2 are always created.
