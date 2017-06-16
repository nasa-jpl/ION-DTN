# Makefile for compiling libbp_abstraction_layer.a and DTNPerf3

ifeq ($(or $(strip $(DTN2_DIR)),$(strip $(ION_DIR)),$(strip $(IBRDTN_DIR))),)
# NOTHING
all: help
else
all:
	make -C al_bp $@
	make -C dtnperf $@ AL_BP_DIR=../al_bp
endif

install:
	make -C al_bp install
	make -C dtnperf install

uninstall:
	make -C al_bp uninstall
	make -C dtnperf uninstall
	
clean:
	make -C al_bp clean
	make -C dtnperf clean
	
help:
	@echo "Usage:"
	@echo "For DTN2:                make DTN2_DIR=<dtn2_dir>"
	@echo "For ION:                 make ION_DIR=<ion_dir>"
	@echo "For IBRDTN:              make IBRDTN_DIR=<ibrdtn_dir>"
	@echo "For both DTN2 & ION:     make DTN2_DIR=<dtn2_dir> ION_DIR=<ion_dir>"
	@echo "For both DTN2 & IBRDTN:  make DTN2_DIR=<dtn2_dir> IBRDTN_DIR=<ibrdtn_dir>"
	@echo "For both ION & IBRDTN:   make ION_DIR=<ion_dir> IBRDTN_DIR=<ibrdtn_dir>"
	@echo "For all three:           make DTN2_DIR=<dtn2_dir> ION_DIR=<ion_dir> IBRDTN_DIR=<ibrdtn_dir>"
	@echo "To compile with debug symbols add DEBUG=1"
