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

#include "common.h"

/*
 * Create a system with 'NB_TASKLETS_PER_DPU' tasklets to compute individual
 * checksums with a 256-byte stack.
 */
#define TASKLETS_INITIALIZER TASKLETS(NB_TASKLETS_PER_DPU, main, 256, 0)
#include <rt.h>

/* Use blocks of 256 bytes */
#define BLOCK_SIZE (2048)

__dma_aligned uint8_t DPU_CACHES[NB_TASKLETS_PER_DPU][BLOCK_SIZE];
__dma_aligned dpu_results_t DPU_RESULTS[NB_TASKLETS_PER_DPU];

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
    dpu_results_t *result = &DPU_RESULTS[tasklet_id];
    uint32_t checksum = 0;
    const uint32_t nb_blocks = (BUFFER_SIZE + BLOCK_SIZE - 1) / BLOCK_SIZE;
    const uint32_t nb_blocks_per_dpu = nb_blocks / NB_TASKLETS_PER_DPU;

    /* Initialize once the cycle counter */
    if (tasklet_id == 0)
        perfcounter_config(COUNT_CYCLES, true);

    for (int32_t block_id = (tasklet_id + 1) * nb_blocks_per_dpu - 1; block_id >= (int32_t)(tasklet_id * nb_blocks_per_dpu);
         block_id--) {

        /* Load cache with current MRAM block. */
        MRAM_READ((mram_addr_t)&DPU_BUFFER[block_id * BLOCK_SIZE], cache, BLOCK_SIZE);

        /* computes the checksum of a cached block */
        for (uint32_t cache_idx = 0; cache_idx < BLOCK_SIZE; cache_idx++) {
            checksum += cache[cache_idx];
        }
    }

    /* keep the 32-bit LSB on the 64-bit cycle counter */
    result->cycles = (uint32_t)perfcounter_get();
    result->checksum = checksum;

    return 0;
}
