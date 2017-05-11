DPU_DIR := dpu
HOST_DIR := host
BUILDDIR := obj
TARGETDIR := bin
LOGDIR := log

ifndef $(DPU_TARGET_TYPE)
DPU_TARGET_TYPE=FUNCTIONAL_SIMULATOR
endif

.PHONY: all remake dpu host clean clean_dpu clean_host
.DEFAULT_GOAL := all

all: dpu host

remake: clean all

dpu:
	$(MAKE) -C $(DPU_DIR)
host:
	$(MAKE) -C $(HOST_DIR) DPU_TARGET_TYPE=$(DPU_TARGET_TYPE)

clean:
	@$(RM) -rf $(BUILDDIR)

cleaner: clean
	@$(RM) -rf $(TARGETDIR)
	@$(RM) -rf $(LOGDIR)

clean_dpu:
	$(MAKE) -C $(DPU_DIR) cleaner

clean_host:
	$(MAKE) -C $(HOST_DIR) cleaner

test: all
	./bin/host/program

