# dpu_demo
Examples of DPU programs using the UPMEM DPU SDK

## Requirements

 * The UPMEM DPU SDK, version 0.7.5-EAP or later
 * Gnu make and gcc, to build the application

##Building the programs

You can build the programs for a given type of target, invoking *make [TARGET_TYPE=...]*.

 * When not set, *TARGET_TYPE* is the functional simulator
 * Otherwise, you may select a specific type, such as *FPGA*

*make clean* removes the resulting builds.

## Executing

The simplest way to execute the programs is to run *make test*.
Otherwise, you may run individual tests manually:

  * *cd TEST_DIR*
  * *bin/host/program*

Where *TEST_DIR* is the test name (e.g. *checksum*).

## List of demo programs

### checksum

This demo schedules the computation of a simple checksum on one DPU.
