![UPMEM Logo](https://www.upmem.com/wp-content/uploads/2018/05/Logo-original-wesbite.png)

The UPMEM Processing-In-Memory solution consists of a pool of programmable co-processors, the DPUs, integrated in the memory.

# Content
Examples of DPU programs generated with UPMEM DPU SDK

# Requirements

 * The [UPMEM DPU SDK](https://sdk.upmem.com/)
 * GNU make and GCC, to build the application

# Building the programs

`make` to build the programs.

`make clean` removes the resulting builds.

# Executing

The simplest way to execute the programs is to run *make test*.
Otherwise, you may run individual tests manually by executing the binary generated in the *build* folder of the targeted program.

# List of demo programs

## checksum

The host application generates a random file of 8MB, and schedules multiple DPUs co-processors to compute the checksum on this file.
As sanity check it compares host result with the DPU results, and prints out the DPU performances:
* Total number of DPU cycles executed
* The number of DPU cycles per byte processed

Macros can be used to change the dimensions of the program:
* NR_TASKLETS set the number of tasklets used by the dpu program (default: 16)
* NR_DPUS set the number of dpus used by the host program (default: 1)

example: `make test NR_TASKLETS=1 NR_DPUS=DPU_ALLOCATE_ALL`

Two implementations of the Host application are provided:
* `host.c` using the C Host API
* `host.py` using the Python Host API

`make test` will run both implementations. Use the Makefile targets `test_c` or `test_python` to specify an implementation.

Note: because the input file is randomly generated, the checksum may differ between the two implementations.

# SDK Documentation
https://sdk.upmem.com/stable/
