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
#include <dpu_management.h>
#include <dpu_config.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "common.h"

#ifndef DPU_BINARY
#define DPU_BINARY "build/checksum_dpu"
#endif

#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_RESET "\x1b[0m"

static uint8_t test_file[BUFFER_SIZE];

/**
 * @brief creates a "test file"
 *
 * @return the theorical checksum value
 */
static uint32_t create_test_file()
{
    uint32_t checksum = 0;
    srand(time(NULL));

    for (unsigned int i = 0; i < BUFFER_SIZE; i++) {
        test_file[i] = (unsigned char)(rand());
        checksum += test_file[i];
    }

    return checksum;
}

static int
checksum_fork(const char *exec, char **args)
{
    pid_t pid;
    int status;

    pid = fork();
    if (!pid) {
        if (execvp(exec, args))
            exit(-1);

        exit(0);
    }

    waitpid(pid, &status, 0);
    if (WEXITSTATUS(status))
        return -1;

    return 0;
}

static int checksum_disable_dpu(int rank_id, int slice_id, int dpu_id)
{
    char *args[6] = { NULL };

    printf("*** Disabling this dpu...\n");

    if (asprintf(&args[0], "upmem-dimm-configure.py") == -1 ||
            asprintf(&args[1], "--rank") == -1 ||
	    asprintf(&args[2], "/dev/dpu_rank%d", rank_id) == -1 ||
            asprintf(&args[3], "--disable-dpu") == -1 ||
	    asprintf(&args[4], "%d.%d", slice_id, dpu_id) == -1) {
        printf("Failed to prepare arguments\n");
        exit(-1);
    }

    int ret = checksum_fork("upmem-dimm-configure.py", args); 

    for (int i = 0; i < 4; ++i)
        free(args[i]);

    return ret;
}

/**
 * @brief Main of the Host Application.
 */
int main()
{
    struct dpu_set_t dpu_set, dpu;
    uint32_t nr_of_dpus;
    uint32_t theoretical_checksum, dpu_checksum;
    uint32_t dpu_cycles;
    bool status = true, disabled_dpu = true;

    for (int i = 0; i < 100000; ++i) {
	    if (disabled_dpu) {
		    DPU_ASSERT(dpu_alloc(NR_DPUS, NULL, &dpu_set));
		    DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY, NULL));
		    DPU_ASSERT(dpu_get_nr_dpus(dpu_set, &nr_of_dpus));
		    printf("Allocated %d DPU(s)\n", nr_of_dpus);
		    disabled_dpu = false;
	    }

	    // Create an "input file" with arbitrary data.
	    // Compute its theoretical checksum value.
	    theoretical_checksum = create_test_file();

	    DPU_ASSERT(dpu_copy_to(dpu_set, XSTR(DPU_BUFFER), 0, test_file, BUFFER_SIZE));
	    DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));

	    dpu_results_t results[nr_of_dpus];
	    uint32_t each_dpu;
	    DPU_FOREACH (dpu_set, dpu, each_dpu) {
		    DPU_ASSERT(dpu_prepare_xfer(dpu, &results[each_dpu]));
	    }
	    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, XSTR(DPU_RESULTS), 0, sizeof(dpu_results_t), DPU_XFER_DEFAULT));

	    bool dpu_status = false;
	    DPU_FOREACH (dpu_set, dpu, each_dpu) {
		    dpu_checksum = 0;
		    dpu_cycles = 0;

		    // Retrieve tasklet results and compute the final checksum.
		    for (unsigned int each_tasklet = 0; each_tasklet < NR_TASKLETS; each_tasklet++) {
			    dpu_result_t *result = &results[each_dpu].tasklet_result[each_tasklet];

			    dpu_checksum += result->checksum;
			    if (result->cycles > dpu_cycles)
				    dpu_cycles = result->cycles;
		    }

		    dpu_status = (dpu_checksum == theoretical_checksum);
		    status = status && dpu_status;
		    if (!dpu_status) {
			    int rank_id, dpu_id, slice_id;

			    rank_id = dpu_get_rank_id(dpu_get_rank(dpu_from_set(dpu))) & 0xFF;
			    slice_id = dpu_get_slice_id(dpu_from_set(dpu));
			    dpu_id = dpu_get_member_id(dpu_from_set(dpu));

			    DPU_ASSERT(dpu_free(dpu_set));
			    disabled_dpu = true;

			    printf("[" ANSI_COLOR_RED "ERROR" ANSI_COLOR_RESET "] checksums differ %d.%d.%d!\n", rank_id, slice_id, dpu_id);

			    checksum_disable_dpu(rank_id, slice_id, dpu_id);
			    goto reloop;
		    }
	    }

reloop:
	    if (dpu_status) {
		    printf("[" ANSI_COLOR_GREEN "OK" ANSI_COLOR_RESET "] checksums are equal\n");
	    } else {
		    printf("[" ANSI_COLOR_RED "ERROR" ANSI_COLOR_RESET "] checksums differ!\n");
	    }
    }

    DPU_ASSERT(dpu_free(dpu_set));
    return status ? 0 : -1;
}
