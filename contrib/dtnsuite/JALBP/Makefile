# Makefile for compiling libbp_abstraction_layer.a and DTNPerf3

ifeq ($(or $(strip $(DTN2_DIR)),$(strip $(ION_DIR)),$(strip $(IBRDTN_DIR))),)
# NOTHING
all: help
else
all:
	make -C JNIWrapper $@
endif

install:
	make -C JNIWrapper install

uninstall:
	make -C JNIWrapper uninstall
	
clean:
	make -C JNIWrapper clean
	
help:
	make -s -C JNIWrapper help

