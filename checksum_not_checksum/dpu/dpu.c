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
#include <string.h>
#include "common.h"

__mram_noinit uint8_t DPU_BUFFER[64 << 20]; //BUFFER_SIZE];
static int idx = 0;
/**
 * @fn main
 * @brief main function executed by each tasklet
 * @return the checksum result
 */
int main()
{
    uint32_t tasklet_id = me();

    if (tasklet_id == 0) {
        uint8_t wram_poison[256];
        uint8_t mram_read_poison[256];

        if (idx) {
            mram_read(&DPU_BUFFER[0 /*32 * 1024 * 1024*/], mram_read_poison, 256);
            for (int i = 0; i < 256; ++i)
                if (mram_read_poison[i] != ((idx - 1) & 0xFF))
                    halt();
        }

        memset(wram_poison, idx & 0xFF, 256);

        mram_write(wram_poison, &DPU_BUFFER[0 /*32 * 1024 * 1024*/], 256);
        mram_read(&DPU_BUFFER[0 /*32 * 1024 * 1024*/], mram_read_poison, 256);
        for (int i = 0; i < 256; ++i)
            if (mram_read_poison[i] != (idx & 0xFF))
                halt();
        idx++;
    }

    return 0;
}
