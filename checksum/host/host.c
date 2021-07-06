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

#include <dpu.h>
#include <dpu_management.h>
#include <dpu_target_macros.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"

#ifndef DPU_BINARY
#define DPU_BINARY "build/checksum_dpu"
#endif

#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_RESET "\x1b[0m"

#define ROTLEFT(a,b) (((a) << (b)) | ((a) >> (32-(b))))
#define ROTRIGHT(a,b) (((a) >> (b)) | ((a) << (32-(b))))

static uint8_t test_file[BUFFER_SIZE];

/**
 * @brief creates a "test file"
 *
 * @return the theorical checksum value
 */
static uint32_t create_test_file()
{

    uint32_t checksum = 0;
    uint32_t temp;

    for (unsigned int i = 0; i < BUFFER_SIZE; i++) {
        test_file[i] = (unsigned char)(rand());
    }

    for (unsigned int i = 0; i < BUFFER_SIZE; i=i+4) {
        temp = (test_file[i+3] * test_file[i+2] << 16) + test_file[i+1] * test_file[i];
        checksum += ROTLEFT(temp,16);
    }

    return checksum;
}

/**
 * @brief Main of the Host Application.
 */
int main(int argc, char **argv)
{
    struct dpu_set_t dpu_set, dpu;
    uint32_t nr_of_dpus;
    uint32_t theoretical_checksum;
    uint32_t dpu_checksum[NB_CKSUM] = {0};
    uint32_t dpu_cycles;
    bool status = true;
    const int iterations= argc > 1 ? atoi(argv[1]) : 0;

    DPU_ASSERT(dpu_alloc(NR_DPUS, NULL, &dpu_set));
    DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY, NULL));

    DPU_ASSERT(dpu_get_nr_dpus(dpu_set, &nr_of_dpus));
    printf("[INFO] Allocated %d DPU(s)\n", nr_of_dpus);

    uint8_t *dpu_buffers = (uint8_t*)calloc(nr_of_dpus, BUFFER_SIZE);

    #ifdef VERBOSE
    printf("[INFO] Iterations %i\n",iterations);
    printf("[INFO] SEED %i\n", SEED);
    #endif

    srand(SEED);

    for(int i = 1; i <= iterations; i++) {
        // Create an "input file" with arbitrary data.
        // Compute its theoretical checksum value.
        theoretical_checksum = create_test_file();

        printf("\r[INFO] PASS %i/%i", i,iterations);
        #ifdef VERBOSE
        printf("\n");
        #else
        fflush(stdout);
        #endif

        #ifdef VERBOSE
        printf("[INFO] Load input data\n");
        #endif
        DPU_ASSERT(dpu_copy_to(dpu_set, XSTR(DPU_BUFFER), 0, test_file, BUFFER_SIZE));

        #ifdef VERBOSE
        printf("[INFO] Run program on DPU(s)\n");
        #endif
        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));

        #ifdef VERBOSE
        DPU_FOREACH (dpu_set, dpu) {
            DPU_ASSERT(dpu_log_read(dpu, stdout));
        }
        #endif

        #ifdef VERBOSE
        printf("Retrieve results\n");
        #endif

        dpu_results_t results[nr_of_dpus];
        uint32_t each_dpu;
        DPU_FOREACH (dpu_set, dpu, each_dpu) {
            DPU_ASSERT(dpu_prepare_xfer(dpu, &results[each_dpu]));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, XSTR(DPU_RESULTS), 0, sizeof(dpu_results_t), DPU_XFER_DEFAULT));

        DPU_FOREACH (dpu_set, dpu, each_dpu) {
            bool dpu_status;
            for(uint32_t iter = 0; iter < NB_CKSUM; iter++)
                dpu_checksum[iter] = 0;
            dpu_cycles = 0;

            // Retrieve tasklet results and compute the final checksum.
            for (unsigned int each_tasklet = 0; each_tasklet < NR_TASKLETS; each_tasklet++) {
                dpu_result_t *result = &results[each_dpu].tasklet_result[each_tasklet];

                for(uint32_t iter = 0; iter < NB_CKSUM; iter++)
                    dpu_checksum[iter] += result->checksum[iter];
                if (result->cycles > dpu_cycles)
                    dpu_cycles = result->cycles;
            }

            dpu_status = (dpu_checksum[0] == theoretical_checksum);
            status = status && dpu_status;

            struct dpu_t *dpu_t = dpu_from_set(dpu);

            //printf("[INFO] DPU execution time  = %g cycles\n", (double)dpu_cycles);
            //printf("[INFO] Performance         = %g cycles/byte\n", (double)dpu_cycles / BUFFER_SIZE);

            if (!dpu_status) {
                printf("\n");
                uint32_t rank_id = dpu_get_rank_id(dpu_get_rank(dpu_t)) & DPU_TARGET_MASK;
                uint32_t ci_id = dpu_get_slice_id(dpu_t);
                uint32_t dpu_id = dpu_get_member_id(dpu_t);
                printf("[INFO] Checksum computed by the HOST = 0x%08x\n", theoretical_checksum);
                printf("[INFO] Checksum computed by the DPU  =");
                for(uint32_t iter = 0; iter < NB_CKSUM; iter++)
                    printf(" 0x%08x", dpu_checksum[iter]);
                printf("\n");
                printf("[" ANSI_COLOR_RED "ERROR" ANSI_COLOR_RESET "] checksums differ! => DPU %d.%d.%d\n", rank_id, ci_id, dpu_id);

                // checking integrity of the dpu buffer
                // copy back the buffer to check its integrity
                DPU_ASSERT(dpu_prepare_xfer(dpu, &dpu_buffers[each_dpu * BUFFER_SIZE]));
                DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, XSTR(DPU_BUFFER), 0, BUFFER_SIZE, DPU_XFER_DEFAULT));

                if((memcmp(test_file, &dpu_buffers[each_dpu * BUFFER_SIZE], BUFFER_SIZE))!=0)
                    printf("[INFO] DPU input (MRAM) buffer is " ANSI_COLOR_RED "CORRUPTED" ANSI_COLOR_RESET "\n");
                else
                    printf("[INFO] DPU input (MRAM) buffer is " ANSI_COLOR_GREEN "CORRECT" ANSI_COLOR_RESET "\n");

                }
                #ifdef VERBOSE
                else {
                    printf("[INFO] checksums are " ANSI_COLOR_GREEN "EQUAL" ANSI_COLOR_RESET "\n");
                }
                #endif
            }
        }

    printf("\n");
    printf("----------------------------------------------------------\n");

    if (status) {
        printf("[" ANSI_COLOR_GREEN "OK" ANSI_COLOR_RESET "] ALL CHECKSUMS ARE EQUAL\n");
    } else {
        printf("[" ANSI_COLOR_RED "ERROR" ANSI_COLOR_RESET "] ONE OR MORE CHECKSUMS DIFFER !\n");
    }


    DPU_ASSERT(dpu_free(dpu_set));
    free(dpu_buffers);
    return status ? 0 : -1;
}
