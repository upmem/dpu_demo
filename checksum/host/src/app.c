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
* The macros DPU_BINARY, NB_OF_DPUS and NB_OF_TASKLETS_PER_DPU are directly
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

#define NB_OF_DPUS 8
#define NB_OF_TASKLETS_PER_DPU 16

#define PRINT_ERROR(fmt, ...) fprintf(stderr, "ERROR: "fmt"\n", ##__VA_ARGS__)

#ifndef DPU_TYPE
#define DPU_TYPE FUNCTIONAL_SIMULATOR
#endif /* !DPU_TYPE */

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
* @param dpus an array to store the DPU instances
* @param dpu_type the target type of the DPUs
* @return 0 in case of success, -1 otherwise.
*/
static int init_dpus(dpu_t *dpus, dpu_type_t dpu_type);

/**
* @brief Free the allocated DPUs.
*
* @param dpus an array of the DPUs to be freed
* @return 0 in case of success, -1 otherwise.
*/
static int free_dpus(dpu_t *dpus);

/**
* @brief Run the DPUs in parallel, until the end of their execution.
*
* @param dpus an array of the DPUs to be run
* @return 0 in case of success, -1 otherwise.
*/
static int run_dpus(dpu_t *dpus);

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
*/
static void post_file_to_dpu(dpu_t dpu, unsigned char *buffer, unsigned int nr_bytes) {
    copy_to_dpu(dpu, buffer, 0, (size_t) nr_bytes);
    dpu_post(dpu, ANY_TASKLET, 0, &nr_bytes, sizeof(nr_bytes));
}

static void summarize_performance_of(dpu_t dpu, unsigned int nr_bytes) {
    double time = dpu_time(dpu);
    printf("DPU execution time  = %g cc\n", time);
    printf("performance         = %g cc/byte\n", time / nr_bytes);
}

/**
* @brief Main of the Host Application.
*/
int main(int argc, char **argv) {
    dpu_t dpus[NB_OF_DPUS];
    unsigned int each_tasklet, i;
    unsigned int checksum[NB_OF_DPUS], theoretical_checksum = 0;
    /* Use 8MB of input. */
    const unsigned int file_size = 8 << 20;
    bool status = false;

    dpu_type_t dpu_type = DPU_TYPE;

    if (argc >= 2) {
        char *dpu_type_string = argv[1];

        if (strcmp(dpu_type_string, "fsim") == 0) {
            dpu_type = FUNCTIONAL_SIMULATOR;
        } else if (strcmp(dpu_type_string, "hsim") == 0) {
            dpu_type = HSIM;
        } else if (strcmp(dpu_type_string, "fpga") == 0) {
            dpu_type = FPGA;
        }
    }

    printf("Init %d DPU(s)\n", NB_OF_DPUS);
    if (init_dpus(dpus, dpu_type) != 0) {
        PRINT_ERROR("cannot initialize DPUs");
        return -1;
    }

    // Create an "input file" with arbitrary data.
    // Compute its theoretical checksum value.
    uint8_t *buffer = create_test_file(file_size, &theoretical_checksum);

    printf("Load input data\n");
    for (i = 0; i < NB_OF_DPUS; i++) {
        post_file_to_dpu(dpus[i], buffer, file_size);
    }

    printf("Run program on DPU(s) \n");
    if (run_dpus(dpus) != 0) {
        PRINT_ERROR("cannot execute program correctly");
        goto err;
    }

    printf("Display DPU Logs\n");
    for (i = 0; i < NB_OF_DPUS; i++) {
        printf("DPU#%d:\n", i);
        if (!dpulog_read_for_dpu(dpus[i], stdout)) {
            PRINT_ERROR("cannot display DPU log correctly");
            goto err;
        }
    }

    printf("Retrieve results\n");
    for (i = 0; i < NB_OF_DPUS; i++) {
        checksum[i] = 0;
        // Retrieve tasklet results and compute the final checksum.
        for (each_tasklet = 0; each_tasklet < NB_OF_TASKLETS_PER_DPU; each_tasklet++) {
            unsigned int tasklet_result = 0;
            dpu_receive(dpus[i], each_tasklet, 0, &tasklet_result, sizeof(tasklet_result));
            checksum[i] += tasklet_result;
        }
    }
    for (i = 0; i < NB_OF_DPUS; i++) {
        summarize_performance_of(dpus[i], file_size);
        printf("checksum computed by the DPU = 0x%08x\n", checksum[i]);
        printf("actual checksum value        = 0x%08x\n", theoretical_checksum);

        status = (checksum[i] == theoretical_checksum);

        if (status) {
            printf("[" ANSI_COLOR_GREEN "OK" ANSI_COLOR_RESET "] checksums are equal\n");
        } else {
            printf("[" ANSI_COLOR_RED "ERROR" ANSI_COLOR_RESET "] checksums differ!\n");
        }
    }
    if (free_dpus(dpus) != 0) {
        PRINT_ERROR("cannot free DPUs");
        return -1;
    }

    return status ? 0 : 1;

    err:
    free_dpus(dpus);
    return -1;
}

static int init_dpus(dpu_t *dpus, dpu_type_t dpu_type) {
    dpu_logging_config_t log_config = {
            .source = KTRACE,
            .destination_directory_name = DPU_LOG_DIRECTORY
    };

    struct dpu_param params = {
            .type = dpu_type,
            .logging_config = &log_config
    };

    for (int each_dpu = 0; each_dpu < NB_OF_DPUS; ++each_dpu) {
        if ((dpus[each_dpu] = dpu_alloc(&params)) == NO_DPU_AVAILABLE)
            goto err;

        if (!dpu_load(dpus[each_dpu], DPU_BINARY))
            goto err;
    }

    return 0;

    err:
    for (int each_dpu = 0; each_dpu < NB_OF_DPUS; ++each_dpu) {
        if (dpus[each_dpu] == NO_DPU_AVAILABLE)
            break;
        dpu_free(dpus[each_dpu]);
    }

    return -1;
}

static int free_dpus(dpu_t *dpus) {
    for (int each_dpu = 0; each_dpu < NB_OF_DPUS; ++each_dpu)
        dpu_free(dpus[each_dpu]);

    return 0;
}

static int run_dpus(dpu_t *dpus) {
    int nb_of_booted_dpus = 0;
    int nb_of_stopped_dpus = 0;
    int res = 0;

    for (int each_dpu = 0; each_dpu < NB_OF_DPUS; ++each_dpu) {
        if (!dpu_boot(dpus[each_dpu], NO_WAIT)) {
            res = -1;
            break;
        }
        ++nb_of_booted_dpus;
    }

    do {
        nb_of_stopped_dpus = 0;
        for (int each_booted_dpu = 0; each_booted_dpu < nb_of_booted_dpus; ++each_booted_dpu) {
            dpu_run_status_t status = dpu_get_status(dpus[each_booted_dpu]);
            switch (status) {
                case STATUS_RUNNING:
                    break;
                case STATUS_IDLE:
                    ++nb_of_stopped_dpus;
                    break;
                case STATUS_ERROR:
                case STATUS_INVALID_DPU:
                    res = -1;
                    ++nb_of_stopped_dpus;
                    break;
            }
        }
    } while (nb_of_stopped_dpus != nb_of_booted_dpus);

    return res;
}

#endif /* DPU_BINARY */
#endif /* DPU_LOG_DIRECTORY */
