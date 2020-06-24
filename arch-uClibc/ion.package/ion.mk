#############################################################
#
# ion
#
#############################################################

#ION_VERSION_MAJOR=3
#ION_VERSION_MINOR=7.1
#ION_VERSION:=$(ION_VERSION_MAJOR).$(ION_VERSION_MINOR)
ION_VERSION:=open-source
ION_SOURCE:=ion-$(ION_VERSION).tar.gz
#ION_SITE:=http://sourceforge.net/projects/ion-dtn/files

define ION_BUILD_CMDS
 mkdir -p $(TARGET_DIR)/opt
 mkdir -p $(TARGET_DIR)/opt/bin
 mkdir -p $(TARGET_DIR)/opt/lib
 mkdir -p $(TARGET_DIR)/opt/include
 mkdir -p $(TARGET_DIR)/opt/man
 mkdir -p $(TARGET_DIR)/opt/man/man1
 mkdir -p $(TARGET_DIR)/opt/man/man3
 mkdir -p $(TARGET_DIR)/opt/man/man5

 $(MAKE) TCC="$(TARGET_CC)" TLD="$(TARGET_CC)" ROOT="$(TARGET_DIR)/opt" -C $(@D)/ici/arm-uClibc all
 $(MAKE) TCC="$(TARGET_CC)" TLD="$(TARGET_CC)" ROOT="$(TARGET_DIR)/opt" -C $(@D)/ici/arm-uClibc install
 $(MAKE) TCC="$(TARGET_CC)" TLD="$(TARGET_CC)" ROOT="$(TARGET_DIR)/opt" -C $(@D)/dgr/arm-uClibc all
 $(MAKE) TCC="$(TARGET_CC)" TLD="$(TARGET_CC)" ROOT="$(TARGET_DIR)/opt" -C $(@D)/dgr/arm-uClibc install
 $(MAKE) TCC="$(TARGET_CC)" TLD="$(TARGET_CC)" ROOT="$(TARGET_DIR)/opt" -C $(@D)/ltp/arm-uClibc all
 $(MAKE) TCC="$(TARGET_CC)" TLD="$(TARGET_CC)" ROOT="$(TARGET_DIR)/opt" -C $(@D)/ltp/arm-uClibc install
 $(MAKE) TCC="$(TARGET_CC)" TLD="$(TARGET_CC)" ROOT="$(TARGET_DIR)/opt" -C $(@D)/bssp/arm-uClibc all
 $(MAKE) TCC="$(TARGET_CC)" TLD="$(TARGET_CC)" ROOT="$(TARGET_DIR)/opt" -C $(@D)/bssp/arm-uClibc install
 $(MAKE) TCC="$(TARGET_CC)" TLD="$(TARGET_CC)" ROOT="$(TARGET_DIR)/opt" -C $(@D)/$(BP)/arm-uClibc all
 $(MAKE) TCC="$(TARGET_CC)" TLD="$(TARGET_CC)" ROOT="$(TARGET_DIR)/opt" -C $(@D)/$(BP)/arm-uClibc install
 $(MAKE) TCC="$(TARGET_CC)" TLD="$(TARGET_CC)" ROOT="$(TARGET_DIR)/opt" -C $(@D)/cfdp/arm-uClibc all
 $(MAKE) TCC="$(TARGET_CC)" TLD="$(TARGET_CC)" ROOT="$(TARGET_DIR)/opt" -C $(@D)/cfdp/arm-uClibc install
 $(MAKE) TCC="$(TARGET_CC)" TLD="$(TARGET_CC)" ROOT="$(TARGET_DIR)/opt" -C $(@D)/ams/arm-uClibc all
 $(MAKE) TCC="$(TARGET_CC)" TLD="$(TARGET_CC)" ROOT="$(TARGET_DIR)/opt" -C $(@D)/ams/arm-uClibc install
 $(MAKE) TCC="$(TARGET_CC)" TLD="$(TARGET_CC)" ROOT="$(TARGET_DIR)/opt" -C $(@D)/bss/arm-uClibc all
 $(MAKE) TCC="$(TARGET_CC)" TLD="$(TARGET_CC)" ROOT="$(TARGET_DIR)/opt" -C $(@D)/bss/arm-uClibc install
endef

define ION_INSTALL_TARGET_CMDS
 $(MAKE) TCC="$(TARGET_CC)" TLD="$(TARGET_CC)" ROOT="$(TARGET_DIR)/opt" -C $(@D)/ici/arm-uClibc install
 $(MAKE) TCC="$(TARGET_CC)" TLD="$(TARGET_CC)" ROOT="$(TARGET_DIR)/opt" -C $(@D)/dgr/arm-uClibc install
 $(MAKE) TCC="$(TARGET_CC)" TLD="$(TARGET_CC)" ROOT="$(TARGET_DIR)/opt" -C $(@D)/ltp/arm-uClibc install
 $(MAKE) TCC="$(TARGET_CC)" TLD="$(TARGET_CC)" ROOT="$(TARGET_DIR)/opt" -C $(@D)/bssp/arm-uClibc install
 $(MAKE) TCC="$(TARGET_CC)" TLD="$(TARGET_CC)" ROOT="$(TARGET_DIR)/opt" -C $(@D)/$(BP)/arm-uClibc install
 $(MAKE) TCC="$(TARGET_CC)" TLD="$(TARGET_CC)" ROOT="$(TARGET_DIR)/opt" -C $(@D)/cfdp/arm-uClibc install
 $(MAKE) TCC="$(TARGET_CC)" TLD="$(TARGET_CC)" ROOT="$(TARGET_DIR)/opt" -C $(@D)/ams/arm-uClibc install
 $(MAKE) TCC="$(TARGET_CC)" TLD="$(TARGET_CC)" ROOT="$(TARGET_DIR)/opt" -C $(@D)/bss/arm-uClibc install
endef

define ION_CLEAN_CMDS
 $(MAKE) TCC="$(TARGET_CC)" TLD="$(TARGET_CC)" ROOT="$(TARGET_DIR)/opt" -C $(@D)/ici/arm-uClibc clean
 $(MAKE) TCC="$(TARGET_CC)" TLD="$(TARGET_CC)" ROOT="$(TARGET_DIR)/opt" -C $(@D)/dgr/arm-uClibc clean
 $(MAKE) TCC="$(TARGET_CC)" TLD="$(TARGET_CC)" ROOT="$(TARGET_DIR)/opt" -C $(@D)/ltp/arm-uClibc clean
 $(MAKE) TCC="$(TARGET_CC)" TLD="$(TARGET_CC)" ROOT="$(TARGET_DIR)/opt" -C $(@D)/bssp/arm-uClibc clean
 $(MAKE) TCC="$(TARGET_CC)" TLD="$(TARGET_CC)" ROOT="$(TARGET_DIR)/opt" -C $(@D)/$(BP)/arm-uClibc clean
 $(MAKE) TCC="$(TARGET_CC)" TLD="$(TARGET_CC)" ROOT="$(TARGET_DIR)/opt" -C $(@D)/cfdp/arm-uClibc clean
 $(MAKE) TCC="$(TARGET_CC)" TLD="$(TARGET_CC)" ROOT="$(TARGET_DIR)/opt" -C $(@D)/ams/arm-uClibc clean
 $(MAKE) TCC="$(TARGET_CC)" TLD="$(TARGET_CC)" ROOT="$(TARGET_DIR)/opt" -C $(@D)/bss/arm-uClibc clean
endef

$(eval $(call GENTARGETS,package,ion))
