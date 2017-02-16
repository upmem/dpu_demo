![UPMEM Logo](http://www.upmem.com/wp-content/uploads/2015/06/logo_upmem2.png)

UPMEM Processing-In-Memory solution consist of a pool of programmable co-processors, the DPUs, integrated in the memory.

# dpu_demo package
Examples of DPU programs generated with UPMEM DPU SDK

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

The host application generates a random file of 8MB and schedules one DPU to compute the checksum.
It compares the provided result with the actual checksum value and prints out the DPU performances in number of instructions per processed byte.
 
