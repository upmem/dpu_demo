#ifndef __COMMON_H__
#define __COMMON_H__

#define XSTR(x) STR(x)
#define STR(x) #x

/* DPU variable that will be read of write by the host */
#define DPU_BUFFER dpu_mram_buffer
#define DPU_CACHES dpu_wram_caches
#define DPU_RESULTS dpu_wram_results

/* Size of the buffer on which the checksum will be performed */
#define BUFFER_SIZE (64 << 20)

/* Number of running tasklets on the DPU */
#define NB_TASKLETS_PER_DPU 16

/* Structure used by both the host and the dpu to communicate information */
typedef struct {
    uint32_t checksum;
    uint32_t cycles;
} dpu_results_t;

#endif /* __COMMON_H__ */
