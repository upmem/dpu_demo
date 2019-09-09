TARGET_TYPE ?= FUNCTIONAL_SIMULATOR

all:
	cd checksum ; make DPU_TARGET_TYPE=$(TARGET_TYPE) all

clean:
	cd checksum ; make cleaner

test:
	cd checksum ; make DPU_TARGET_TYPE=$(TARGET_TYPE) test

