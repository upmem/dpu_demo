![UPMEM Logo](https://www.upmem.com/wp-content/uploads/2018/05/Logo-original-wesbite.png)

The UPMEM Processing-In-Memory solution consists of a pool of programmable co-processors, the DPUs, integrated in the memory.

# Content
Examples of DPU programs generated with UPMEM DPU SDK

# Requirements

 * The [UPMEM DPU SDK](https://sdk.upmem.com/)
 * Gnu make and gcc, to build the application

#Building the programs

*make* to build the programs.

*make clean* removes the resulting builds.

# Executing

The simplest way to execute the programs is to run *make test*.
Otherwise, you may run individual tests manually by executing the binary generated in the *build* folder of the targeted program.

# List of demo programs

## checksum

The host application generates a random file of 8MB, and schedules multiple DPUs co-processors to compute the checksum on this file.
As sanity check it compares host result with the DPU results, and prints out the DPU performances:
* 1) Total number of DPU cycles executed
* 2) The number of DPU cycles per byte processed

Macros can be used to change the of the program:
* 1) NR_TASKLETS set the number of tasklets used by the dpu program (default: 16)
* 2) NR_DPUS set the number of dpu used by the host program (default: 1)

example: *NR_TASKLETS=1 NR_DPUS=DPU_ALLOCATE_ALL make test*

# SDK Documentation
https://sdk.upmem.com/stable/
