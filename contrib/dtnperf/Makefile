# Makefile for compiling libbp_abstraction_layer.a and DTNPerf3

ifeq ($(or $(ION_DIR),$(DTN2_DIR)),)
# NOTHING
all: help
endif 

all:
	make -C al_bp $@ all
	make -C dtnperf $@ AL_BP_DIR=../al_bp

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
	@echo "For DTN2: 	make DTN2_DIR=<dtn2_dir>"
	@echo "For ION:	make ION_DIR=<ion_dir> "
	@echo "For both: 	make DTN2_DIR=<dtn2_dir> ION_DIR=<ion_dir>"
	@echo "To compile with debug symbols add DEBUG=1"

