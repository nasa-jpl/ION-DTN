# Makefile for compiling DTNPerf3

# NAME OF BIN USED FOR INSTALL/UNINSTALL (NEVER LEAVE IT BLANK!!!)
BIN_NAME_BASE=dtnperf
CC=gcc
LIB_PATHS=-L/usr/local/lib -L$(AL_BP_DIR)
INSTALL_PATH=/usr/local/bin
DEBUG=0
ifeq ($(DEBUG),0)
DEBUG_FLAG=-O2
else
DEBUG_FLAG=-g -fno-inline -O0
endif
CFLAGS= $(DEBUG_FLAG) -Wall -fmessage-length=0 -Werror

INSTALLED=$(wildcard $(INSTALL_PATH)/$(BIN_NAME_BASE)*)

ifeq ($(strip $(AL_BP_DIR)),)
all: help
else ifeq ($(or $(strip $(DTN2_DIR)),$(strip $(ION_DIR)),$(strip $(IBRDTN_DIR))),)
# NOTHING
all: help
else
all: bin
endif

ifeq ($(or $(strip $(ION_DIR)),$(strip $(IBRDTN_DIR))),)
ifneq ($(strip $(DTN2_DIR)),)
# DTN2
INC=-I$(AL_BP_DIR)/src/bp_implementations -I$(AL_BP_DIR)/src -I$(DTN2_DIR) -I$(DTN2_DIR)/applib
LIBS=$(AL_BP_DIR)/libal_bp_vDTN2.a -ldtnapi -lpthread
BIN_NAME=$(BIN_NAME_BASE)_vDTN2
endif
else ifeq ($(or $(strip $(DTN2_DIR)),$(strip $(IBRDTN_DIR))),)
ifneq ($(strip $(ION_DIR)),)
# ION
INC=-I$(AL_BP_DIR)/src/bp_implementations -I$(AL_BP_DIR)/src 
LIBS=$(AL_BP_DIR)/libal_bp_vION.a -lbp -lici -limcfw -ldtn2fw -lipnfw -lpthread
#LIBS=$(AL_BP_DIR)/libal_bp_vION.a -lbp -lici -lpthread
BIN_NAME=$(BIN_NAME_BASE)_vION
endif
else ifeq ($(or $(strip $(DTN2_DIR)),$(strip $(ION_DIR))),)
ifneq ($(strip $(IBRDTN_DIR)),)
# IBRDTN
IBRDTN_VERSION=1.0.1
INC=-I$(AL_BP_DIR)/src/bp_implementations -I$(AL_BP_DIR)/src -I$(IBRDTN_DIR)/ibrcommon-$(IBRDTN_VERSION) -I$(IBRDTN_DIR)/ibrdtn-$(IBRDTN_VERSION)
LIBS=$(AL_BP_DIR)/libal_bp_vIBRDTN.a -librcommon -librdtn -lz -lpthread
BIN_NAME=$(BIN_NAME_BASE)_vIBRDTN
endif
else ifeq ($(strip $(IBRDTN_DIR)),)
ifneq ($(and $(strip $(DTN2_DIR)),$(strip $(ION_DIR))),)
# DTN2 & ION
INC=-I$(AL_BP_DIR)/src/bp_implementations -I$(AL_BP_DIR)/src -I$(DTN2_DIR) -I$(DTN2_DIR)/applib
LIBS=$(AL_BP_DIR)/libal_bp_vDTN2+ION.a -ldtnapi -lbp -lici -limcfw -ldtn2fw -lipnfw -lpthread
#LIBS=$(AL_BP_DIR)/libal_bp_vDTN2+ION.a -ldtnapi -lbp -lici -lpthread 
BIN_NAME=$(BIN_NAME_BASE)_vDTN2+ION
endif
else ifeq ($(strip $(ION_DIR)),)
ifneq ($(and $(strip $(DTN2_DIR)),$(strip $(IBRDTN_DIR))),)
# DTN2 & IBRDTN
IBRDTN_VERSION=1.0.1
INC=-I$(AL_BP_DIR)/src/bp_implementations -I$(AL_BP_DIR)/src -I$(DTN2_DIR) -I$(DTN2_DIR)/applib
LIBS=$(AL_BP_DIR)/libal_bp_vDTN2+IBRDTN.a -ldtnapi -librcommon -librdtn -lz -lpthread
BIN_NAME=$(BIN_NAME_BASE)_vDTN2+IBRDTN
endif
else ifeq ($(strip $(DTN2_DIR)),)
ifneq ($(and $(strip $(ION_DIR)),$(strip $(IBRDTN_DIR))),)
# ION & IBRDTN
IBRDTN_VERSION=1.0.1
INC=-I$(AL_BP_DIR)/src/bp_implementations -I$(AL_BP_DIR)/src -I$(IBRDTN_DIR)/ibrcommon-$(IBRDTN_VERSION) -I$(IBRDTN_DIR)/ibrdtn-$(IBRDTN_VERSION)
LIBS=$(AL_BP_DIR)/libal_bp_vION+IBRDTN.a -lbp -lici -limcfw -ldtn2fw -lipnfw -librcommon -librdtn -lz -lpthread
#LIBS=$(AL_BP_DIR)/libal_bp_vION+IBRDTN.a -lbp -lici -librcommon -librdtn -lz -lpthread 
BIN_NAME=$(BIN_NAME_BASE)_vION+IBRDTN
endif
else ifneq ($(and $(strip $(DTN2_DIR)),$(strip $(ION_DIR)),$(strip $(IBRDTN_DIR))),)
# ALL THREE
IBRDTN_VERSION=1.0.1
INC=-I$(AL_BP_DIR)/src/bp_implementations -I$(AL_BP_DIR)/src -I$(DTN2_DIR) -I$(DTN2_DIR)/applib -I$(IBRDTN_DIR)/ibrcommon-$(IBRDTN_VERSION) -I$(IBRDTN_DIR)/ibrdtn-$(IBRDTN_VERSION)
LIBS=$(AL_BP_DIR)/libal_bp.a -ldtnapi -lbp -lici -limcfw -ldtn2fw -lipnfw -librcommon -librdtn -lz -lpthread
#LIBS=$(AL_BP_DIR)/libal_bp.a -ldtnapi -lbp -lici -librcommon -librdtn -lz -lpthread 
BIN_NAME=$(BIN_NAME_BASE)
endif

