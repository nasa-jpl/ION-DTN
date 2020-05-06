# Makefile for compiling AL-BP and all DTNsuite applications

ifeq ($(or $(strip $(DTN2_DIR)),$(strip $(ION_DIR)),$(strip $(IBRDTN_DIR))),)
# NOTHING
all: help
else
all:
	make -C al_bp $@
	make -C dtnperf $@ AL_BP_DIR=../al_bp
	make -C dtnproxy $@ AL_BP_DIR=../al_bp
#	make -C dtnbox $@ AL_BP_DIR=../al_bp
	make -C dtnfog $@ AL_BP_DIR=../al_bp
endif

install:
	make -C al_bp install
	make -C dtnperf install
	make -C dtnproxy install
#	make -C dtnbox install
	make -C dtnfog install
uninstall:
	make -C al_bp uninstall
	make -C dtnperf uninstall
	make -C dtnproxy uninstall
#	make -C dtnbox uninstall
	make -C dtnfog uninstall
	
clean:
	make -C al_bp clean
	make -C dtnperf clean
	make -C dtnproxy clean
#	make -C dtnbox clean
	make -C dtnfog clean
	
help:
	@echo "Usage (enter absolute paths):"
	@echo "For DTN2:                make DTN2_DIR=<dtn2_dir>"
	@echo "For ION:                 make ION_DIR=<ion_dir>"
	@echo "For IBRDTN:              make IBRDTN_DIR=<ibrdtn_dir>"
	@echo "For both DTN2 & ION:     make DTN2_DIR=<dtn2_dir> ION_DIR=<ion_dir>"
	@echo "For both DTN2 & IBRDTN:  make DTN2_DIR=<dtn2_dir> IBRDTN_DIR=<ibrdtn_dir>"
	@echo "For both ION & IBRDTN:   make ION_DIR=<ion_dir> IBRDTN_DIR=<ibrdtn_dir>"
	@echo "For all three:           make DTN2_DIR=<dtn2_dir> ION_DIR=<ion_dir> IBRDTN_DIR=<ibrdtn_dir>"
	@echo "To compile with debug symbols add DEBUG=1"
	@echo "To force BPv6 compilation in ION 4.0.0 add BP=bpv6"
