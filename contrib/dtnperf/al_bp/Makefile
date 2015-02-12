# Makefile for compiling libbp_abstraction_layer.a

LIB=static
#LIB=dynamic
LIB_NAME_BASE=libal_bp
CC=gcc
DIR_BP_IMPL=./src/bp_implementations/
SUFFIX=
DEBUG=0
ifeq ($(DEBUG),0)
DEBUG_FLAG=
else
DEBUG_FLAG=-g -fno-inline
endif
CFLAGS= $(DEBUG_FLAG) -Wall -fPIC -Werror -O2

ifeq ($(or $(ION_DIR),$(DTN2_DIR)),)
# NOTHING
all: help
else 
all: lib
endif 

LIB_NAME=$(LIB_NAME_BASE)

ifeq ($(strip $(DTN2_DIR)),)
ifneq ($(strip $(ION_DIR)),)
# ION
LIB_NAME=$(LIB_NAME_BASE)_vION
INC=-I$(ION_DIR) -I$(ION_DIR)/bp/include -I$(ION_DIR)/bp/library -I$(ION_DIR)/ici/include
OPT=-DION_IMPLEMENTATION $(CFLAGS)
endif
else ifeq ($(strip $(ION_DIR)),)
ifneq ($(strip $(DTN2_DIR)),)
# DTN2
LIB_NAME=$(LIB_NAME_BASE)_vDTN2
INC=-I$(DTN2_DIR) -I$(DTN2_DIR)/applib/ -I$(DTN2_DIR)/oasys/include
OPT=-DDTN2_IMPLEMENTATION $(CFLAGS)
endif
else ifneq ($(strip $(ION_DIR)),)
ifneq ($(strip $(DTN2_DIR)),)
# BOTH
LIB_NAME=$(LIB_NAME_BASE)
INC=-I$(DTN2_DIR) -I$(DTN2_DIR)/applib/ -I$(DTN2_DIR)/oasys/include -I$(ION_DIR)/bp/include -I$(ION_DIR)/bp/library -I$(ION_DIR)/ici/include
OPT=-DION_IMPLEMENTATION -DDTN2_IMPLEMENTATION $(CFLAGS)
endif
#else ifeq ($(and $(strip $(DTN2_DIR)), $(strip $(ION_DIR))),)
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

help:
	@echo "Usage:"
	@echo "For DTN2:	make DTN2_DIR=<dtn2_dir>"
	@echo "For ION:		make ION_DIR=<ion_dir> ION_VERS_UP_3.3.0=<yes|no>"
	@echo "For both:	make DTN2_DIR=<dtn2_dir> ION_DIR=<ion_dir> ION_VERS_UP_3.3.0=<yes|no>"
	@echo "To compile with debug symbols add DEBUG=1"

clean:
	@rm -rf *.o *.so *.a
