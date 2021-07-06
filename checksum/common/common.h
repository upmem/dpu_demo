#ifndef __COMMON_H__
#define __COMMON_H__

#define XSTR(x) STR(x)
#define STR(x) #x

/* DPU variable that will be read of write by the host */
#define DPU_BUFFER dpu_mram_buffer
#define DPU_CACHES dpu_wram_caches
#define DPU_RESULTS dpu_wram_results

/* Size of the buffer on which the checksum will be performed */
#define BUFFER_SIZE (8 << 20)

/* Structure used by both the host and the dpu to communicate information */

#include <stdint.h>

//#define VERBOSE   // Print info on stdout
//#define PRINT_SEQ // Print info in sequential mode instead of flush stdout

#define NB_CKSUM 1  // Run NB_CKSUM checksum(s) from one MRAM database

#define RELOAD_MRAM // Reload database from MRAM to WRAM for each DPU checksum, if NB_CKSUM > 1

//#define READ_RESULT_FROM_MRAM //Read DPU checksum results from MRAM, or else by CI

typedef struct {
    uint32_t checksum[NB_CKSUM];
    uint32_t cycles;
} dpu_result_t;

typedef struct {
    dpu_result_t tasklet_result[NR_TASKLETS];
} dpu_results_t;

#endif /* __COMMON_H__ */
