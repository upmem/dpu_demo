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

static uint8_t *dpu_test_file[64];

uint64_t concat_word(int i )
{
    uint64_t mod_i = (uint64_t)(i & 0xFF);
    return (mod_i << 56 | mod_i << 48 | mod_i << 40 | mod_i << 32 | mod_i << 24 | mod_i << 16 | mod_i << 8 | mod_i);
}

int nb_errors = 0;

dpu_error_t callback(struct dpu_set_t dpu_set, uint32_t rank_id, void *arg)
{
    struct dpu_set_t dpu;
    uint32_t each_dpu;
    int i = (int)(uintptr_t)arg;

    //printf("iteration = %d\n", i);

    //DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));

    DPU_FOREACH (dpu_set, dpu, each_dpu) {
        DPU_ASSERT(dpu_prepare_xfer(dpu, &dpu_test_file[rank_id][each_dpu * 256]));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, XSTR(DPU_BUFFER), 0 /* 32 * 1024 * 1024 */, 256, DPU_XFER_DEFAULT));

    // 3/ Check the end mark
    DPU_FOREACH (dpu_set, dpu, each_dpu) {
        for (uint32_t j = 0; j < 256 / sizeof(uint64_t); ++j) {
            if (((uint64_t *)&dpu_test_file[rank_id][each_dpu * 256])[j] != concat_word(i)) {
                printf("%x.%d.%d at word %d at pass %d", dpu_get_rank_id(dpu_get_rank(dpu_from_set(dpu))), dpu_get_slice_id(dpu_from_set(dpu)), dpu_get_member_id(dpu_from_set(dpu)), j, i);
                nb_errors++;
                printf("\t%lx != %lx\n", ((uint64_t *)&dpu_test_file[rank_id][each_dpu * 256])[j], concat_word(i));
            }
        }
    }

    return DPU_OK;
}

/**
 * @brief Main of the Host Application.
 */
int main()
{
    struct dpu_set_t dpu_set;
    uint32_t nr_of_dpus, nr_of_ranks;
    bool status = true;
    int i;

    DPU_ASSERT(dpu_alloc(NR_DPUS, NULL, &dpu_set));
    DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY, NULL));

    DPU_ASSERT(dpu_get_nr_dpus(dpu_set, &nr_of_dpus));
    printf("Allocated %d DPU(s)\n", nr_of_dpus);

    DPU_ASSERT(dpu_get_nr_ranks(dpu_set, &nr_of_ranks));

    for (i = 0; i < (int)nr_of_ranks; ++i) {
        dpu_test_file[i] = mmap(0, 64 * 256, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
        if (dpu_test_file[i] == MAP_FAILED) {
            printf("Error allocating dpu_test_file\n");
            exit(-1);
        }
    }

    // Create an "input file" with arbitrary data.
    // Compute its theoretical checksum value.
    for (int i = 0; i < 500000000; ++i) {
        if (i % 1000 == 0)
            printf("Pass %d...%d errors\n", i, nb_errors);
        DPU_ASSERT(dpu_launch(dpu_set, DPU_ASYNCHRONOUS));
        DPU_ASSERT(dpu_callback(dpu_set, callback, (void *)(unsigned long long)i, DPU_CALLBACK_ASYNC));
    }

    DPU_ASSERT(dpu_sync(dpu_set));

    DPU_ASSERT(dpu_free(dpu_set));
    return status ? 0 : -1;
}
