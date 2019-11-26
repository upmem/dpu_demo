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
* @file app.c
* @brief Template for a Host Application Source File.
*
* The macros DPU_BINARY and NB_OF_TASKLETS_PER_DPU are directly
* used in the static functions, and are not passed as arguments of these functions.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <dpu.h>
#include <dpu_log.h>

// Define the DPU Binary path as DPU_BINARY here.
// If you want to launch the application from the root of the project, without
// changing any file location, just uncomment the following define:
#define DPU_BINARY "bin/dpu/dpu_app.bin"

#define NB_OF_TASKLETS_PER_DPU 16

#define PRINT_ERROR(fmt, ...) fprintf(stderr, "ERROR: "fmt"\n", ##__VA_ARGS__)

#ifndef DPU_BINARY

#error The path to the DPU Binary must be defined as DPU_BINARY

#else

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_RESET   "\x1b[0m"

/**
* @brief Allocate and initialize the DPUs.
*
* @param rank the unique identifier of the allocated rank
* @return 0 in case of success, -1 otherwise.
*/
static int init_dpus(struct dpu_rank_t **rank);

/**
* @brief Run the DPUs in parallel, until the end of their execution.
*
* @param rank the targeted rank
* @param nr_of_dpus number of dpus in the rank
* @return 0 in case of success, -1 otherwise.
*/
static int run_dpus(struct dpu_rank_t *rank, uint32_t nr_of_dpus);

static unsigned char test_file[64 << 20];

/**
* @brief creates a "test file" by filling a buffer of 64MB with pseudo-random values
* @param checksum set to the theoretical checksum value in this file
* @param nr_bytes how many bytes we want the file to be
* @return the buffer address
*/
static unsigned char *create_test_file(unsigned int nr_bytes, unsigned int *checksum) {
    unsigned int i;
    srand(0);
    *checksum = 0;

    for (i = 0; i < nr_bytes; i++) {
        test_file[i] = (unsigned char) (rand());
        *checksum += test_file[i];
    }

    return test_file;
}

/**
* @brief Writes a "test file" into the DPU MRAM and post its size
* @param dpu the target DPU
* @param buffer the buffer containing the test file
* @param nr_bytes the test file size, in bytes
* @return whether the post was successful
*/
static bool post_file_to_dpu(struct dpu_t *dpu, unsigned char *buffer, unsigned int nr_bytes) {
    return dpu_copy_to_dpu(dpu, buffer, 0, (size_t) nr_bytes) == DPU_API_SUCCESS &&
            dpu_tasklet_post(dpu, ANY_TASKLET, 0, &nr_bytes, sizeof(nr_bytes)) == DPU_API_SUCCESS;
}

static void summarize_performance_of(struct dpu_t *dpu, unsigned int nr_bytes, uint32_t cycles) {
    double cc = cycles;
    printf("DPU execution time  = %g cc\n", cc);
    printf("performance         = %g cc/byte\n", cc / nr_bytes);
}

/**
* @brief Main of the Host Application.
*/
int main(int argc, char **argv) {
    struct dpu_rank_t *rank;
    struct dpu_t *dpu;
    uint32_t nr_of_dpus;
    unsigned int each_tasklet;
    unsigned int theoretical_checksum = 0;
    /* Use 8MB of input. */
    const unsigned int file_size = 8 << 20;
    bool status = false;

    if (init_dpus(&rank) != 0) {
        PRINT_ERROR("cannot initialize DPUs");
        return -1;
    }
    if (dpu_get_nr_of_dpus_in(rank, &nr_of_dpus) != DPU_API_SUCCESS)
        goto err;
    printf("Allocated %d DPU(s)\n", nr_of_dpus);

    // Create an "input file" with arbitrary data.
    // Compute its theoretical checksum value.
    uint8_t *buffer = create_test_file(file_size, &theoretical_checksum);

    printf("Load input data\n");
    DPU_FOREACH(rank, dpu) {
        if (!post_file_to_dpu(dpu, buffer, file_size)) {
            PRINT_ERROR("cannot post file to DPU correctly");
            goto err;
        }
    }

    printf("Run program on DPU(s) \n");
    if (run_dpus(rank, nr_of_dpus) != 0) {
        PRINT_ERROR("cannot execute program correctly");
        goto err;
    }

    printf("Display DPU Logs\n");
    DPU_FOREACH(rank, dpu) {
        printf("DPU#%d:\n", dpu_get_member_id(dpu));
        if (dpulog_read_for_dpu(dpu, stdout) != DPU_API_SUCCESS) {
            PRINT_ERROR("cannot display DPU log correctly");
            goto err;
        }
    }

    printf("Retrieve results\n");
    struct result {
        uint32_t checksum;
        uint32_t cycles;
    } *results;
    struct result *result_ptr; 
    results = calloc(sizeof(results[0]), nr_of_dpus);
    if (!results)
        goto err;
    result_ptr = results;
    DPU_FOREACH(rank, dpu) {
        result_ptr->checksum = 0;
        result_ptr->cycles = 0;
        // Retrieve tasklet results and compute the final checksum.
        for (each_tasklet = 0; each_tasklet < NB_OF_TASKLETS_PER_DPU; each_tasklet++) {
            struct result tasklet_result = { 0 };
            if (dpu_tasklet_receive(dpu, each_tasklet, 0, (uint32_t *)&tasklet_result, sizeof(tasklet_result)) != DPU_API_SUCCESS) {
                PRINT_ERROR("cannot receive DPU results correctly");
                free(results);
                goto err;
            }
            result_ptr->checksum += tasklet_result.checksum;
            if (tasklet_result.cycles > result_ptr->cycles)
                result_ptr->cycles = tasklet_result.cycles;
        }
	result_ptr++;
    }
    result_ptr = results;
    DPU_FOREACH(rank, dpu) {
        summarize_performance_of(dpu, file_size, result_ptr->cycles);
        printf("checksum computed by the DPU = 0x%08x\n", result_ptr->checksum);
        printf("actual checksum value        = 0x%08x\n", theoretical_checksum);

        status = (result_ptr->checksum == theoretical_checksum);

        if (status) {
            printf("[" ANSI_COLOR_GREEN "OK" ANSI_COLOR_RESET "] checksums are equal\n");
        } else {
            printf("[" ANSI_COLOR_RED "ERROR" ANSI_COLOR_RESET "] checksums differ!\n");
        }
	result_ptr++;
    }
    free(results);
    if (dpu_free(rank) != DPU_API_SUCCESS) {
        PRINT_ERROR("cannot free DPUs");
        return -1;
    }

    return status ? 0 : 1;

    err:
    dpu_free(rank);
    return -1;
}

static int init_dpus(struct dpu_rank_t **rank) {
    if (dpu_alloc(NULL, rank) != DPU_API_SUCCESS)
        return -1;

    if (dpu_load_all(*rank, DPU_BINARY) != DPU_API_SUCCESS) {
        dpu_free(*rank);
        return -1;
    }

    return 0;
}

static int run_dpus(struct dpu_rank_t *rank, uint32_t nr_of_dpus) {
    uint32_t nb_of_running_dpus = 0;
    int res = 0;

    if (dpu_boot_all(rank, ASYNCHRONOUS) != DPU_API_SUCCESS)
        return -1;

    dpu_run_status_t *status = calloc(sizeof(dpu_run_status_t), nr_of_dpus);
    if (!status)
        return -1;
    do {
        dpu_get_all_status(rank, status, &nb_of_running_dpus);
    } while (nb_of_running_dpus != 0);

    free(status);
    return res;
}

#endif /* DPU_BINARY */