bin:
	$(CC) $(INC) $(CFLAGS) -c src/*.c src/dtnperf_modes/*.c
	g++ $(LIB_PATHS) -o $(BIN_NAME) *.o $(LIBS)

install:
	cp $(BIN_NAME_BASE)* $(INSTALL_PATH)/

uninstall:
	@if test `echo $(INSTALLED) | wc -w` -eq 1 -a -f "$(INSTALLED)"; then rm -rf $(INSTALLED); else echo "MORE THAN 1 FILE, DELETE THEM MANUALLY: $(INSTALLED)"; fi

help:
	@echo "Usage:"
	@echo "For DTN2:                make DTN2_DIR=<dtn2_dir> AL_BP_DIR=<al_bp_dir>"
	@echo "For ION:                 make ION_DIR=<ion_dir> AL_BP_DIR=<al_bp_dir>"
	@echo "For IBRDTN:              make IBRDTN_DIR=<ibrdtn_dir> AL_BP_DIR=<al_bp_dir>"
	@echo "For both DTN2 & ION:     make DTN2_DIR=<dtn2_dir> ION_DIR=<ion_dir> AL_BP_DIR=<al_bp_dir>"
	@echo "For both DTN2 & IBRDTN:  make DTN2_DIR=<dtn2_dir> IBRDTN_DIR=<ibrdtn_dir> AL_BP_DIR=<al_bp_dir>"
	@echo "For both ION & IBRDTN:   make ION_DIR=<ion_dir> IBRDTN_DIR=<ibrdtn_dir> AL_BP_DIR=<al_bp_dir>"
	@echo "For all three:           make DTN2_DIR=<dtn2_dir> ION_DIR=<ion_dir> IBRDTN_DIR=<ibrdtn_dir> AL_BP_DIR=<al_bp_dir>"
	@echo "To compile with debug symbols add DEBUG=1"

clean:
	@rm -rf *.o $(BIN_NAME_BASE)*
