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
* Every tasklet reads the file size from the system mailbox and
* processes specific areas of the MRAM, following the "rake"
* strategy:
*  - Tasklet number T is first processing block number TxN, where
*    N is a constant block size
*  - It then handles block number (TxN) + (NxM) where M is the
*    number of scheduled tasklets
*  - And so on...
*
* The resulting checksum is posted back into each tasklet's
* mailbox, so that the host is in charge of computing the
* final checksum by adding all the individual results.
*/
#include <stdint.h>
#include <defs.h>
#include <sys.h>
#include <mbox.h>
#include <mram.h>
#include <alloc.h>
#include <mram.h>
#include <ktrace.h>
#include <perfcounter.h>

/*
 * Create a system with 16 tasklets (0 to 15) to compute individual checksums
 * with a 512-byte stack and two 32-bits words of mailbox per tasklet
 */
#define NR_TASKLETS_LOG2 4
#define NR_TASKLETS      16
#define TASKLETS_INITIALIZER \
    TASKLETS(NR_TASKLETS, task_main, 512, 2)
#include <rt.h>

/*
 * Post the input file size into the system mailbox (One 32-bits word),
 * accessible to any thread.
 */
SYSTEM_MAILBOX_INITIALIZER(1);

#define printf ktrace

// Use blocks of 256 bytes
#define BLOCK_SIZE_LOG2 8
#define BLOCK_SIZE (1 << BLOCK_SIZE_LOG2)

/**
* @fn compute_checksum
* @brief computes the checksum of a cached block of 256 bytes
* @param buffer the buffer address
* @return the sum of all bytes within this block
*/
static unsigned int compute_checksum(uint8_t *buffer) {
    unsigned int i, result = 0;
    for (i = 0; i < BLOCK_SIZE; i++) result += buffer[i];
    return result;
}

/**
* @fn task_main
* @brief main function executed by each tasklet
* @return the checksum result
*/
int task_main() {
    unsigned int tasklet_id = me();
    unsigned int file_size = *((uint32_t *) sys_mbox_recv());
    if (tasklet_id == 0) /* Initialize once the cycle counter */
        perfcounter_config(COUNT_CYCLES, true);
    /* Address of the current processing block in MRAM. */
    mram_addr_t current_mram_block_addr = (mram_addr_t)(tasklet_id << BLOCK_SIZE_LOG2);
    /* Initialize a local cache to store the MRAM block. */
    uint8_t *cache = (uint8_t *) mem_alloc_dma(BLOCK_SIZE);
    struct result {
        uint32_t checksum;
        uint32_t cycles;
    } result = { 0 };

    for (;
            current_mram_block_addr < file_size;
            current_mram_block_addr += (BLOCK_SIZE << NR_TASKLETS_LOG2)
            ) {
        /* Load cache with current MRAM block. */
        mram_read256(current_mram_block_addr, cache);
        result.checksum += compute_checksum(cache);
    }
    /* keep the 32-bit LSB on the 64-bit cycle counter */
    result.cycles = perfcounter_get();
    printf("[%02d] Checksum = 0x%08x\n", tasklet_id, result.checksum);
    /* Send the resulting checksum and cycle count to host application. */
    mbox_send(&result, sizeof(result));
    return result.checksum;
}
