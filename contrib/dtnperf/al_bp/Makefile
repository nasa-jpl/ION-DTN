# Makefile for compiling libbp_abstraction_layer.a

LIB=static
#LIB=dynamic
LIB_NAME_BASE=libal_bp
CC=gcc
DIR_BP_IMPL=./src/bp_implementations/
SUFFIX=
DEBUG=0
ifeq ($(DEBUG),0)
DEBUG_FLAG= -O2
else
DEBUG_FLAG=-g -fno-inline -O0
endif
CFLAGS= $(DEBUG_FLAG) -Wall -fPIC -Werror

ifeq ($(or $(strip $(DTN2_DIR)),$(strip $(ION_DIR)),$(strip $(IBRDTN_DIR))),)
# NOTHING
all: help
else 
all: lib
endif 

LIB_NAME=$(LIB_NAME_BASE)

ifeq ($(or $(strip $(ION_DIR)),$(strip $(IBRDTN_DIR))),)
ifneq ($(strip $(DTN2_DIR)),)
# DTN2
LIB_NAME=$(LIB_NAME_BASE)_vDTN2
INC=-I$(DTN2_DIR) -I$(DTN2_DIR)/applib/ -I$(DTN2_DIR)/oasys/include
OPT=-DDTN2_IMPLEMENTATION $(CFLAGS)
endif
else ifeq ($(or $(strip $(DTN2_DIR)),$(strip $(IBRDTN_DIR))),)
ifneq ($(strip $(ION_DIR)),)
# ION
ION_VERSION=-DNEW_ZCO #obsolete
LIB_NAME=$(LIB_NAME_BASE)_vION
INC=-I$(ION_DIR) -I$(ION_DIR)/bp/include -I$(ION_DIR)/bp/library -I$(ION_DIR)/ici/include
OPT=-DION_IMPLEMENTATION $(ION_VERSION) $(CFLAGS)
endif
else ifeq ($(or $(strip $(DTN2_DIR)),$(strip $(ION_DIR))),)
ifneq ($(strip $(IBRDTN_DIR)),)
# IBRDTN
IBRDTN_VERSION=1.0.1
LIB_NAME=$(LIB_NAME_BASE)_vIBRDTN
INC=-I$(IBRDTN_DIR)/ibrcommon-$(IBRDTN_VERSION) -I$(IBRDTN_DIR)/ibrdtn-$(IBRDTN_VERSION)
OPT=-DIBRDTN_IMPLEMENTATION $(CFLAGS)
endif
else ifeq ($(strip $(IBRDTN_DIR)),)
ifneq ($(and $(strip $(DTN2_DIR)),$(strip $(ION_DIR))),)
# DTN2 & ION
ION_VERSION=-DNEW_ZCO
LIB_NAME=$(LIB_NAME_BASE)_vDTN2+ION
INC=-I$(DTN2_DIR) -I$(DTN2_DIR)/applib/ -I$(DTN2_DIR)/oasys/include -I$(ION_DIR)/bp/include -I$(ION_DIR)/bp/library -I$(ION_DIR)/ici/include
OPT=-DION_IMPLEMENTATION -DDTN2_IMPLEMENTATION $(ION_VERSION) $(CFLAGS)
endif
else ifeq ($(strip $(ION_DIR)),)
ifneq ($(and $(strip $(DTN2_DIR)),$(strip $(IBRDTN_DIR))),)
# DTN2 & IBRDTN
IBRDTN_VERSION=1.0.1
LIB_NAME=$(LIB_NAME_BASE)_vDTN2+IBRDTN
INC=-I$(DTN2_DIR) -I$(DTN2_DIR)/applib/ -I$(DTN2_DIR)/oasys/include -I$(IBRDTN_DIR)/ibrcommon-$(IBRDTN_VERSION) -I$(IBRDTN_DIR)/ibrdtn-$(IBRDTN_VERSION)
OPT=-DDTN2_IMPLEMENTATION -DIBRDTN_IMPLEMENTATION $(CFLAGS)
endif
else ifeq ($(strip $(DTN2_DIR)),)
ifneq ($(and $(strip $(ION_DIR)),$(strip $(IBRDTN_DIR))),)
# ION & IBRDTN
ION_VERSION=-DNEW_ZCO
IBRDTN_VERSION=1.0.1
LIB_NAME=$(LIB_NAME_BASE)_vION+IBRDTN
INC=-I$(ION_DIR) -I$(ION_DIR)/bp/include -I$(ION_DIR)/bp/library -I$(ION_DIR)/ici/include -I$(IBRDTN_DIR)/ibrcommon-$(IBRDTN_VERSION) -I$(IBRDTN_DIR)/ibrdtn-$(IBRDTN_VERSION)
OPT=-DION_IMPLEMENTATION -DIBRDTN_IMPLEMENTATION $(ION_VERSION) $(CFLAGS)
endif
else ifneq ($(and $(strip $(DTN2_DIR)),$(strip $(ION_DIR)),$(strip $(IBRDTN_DIR))),)
# ALL THREE
ION_VERSION=-DNEW_ZCO
IBRDTN_VERSION=1.0.1
LIB_NAME=$(LIB_NAME_BASE)
INC=-I$(DTN2_DIR) -I$(DTN2_DIR)/applib/ -I$(DTN2_DIR)/oasys/include -I$(ION_DIR)/bp/include -I$(ION_DIR)/bp/library -I$(ION_DIR)/ici/include -I$(IBRDTN_DIR)/ibrcommon-$(IBRDTN_VERSION) -I$(IBRDTN_DIR)/ibrdtn-$(IBRDTN_VERSION)
OPT=-DION_IMPLEMENTATION -DDTN2_IMPLEMENTATION -DIBRDTN_IMPLEMENTATION $(ION_VERSION) $(CFLAGS)
endif

ifeq ($(strip $(LIB)),static)
LIB_CC=ar crs $(LIB_NAME).a
else
LIB_CC=$(CC) -shared -Wl,-soname,$(LIB_NAME).so -o $(LIB_NAME).so
endif

INSTALLED=$(wildcard /usr/lib/$(LIB_NAME)*)

lib: objs
	$(LIB_CC) *.o
	
install: 
	cp $(LIB_NAME)* /usr/lib/

uninstall:
	@if test `echo $(INSTALLED) | wc -w` -eq 1 -a -f "$(INSTALLED)"; then rm -rf $(INSTALLED); else if test -n "$(INSTALLED)"; then echo "MORE THAN 1 FILE, DELETE THEM MANUALLY: $(INSTALLED)"; else echo "NOT INSTALLED"; fi fi

objs:
	$(CC) -I$(DIR_BP_IMPL) $(INC) $(OPT) -c src/*.c
	$(CC) $(INC) $(OPT) -c src/bp_implementations/*.c
	g++ $(INC) $(OPT) -c src/bp_implementations/*.cpp

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

clean:
	@rm -rf *.o *.so *.a
