# Makefile to compile the JALBP
LIB_NAME_BASE=libal_bp
CC=gcc
DIR_BP_IMPL=./src/bp_implementations/
SUFFIX=
DEBUG=0
ifeq ($(DEBUG),0)
DEBUG_FLAG=-O2
else
DEBUG_FLAG=-g -fno-inline -O0
endif
CFLAGS= $(DEBUG_FLAG) -Wall -fPIC -Werror
JAVA_DIR=/usr/lib/jvm/default-java/include/

ifeq ($(strip $(AL_BP_DIR)),)
all: help
else ifeq ($(or $(strip $(DTN2_DIR)),$(strip $(ION_DIR)),$(strip $(IBRDTN_DIR))),)
# NOTHING
all: help
else
all: lib
endif

AL_BP_A_FILE_EXTENSION=
LIB_NAME=$(LIB_NAME_BASE)

ifeq ($(or $(strip $(ION_DIR)),$(strip $(IBRDTN_DIR))),)
ifneq ($(strip $(DTN2_DIR)),)
# DTN2
INC=-I$(AL_BP_DIR)/src/bp_implementations -I$(AL_BP_DIR)/src -I$(DTN2_DIR) -I$(DTN2_DIR)/applib
LIBS=$(AL_BP_DIR)/libal_bp_vDTN2.a -ldtnapi -lpthread
AL_BP_A_FILE_EXTENSION=_vDTN2
endif
else ifeq ($(or $(strip $(DTN2_DIR)),$(strip $(IBRDTN_DIR))),)
ifneq ($(strip $(ION_DIR)),)
# ION
INC=-I$(AL_BP_DIR)/src/bp_implementations -I$(AL_BP_DIR)/src -I$(ION_DIR)/include -I$(ION_DIR)/library
LIBS=$(AL_BP_DIR)/libal_bp_vION.a -lbp -lici -limcfw -ldtn2fw -lipnfw -lpthread
#LIBS=$(AL_BP_DIR)/libal_bp_vION.a -lbp -lici -lpthread
AL_BP_A_FILE_EXTENSION=_vION
endif
else ifeq ($(or $(strip $(DTN2_DIR)),$(strip $(ION_DIR))),)
ifneq ($(strip $(IBRDTN_DIR)),)
# IBRDTN
IBRDTN_VERSION=1.0.1
INC=-I$(AL_BP_DIR)/src/bp_implementations -I$(AL_BP_DIR)/src -I$(IBRDTN_DIR)/ibrcommon-$(IBRDTN_VERSION) -I$(IBRDTN_DIR)/ibrdtn-$(IBRDTN_VERSION)
LIBS=$(AL_BP_DIR)/libal_bp_vIBRDTN.a -librcommon -librdtn -lz -lpthread
AL_BP_A_FILE_EXTENSION=_vIBRDTN
endif
else ifeq ($(strip $(IBRDTN_DIR)),)
ifneq ($(and $(strip $(DTN2_DIR)),$(strip $(ION_DIR))),)
# DTN2 & ION
INC=-I$(AL_BP_DIR)/src/bp_implementations -I$(AL_BP_DIR)/src -I$(DTN2_DIR) -I$(DTN2_DIR)/applib -I$(ION_DIR)/include -I$(ION_DIR)/library
LIBS=$(AL_BP_DIR)/libal_bp_vDTN2+ION.a -ldtnapi -lbp -lici -limcfw -ldtn2fw -lipnfw -lpthread
#LIBS=$(AL_BP_DIR)/libal_bp_vDTN2+ION.a -ldtnapi -lbp -lici -lpthread
AL_BP_A_FILE_EXTENSION=_vDTN2+ION
endif
else ifeq ($(strip $(ION_DIR)),)
ifneq ($(and $(strip $(DTN2_DIR)),$(strip $(IBRDTN_DIR))),)
# DTN2 & IBRDTN
IBRDTN_VERSION=1.0.1
INC=-I$(AL_BP_DIR)/src/bp_implementations -I$(AL_BP_DIR)/src -I$(DTN2_DIR) -I$(DTN2_DIR)/applib
LIBS=$(AL_BP_DIR)/libal_bp_vDTN2+IBRDTN.a -ldtnapi -librcommon -librdtn -lz -lpthread
AL_BP_A_FILE_EXTENSION=_vDTN2+IBRDTN
endif
else ifeq ($(strip $(DTN2_DIR)),)
ifneq ($(and $(strip $(ION_DIR)),$(strip $(IBRDTN_DIR))),)
# ION & IBRDTN
IBRDTN_VERSION=1.0.1
INC=-I$(AL_BP_DIR)/src/bp_implementations -I$(AL_BP_DIR)/src -I$(ION_DIR)/include -I$(ION_DIR)/library -I$(IBRDTN_DIR)/ibrcommon-$(IBRDTN_VERSION) -I$(IBRDTN_DIR)/ibrdtn-$(IBRDTN_VERSION)
LIBS=$(AL_BP_DIR)/libal_bp_vION+IBRDTN.a -lbp -lici -limcfw -ldtn2fw -lipnfw -librcommon -librdtn -lz -lpthread
#LIBS=$(AL_BP_DIR)/libal_bp_vION+IBRDTN.a -lbp -lici -librcommon -librdtn -lz -lpthread
AL_BP_A_FILE_EXTENSION=_vION+IBRDTN
endif
else ifneq ($(and $(strip $(DTN2_DIR)),$(strip $(ION_DIR)),$(strip $(IBRDTN_DIR))),)
# ALL THREE
IBRDTN_VERSION=1.0.1
INC=-I$(AL_BP_DIR)/src/bp_implementations -I$(AL_BP_DIR)/src -I$(DTN2_DIR) -I$(DTN2_DIR)/applib -I$(ION_DIR)/include -I$(ION_DIR)/library -I$(IBRDTN_DIR)/ibrcommon-$(IBRDTN_VERSION) -I$(IBRDTN_DIR)/ibrdtn-$(IBRDTN_VERSION)
LIBS=$(AL_BP_DIR)/libal_bp.a -ldtnapi -lbp -lici -limcfw -ldtn2fw -lipnfw -librcommon -librdtn -lz -lpthread
#LIBS=$(AL_BP_DIR)/libal_bp.a -ldtnapi -lbp -lici -librcommon -librdtn -lz -lpthread
AL_BP_A_FILE_EXTENSION=
endif

lib: objs
	$(CC) -shared -Wl,-soname,$(LIB_NAME).so -o $(LIB_NAME).so *.o $(LIBS) $(AL_BP_DIR)/$(LIB_NAME)$(AL_BP_A_FILE_EXTENSION).a

install:
	cp $(LIB_NAME).so /usr/lib/

uninstall:
	@rm -rf /usr/lib/$(LIB_NAME).so

objs:
	$(CC) $(INC) -I$(JAVA_DIR) -I$(JAVA_DIR)/linux/ $(CFLAGS) -c *.c

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
	@echo "To override the Java library path add JAVA_DIR=<java_library_dir>"

clean:
#@rm -rf /usr/lib/$(LIB_NAME).so
	@rm -rf *.o *.so *.a


