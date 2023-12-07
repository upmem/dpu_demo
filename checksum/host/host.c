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

#include <dpu.h> // for DPU_ASSERT, DPU_FOREACH, dpu_alloc, dpu_set_t
#include <dpu_management.h> // for dpu_get_member_id, dpu_get_rank, dpu_...
#include <dpu_target_macros.h> // for DPU_TARGET_MASK
#include <stdbool.h> // for bool, true
#include <stdint.h> // for uint32_t, uint8_t
#include <stdio.h> // for printf, NULL, stdout
#include <stdlib.h> // for free, malloc, rand, srand

#include "common.h" // for dpu_results_t, dpu_result_t, BUFFER_SIZE

#ifndef DPU_BINARY
#define DPU_BINARY "build/checksum_dpu"
#endif

#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_RESET "\x1b[0m"

/**
 * @brief creates a "test file"
 *
 * @return the theorical checksum value
 */
static uint32_t create_test_file(uint8_t *test_file)
{
    uint32_t checksum = 0;
    srand(0);

    for (unsigned int i = 0; i < BUFFER_SIZE; i++) {
        test_file[i] = (unsigned char)(rand());
        checksum += test_file[i];
    }

    return checksum;
}

/**
 * @brief Checks the DPU result against the theoretical checksum.
 *
 * This function compares the checksum calculated by the DPU with the theoretical checksum
 * to determine if the DPU result is correct. It also outputs the DPU execution time and
 * its performance.
 *
 * @param results The DPU results containing the calculated checksum.
 * @param theoretical_checksum The theoretical checksum to compare against.
 * @return true if the DPU result matches the theoretical checksum, false otherwise.
 */
bool check_dpu_result(dpu_results_t *results, uint32_t theoretical_checksum)
{
    uint32_t dpu_checksum = 0;
    uint32_t dpu_cycles = 0;

    // Retrieve tasklet results and compute the final checksum.
    for (unsigned int each_tasklet = 0; each_tasklet < NR_TASKLETS; each_tasklet++) {
        dpu_result_t *result = &results->tasklet_result[each_tasklet];

        dpu_checksum += result->checksum;
        if (result->cycles > dpu_cycles) {
            dpu_cycles = result->cycles;
        }
    }

    bool dpu_status = (dpu_checksum == theoretical_checksum);

    printf("DPU execution time  = %g cycles\n", (double)dpu_cycles);
    printf("performance         = %g cycles/byte\n", (double)dpu_cycles / BUFFER_SIZE);
    printf("checksum computed by the DPU = 0x%08x\n", dpu_checksum);
    printf("actual checksum value        = 0x%08x\n", theoretical_checksum);
    if (dpu_status) {
        printf("[" ANSI_COLOR_GREEN "OK" ANSI_COLOR_RESET "] checksums are equal\n");
    } else {
        printf("[" ANSI_COLOR_RED "ERROR" ANSI_COLOR_RESET "] checksums differ!\n");
    }

    return dpu_status;
}

/**
 * Outputs the list of faulty DPUs.
 *
 * @param faulty_dpus Pointer to the set of faulty DPUs.
 * @param n_faulty_dpu Number of faulty DPUs.
 */
void output_faulty_dpus(struct dpu_set_t *faulty_dpus, uint32_t n_faulty_dpu)
{
    if (n_faulty_dpu > 0) {
        printf("Faulty DPUs:\n");
        for (uint32_t i = 0; i < n_faulty_dpu; i++) {
            dpu_id_t rank_id = dpu_get_rank_id(dpu_get_rank(faulty_dpus[i].dpu)) & DPU_TARGET_MASK;
            dpu_slice_id_t slice_id = dpu_get_slice_id(faulty_dpus[i].dpu);
            dpu_member_id_t member_id = dpu_get_member_id(faulty_dpus[i].dpu);
            printf("  - RANK %d, SLICE %d, DPU %d\n", rank_id, slice_id, member_id);
        }
    }
}

/**
 * @brief Outputs the results of the DPUs computations and compares the checksums with the theoretical checksum.
 *
 * @param dpu_set The DPU set containing the DPUs.
 * @param nr_of_dpus The number of DPUs in the set.
 * @param results Pointer to the results structure.
 * @param theoretical_checksum The theoretical checksum to compare with.
 * @return true if the checksum matches the theoretical checksum for all DPUs, false otherwise.
 */
bool output_results(struct dpu_set_t dpu_set, uint32_t nr_of_dpus, dpu_results_t *results, uint32_t theoretical_checksum)
{
    struct dpu_set_t *faulty_dpus = malloc(sizeof(struct dpu_set_t[nr_of_dpus]));
    bool status = true;
    uint32_t n_faulty_dpu = 0;

    struct dpu_set_t dpu;
    uint32_t each_dpu = 0;
    DPU_FOREACH (dpu_set, dpu, each_dpu) {
        bool dpu_status = check_dpu_result(&results[each_dpu], theoretical_checksum);
        if (!dpu_status) {
            faulty_dpus[n_faulty_dpu++] = dpu;
        }
        status &= dpu_status;
    }

    output_faulty_dpus(faulty_dpus, n_faulty_dpu);

    free(faulty_dpus);
    return status;
}

/**
 * @brief Main of the Host Application.
 */
int main()
{
    struct dpu_set_t dpu_set;

    DPU_ASSERT(dpu_alloc(NR_DPUS, NULL, &dpu_set));
    DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY, NULL));

    uint32_t nr_of_dpus = 0;
    DPU_ASSERT(dpu_get_nr_dpus(dpu_set, &nr_of_dpus));
    printf("Allocated %u DPU(s)\n", nr_of_dpus);

    // Create an "input file" with arbitrary data.
    // Compute its theoretical checksum value.
    static uint8_t test_file[BUFFER_SIZE];
    uint32_t theoretical_checksum = create_test_file(test_file);

    printf("Load input data\n");
    DPU_ASSERT(dpu_copy_to(dpu_set, XSTR(DPU_BUFFER), 0, test_file, BUFFER_SIZE));

    printf("Run program on DPU(s)\n");
    DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));

    struct dpu_set_t dpu;
    DPU_FOREACH (dpu_set, dpu) {
        DPU_ASSERT(dpu_log_read(dpu, stdout));
    }

    printf("Retrieve results\n");
    dpu_results_t results[nr_of_dpus];
    uint32_t each_dpu = 0;
    DPU_FOREACH (dpu_set, dpu, each_dpu) {
        DPU_ASSERT(dpu_prepare_xfer(dpu, &results[each_dpu]));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, XSTR(DPU_RESULTS), 0, sizeof(dpu_results_t), DPU_XFER_DEFAULT));

    bool status = output_results(dpu_set, nr_of_dpus, results, theoretical_checksum);

    DPU_ASSERT(dpu_free(dpu_set));
    return status ? 0 : -1;
}
