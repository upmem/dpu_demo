DPU_DIR := dpu
HOST_DIR := host
BUILDDIR := obj
TARGETDIR := bin

.PHONY: all remake dpu host clean clean_dpu clean_host
.DEFAULT_GOAL := all

all: dpu host

remake: clean all

dpu:
	$(MAKE) -C $(DPU_DIR)
host:
	$(MAKE) -C $(HOST_DIR)

clean:
	@$(RM) -rf $(BUILDDIR)

cleaner: clean
	@$(RM) -rf $(TARGETDIR)

clean_dpu:
	$(MAKE) -C $(DPU_DIR) cleaner

clean_host:
	$(MAKE) -C $(HOST_DIR) cleaner

test: all
	./bin/host/program

