DPU_DIR := dpu
HOST_DIR := host
BUILDDIR ?= build

HOST_TARGET := ${BUILDDIR}/checksum_host
DPU_TARGET := ${BUILDDIR}/checksum_dpu

COMMON_INCLUDES := common
HOST_SOURCES := $(wildcard ${HOST_DIR}/*.c)
DPU_SOURCES := $(wildcard ${DPU_DIR}/*.c)

.PHONY: all clean test

__dirs := $(shell mkdir -p ${BUILDDIR})

COMMON_FLAGS := -Wall -Wextra -Werror -g -I${COMMON_INCLUDES}
HOST_FLAGS := ${COMMON_FLAGS} -std=c11 -O3 `dpu-pkg-config --cflags --libs dpu`
DPU_FLAGS := ${COMMON_FLAGS} -O2

all: ${HOST_TARGET} ${DPU_TARGET}

${HOST_TARGET}: ${HOST_SOURCES} ${COMMON_INCLUDES}
	$(CC) -o $@ ${HOST_SOURCES} ${HOST_FLAGS}

${DPU_TARGET}: ${DPU_SOURCES} ${COMMON_INCLUDES}
	dpu-upmem-dpurte-clang -o $@ ${DPU_SOURCES} ${DPU_FLAGS}

clean:
	$(RM) -rf $(BUILDDIR)

test: all
	./${HOST_TARGET}

