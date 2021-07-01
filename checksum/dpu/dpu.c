/*
 * Copyright (c) 2014-2017 - uPmem
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * An example of checksum computation with multiple tasklets.
 *
 * Every tasklet processes specific areas of the MRAM, following the "rake"
 * strategy:
 *  - Tasklet number T is first processing block number TxN, where N is a
 *    constant block size
 *  - It then handles block number (TxN) + (NxM) where M is the number of
 *    scheduled tasklets
 *  - And so on...
 *
 * The host is in charge of computing the final checksum by adding all the
 * individual results.
 */
#include <defs.h>
#include <mram.h>
#include <perfcounter.h>
#include <stdint.h>
#include <stdio.h>

#include "common.h"

#define ROTLEFT(a,b) (((a) << (b)) | ((a) >> (32-(b))))
#define ROTRIGHT(a,b) (((a) >> (b)) | ((a) << (32-(b))))

/* Use blocks of 256 bytes */
#define BLOCK_SIZE (256)

#define PRINT_BUFFER_SIZE (11*NB_CKSUM + 25)

__dma_aligned uint8_t DPU_CACHES[NR_TASKLETS][BLOCK_SIZE];
__host dpu_results_t DPU_RESULTS;

__mram_noinit uint8_t DPU_BUFFER[BUFFER_SIZE];

/**
 * @fn main
 * @brief main function executed by each tasklet
 * @return the checksum result
 */
int main()
{
    uint32_t tasklet_id = me();
    uint8_t *cache = DPU_CACHES[tasklet_id];
    dpu_result_t *result = &DPU_RESULTS.tasklet_result[tasklet_id];
    uint32_t checksum[NB_CKSUM] = {0};
    uint32_t temp;
//    char print_buffer[PRINT_BUFFER_SIZE];

    /* Initialize once the cycle counter */
    if (tasklet_id == 0)
        perfcounter_config(COUNT_CYCLES, true);

    for (uint32_t buffer_idx = tasklet_id * BLOCK_SIZE; buffer_idx < BUFFER_SIZE; buffer_idx += (NR_TASKLETS * BLOCK_SIZE)) {

#ifdef RELOAD_MRAM
        for(uint32_t iter = 0; iter < NB_CKSUM; iter++) {
            /* load cache with current mram block. */
            mram_read(&DPU_BUFFER[buffer_idx], cache, BLOCK_SIZE);
#else
        /* load cache with current mram block. */
        mram_read(&DPU_BUFFER[buffer_idx], cache, BLOCK_SIZE);

        for(uint32_t iter = 0; iter < NB_CKSUM; iter++) {
#endif
            /* computes the checksum of a cached block */
            for (uint32_t cache_idx = 0; cache_idx < BLOCK_SIZE; cache_idx = cache_idx + 4) {
                temp = (cache[cache_idx+3] * cache[cache_idx+2] << 16) + cache[cache_idx+1] * cache[cache_idx];
                checksum[iter] += ROTLEFT(temp,16);;
            }
        }

    }


    for(uint32_t iter = 0; iter < NB_CKSUM; iter++)
        result->checksum[iter] = checksum[iter];
    /* keep the 32-bit LSB on the 64-bit cycle counter */
    result->cycles = (uint32_t)perfcounter_get();

    #ifdef VERBOSE
    /*
    uint32_t n = 0;
    n = sprintf(print_buffer, "[%02d] Checksum =", tasklet_id);
    for(uint32_t iter = 0; iter < NB_CKSUM - 1; iter++){
        n += sprintf(print_buffer + n , " 0x%08x", checksum[iter]);
    }
    n += sprintf(print_buffer + n, "\n");
    printf("%s", print_buffer);
    */
    //printf("[%02d] Checksum = 0x%08x 0x%08x 0x%08x\n", tasklet_id, checksum[0], checksum[1], checksum[2]);
    printf("[%02d] Checksum = 0x%08x\n", tasklet_id, result->checksum[0]);
    #endif

    return 0;
}
