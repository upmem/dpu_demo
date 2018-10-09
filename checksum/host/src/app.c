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
#include <dpulog.h>

// Define the DPU Binary path as DPU_BINARY here.
// If you want to launch the application from the root of the project, without
// changing any file location, just uncomment the following define:
#define DPU_BINARY "bin/dpu/dpu_app.bin"

// Define the DPU Log File Directory as DPU_LOG_DIRECTORY here.
// If you want to launch the application from the root of the project, without
// changing any file location, just uncomment the following define:
#define DPU_LOG_DIRECTORY "log/host/"

#define NB_OF_TASKLETS_PER_DPU 16

#define PRINT_ERROR(fmt, ...) fprintf(stderr, "ERROR: "fmt"\n", ##__VA_ARGS__)

#ifndef DPU_TYPE
#define DPU_TYPE FUNCTIONAL_SIMULATOR
#endif /* !DPU_TYPE */
#ifdef DPU_PROFILE
#define STR(a) #a
#define STRINGIFY(a) STR(a)
#define DPU_PROFILE_STR STRINGIFY(DPU_PROFILE)
#else
#define DPU_PROFILE_STR ""
#endif /* !DPU_PROFILE */

#ifndef DPU_BINARY

#error The path to the DPU Binary must be defined as DPU_BINARY

#else

#ifndef DPU_LOG_DIRECTORY

#error The path to the DPU Log Directory must be defined as DPU_LOG_DIRECTORY

#else

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_RESET   "\x1b[0m"

/**
* @brief Allocate and initialize the DPUs.
*
* @param rank the unique identifier of the allocated rank
* @param dpu_type the target type of the DPUs
* @return 0 in case of success, -1 otherwise.
*/
static int init_dpus(dpu_rank_t *rank, dpu_type_t dpu_type);

/**
* @brief Run the DPUs in parallel, until the end of their execution.
*
* @param rank the targeted rank
* @param nr_of_dpus number of dpus in the rank
* @return 0 in case of success, -1 otherwise.
*/
static int run_dpus(dpu_rank_t rank, uint32_t nr_of_dpus);

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
static bool post_file_to_dpu(dpu_t dpu, unsigned char *buffer, unsigned int nr_bytes) {
    return dpu_copy_to_individual(dpu, buffer, 0, (size_t) nr_bytes) == DPU_API_SUCCESS &&
            dpu_tasklet_post_individual(dpu, ANY_TASKLET, 0, &nr_bytes, sizeof(nr_bytes)) == DPU_API_SUCCESS;
}

static void summarize_performance_of(dpu_t dpu, unsigned int nr_bytes, uint32_t cycles) {
    double cc = cycles;
    printf("DPU execution time  = %g cc\n", cc);
    printf("performance         = %g cc/byte\n", cc / nr_bytes);
}

/**
* @brief Main of the Host Application.
*/
int main(int argc, char **argv) {
    dpu_rank_t rank;
    uint32_t nr_of_dpus;
    unsigned int each_tasklet, i;
    unsigned int theoretical_checksum = 0;
    /* Use 8MB of input. */
    const unsigned int file_size = 8 << 20;
    bool status = false;

    dpu_type_t dpu_type = DPU_TYPE;

    if (argc >= 2) {
        char *dpu_type_string = argv[1];

        if (strcmp(dpu_type_string, "fsim") == 0) {
            dpu_type = FUNCTIONAL_SIMULATOR;
        } else if (strcmp(dpu_type_string, "asic") == 0) {
            dpu_type = HW;
        } else if (strcmp(dpu_type_string, "fpga") == 0) {
            dpu_type = HW;
        }
    }

    if (init_dpus(&rank, dpu_type) != 0) {
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
    for (i = 0; i < nr_of_dpus; i++) {
        if (!post_file_to_dpu(dpu_get_id(rank, i), buffer, file_size)) {
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
    for (i = 0; i < nr_of_dpus; i++) {
        printf("DPU#%d:\n", i);
        if (!dpulog_read_for_dpu(dpu_get_id(rank, i), stdout)) {
            PRINT_ERROR("cannot display DPU log correctly");
            goto err;
        }
    }

    printf("Retrieve results\n");
    struct result {
        uint32_t checksum;
        uint32_t cycles;
    } *results;
    results = calloc(sizeof(results[0]), nr_of_dpus);
    if (!results)
        goto err;
    for (i = 0; i < nr_of_dpus; i++) {
        results[i].checksum = 0;
        results[i].cycles = 0;
        // Retrieve tasklet results and compute the final checksum.
        for (each_tasklet = 0; each_tasklet < NB_OF_TASKLETS_PER_DPU; each_tasklet++) {
            struct result tasklet_result = { 0 };
            if (dpu_tasklet_receive_individual(dpu_get_id(rank, i), each_tasklet, 0, (uint32_t *)&tasklet_result, sizeof(tasklet_result)) != DPU_API_SUCCESS) {
                PRINT_ERROR("cannot receive DPU results correctly");
                free(results);
                goto err;
            }
            results[i].checksum += tasklet_result.checksum;
            if (tasklet_result.cycles > results[i].cycles)
                results[i].cycles = tasklet_result.cycles;
        }
    }
    for (i = 0; i < nr_of_dpus; i++) {
        summarize_performance_of(dpu_get_id(rank, i), file_size, results[i].cycles);
        printf("checksum computed by the DPU = 0x%08x\n", results[i].checksum);
        printf("actual checksum value        = 0x%08x\n", theoretical_checksum);

        status = (results[i].checksum == theoretical_checksum);

        if (status) {
            printf("[" ANSI_COLOR_GREEN "OK" ANSI_COLOR_RESET "] checksums are equal\n");
        } else {
            printf("[" ANSI_COLOR_RED "ERROR" ANSI_COLOR_RESET "] checksums differ!\n");
        }
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

static int init_dpus(dpu_rank_t *rank, dpu_type_t dpu_type) {
    dpu_logging_config_t log_config = {
            .source = KTRACE,
            .destination_directory_name = DPU_LOG_DIRECTORY
    };

    struct dpu_param params = {
            .type = dpu_type,
            .profile = DPU_PROFILE_STR,
            .logging_config = &log_config
    };

    if (dpu_alloc(&params, rank) != DPU_API_SUCCESS)
        return -1;

    if (dpu_load_all(*rank, DPU_BINARY) != DPU_API_SUCCESS) {
        dpu_free(*rank);
        return -1;
    }

    return 0;
}

static int run_dpus(dpu_rank_t rank, uint32_t nr_of_dpus) {
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
#endif /* DPU_LOG_DIRECTORY */
