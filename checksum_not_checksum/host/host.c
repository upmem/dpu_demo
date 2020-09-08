/*
 * Copyright (c) 2014-2019 - UPMEM
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
 * @file host.c
 * @brief Template for a Host Application Source File.
 */
#define _GNU_SOURCE
#include <dpu.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "common.h"
#include <dpu_management.h>
#include <dpu_runner.h>

#ifndef DPU_BINARY
#define DPU_BINARY "build/checksum_dpu"
#endif

#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_RESET "\x1b[0m"

static uint8_t test_file[BUFFER_SIZE];

static uint8_t *dpu_test_file;
/**
 * @brief creates a "test file"
 *
 * @return the theorical checksum value
 */
static uint64_t create_test_file()
{
    uint64_t checksum = 0;
    struct timeval time;

    gettimeofday(&time,NULL);

     // microsecond has 1 000 000
     // Assuming you did not need quite that accuracy
     // Also do not assume the system clock has that accuracy.
     srand((time.tv_sec * 1000) + (time.tv_usec / 1000));

    for (unsigned int i = 0; i < BUFFER_SIZE; i++) {
        test_file[i] = (unsigned char)(rand());
        checksum += test_file[i];
    }

    return checksum;
}

//static void compute_and_check_checksum(uint64_t theoretical_checksum, uint8_t *ptr)
//{
//    uint64_t checksum = 0;
//
//    for (unsigned int i = 0; i < BUFFER_SIZE; ++i) {
//        checksum += ptr[i];
//    }
//
//    if (checksum != theoretical_checksum) {
//        printf("[" ANSI_COLOR_RED "ERROR" ANSI_COLOR_RESET "] checksums differ!\n");
//        while (1);
//    }
//}

/**
 * @brief Main of the Host Application.
 */
int main()
{
    struct dpu_set_t dpu_set, dpu;
    uint32_t nr_of_dpus;
    uint32_t each_dpu;
    bool status = true;

    DPU_ASSERT(dpu_alloc(NR_DPUS, NULL, &dpu_set));
    DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY, NULL));

    DPU_ASSERT(dpu_get_nr_dpus(dpu_set, &nr_of_dpus));
    printf("Allocated %d DPU(s)\n", nr_of_dpus);

    dpu_test_file = mmap(0, nr_of_dpus * 2048, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (dpu_test_file == MAP_FAILED) {
        printf("Error allocating dpu_test_file\n");
        exit(-1);
    }

    // Create an "input file" with arbitrary data.
    // Compute its theoretical checksum value.
    for (int i = 0; i < 50000; ++i) {
        create_test_file();

        //printf("Load input data\n");
        DPU_ASSERT(dpu_copy_to(dpu_set, XSTR(DPU_BUFFER), 0, test_file, BUFFER_SIZE));

        //printf("Run program on DPU(s)\n");
        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));

        // Read back what's in MRAM and compute the checksum from the host again.
        // 2/ Read mram
        DPU_FOREACH (dpu_set, dpu, each_dpu) {
            DPU_ASSERT(dpu_prepare_xfer(dpu, &dpu_test_file[each_dpu * 2048]));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, XSTR(DPU_BUFFER), 0, 2048, DPU_XFER_DEFAULT));

        // 3/ Check the end mark
        DPU_FOREACH (dpu_set, dpu, each_dpu) {
            for (int j = 0; j < 2048; ++j) {
                if (((uint8_t *)&dpu_test_file[each_dpu * 2048])[j] != 0) {
                    printf("ERROR not null in dpu%d byte %d at pass %d!!\n", each_dpu, j, i);
                }
            }
        }
    }

    DPU_ASSERT(dpu_free(dpu_set));
    return status ? 0 : -1;
}
