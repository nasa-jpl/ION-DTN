# Copyright (C) 2009 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := iondtn

MY_ICI		:= $(LOCAL_PATH)/../../../ici

MY_ICISOURCES := \
	$(MY_ICI)/library/platform.c    \
	$(MY_ICI)/library/platform_sm.c \
	$(MY_ICI)/library/memmgr.c      \
	$(MY_ICI)/library/llcv.c        \
	$(MY_ICI)/library/lyst.c        \
	$(MY_ICI)/library/psm.c         \
	$(MY_ICI)/library/smlist.c      \
	$(MY_ICI)/library/smrbt.c       \
	$(MY_ICI)/library/ion.c         \
	$(MY_ICI)/library/ionsec.c      \
	$(MY_ICI)/library/rfx.c         \
	$(MY_ICI)/library/zco.c         \
	$(MY_ICI)/crypto/NULL_SUITES/crypto.c	\
	$(MY_ICI)/crypto/NULL_SUITES/csi.c	\
	$(MY_ICI)/bulk/STUB_BULK/bulk.c	\
	$(MY_ICI)/libbloom-master/bloom.c	\
	$(MY_ICI)/libbloom-master/murmur2/MurmurHash2.c	\
	$(MY_ICI)/sdr/sdrtable.c        \
	$(MY_ICI)/sdr/sdrhash.c         \
	$(MY_ICI)/sdr/sdrxn.c           \
	$(MY_ICI)/sdr/sdrmgt.c          \
	$(MY_ICI)/sdr/sdrstring.c       \
	$(MY_ICI)/sdr/sdrlist.c         \
	$(MY_ICI)/sdr/sdrcatlg.c        \
	$(MY_ICI)/daemon/rfxclock.c     \
	$(MY_ICI)/utils/ionadmin.c      \
	$(MY_ICI)/utils/sdrmend.c       \
	$(MY_ICI)/utils/ionsecadmin.c 	\
	$(MY_ICI)/utils/ionwarn.c	\
	$(MY_ICI)/utils/ionunlock.c	\
	$(MY_ICI)/utils/ionxnowner.c	\
	$(MY_ICI)/utils/ionlog.c

#	$(MY_ICI)/utils/ionexit.c      \

#	MY_RESTART	:= $(LOCAL_PATH)/../../../restart

#	MY_RESTARTSOURCE :=     \
#		$(MY_RESTART)/utils/ionrestart.c

MY_DGR		:= $(LOCAL_PATH)/../../../dgr

MY_DGRSOURCES :=     \
	$(MY_DGR)/library/libdgr.c    \

#	MY_LTP		:= $(LOCAL_PATH)/../../../ltp

#	MY_LTPSOURCES :=     \
#		$(MY_LTP)/library/libltp.c    \
#		$(MY_LTP)/library/libltpP.c   \
#		$(MY_LTP)/daemon/ltpclock.c   \
#		$(MY_LTP)/daemon/ltpdeliv.c   \
#		$(MY_LTP)/daemon/ltpdeliv.c   \
#		$(MY_LTP)/daemon/ltpmeter.c   \
#		$(MY_LTP)/udp/udplsi.c        \
#		$(MY_LTP)/udp/udplso.c        \
#		$(MY_LTP)/utils/ltpadmin.c

MY_BSSP		:= $(LOCAL_PATH)/../../../bssp

MY_BSSPSOURCES :=     \
	$(MY_BSSP)/library/libbssp.c    \
	$(MY_BSSP)/library/libbsspP.c   \
	$(MY_BSSP)/daemon/bsspclock.c   \
	$(MY_BSSP)/udp/udpbsi.c         \
	$(MY_BSSP)/udp/udpbso.c         \
	$(MY_BSSP)/tcp/tcpbsi.c         \
	$(MY_BSSP)/tcp/tcpbso.c         \
	$(MY_BSSP)/tcp/libtcpbsa.c      \
	$(MY_BSSP)/utils/bsspadmin.c

MY_BP		:= $(LOCAL_PATH)/../../../bp

MY_BPSOURCES :=      \
	$(MY_BP)/library/libbp.c      \
	$(MY_BP)/library/libbpP.c     \
	$(MY_BP)/library/libbpnm.c    \
	$(MY_BP)/library/bpsec.c      \
	$(MY_BP)/library/saga.c       \
	$(MY_BP)/daemon/bpclock.c     \
	$(MY_BP)/daemon/bptransit.c   \
	$(MY_BP)/daemon/bpclm.c       \
	$(MY_BP)/utils/bpadmin.c      \
	$(MY_BP)/utils/bpsecadmin.c   \
	$(MY_BP)/utils/bpstats.c      \
	$(MY_BP)/utils/bptrace.c      \
	$(MY_BP)/utils/bplist.c       \
	$(MY_BP)/utils/lgagent.c      \
	$(MY_BP)/cgr/libcgr.c         \
	$(MY_BP)/ipnd/beacon.c        \
	$(MY_BP)/ipnd/helper.c        \
	$(MY_BP)/ipnd/ipnd.c          \
	$(MY_BP)/ipnd/node.c          \
	$(MY_BP)/ipn/ipnfw.c          \
	$(MY_BP)/ipn/ipnadmin.c       \
	$(MY_BP)/ipn/libipnfw.c       \
	$(MY_BP)/ipn/ipnadminep.c     \
	$(MY_BP)/dtn2/dtn2admin.c     \
	$(MY_BP)/dtn2/dtn2fw.c        \
	$(MY_BP)/dtn2/dtn2adminep.c   \
	$(MY_BP)/dtn2/libdtn2fw.c     \
	$(MY_BP)/imc/imcadmin.c       \
	$(MY_BP)/imc/imcfw.c          \
	$(MY_BP)/imc/libimcfw.c       \
	$(MY_BP)/udp/udpcli.c         \
	$(MY_BP)/udp/udpclo.c         \
	$(MY_BP)/bibe/bibeclo.c       \
	$(MY_BP)/udp/libudpcla.c      \
	$(MY_BP)/tcp/tcpcli.c         \
	$(MY_BP)/tcp/tcpclo.c         \
	$(MY_BP)/stcp/stcpcli.c       \
	$(MY_BP)/stcp/stcpclo.c       \
	$(MY_BP)/stcp/libstcpcla.c    \
	$(MY_BP)/dgr/dgrcli.c         \
	$(MY_BP)/dgr/dgrclo.c         \
	$(MY_BP)/library/eureka.c     \
	$(MY_BP)/library/bei.c        \
	$(MY_BP)/library/ext/phn/snw.c \
	$(MY_BP)/library/ext/phn/phn.c \
	$(MY_BP)/library/ext/ecos/ecos.c \
	$(MY_BP)/library/ext/meb/meb.c \
	$(MY_BP)/library/ext/bae/bae.c

#	$(MY_BP)/ltp/ltpcli.c         \
#	$(MY_BP)/ltp/ltpclo.c         \

MY_SBSP	:= $(MY_BP)/library/ext/sbsp

MY_SBSPSOURCES :=                    \
	$(MY_SBSP)/sbsp_util.c      \
	$(MY_SBSP)/sbsp_instr.c     \
	$(MY_SBSP)/profiles.c        \
	$(MY_SBSP)/sbsp_bib.c       \
	$(MY_SBSP)/sbsp_bcb.c

MY_NM		:= $(LOCAL_PATH)/../../../nm

MY_NMSOURCES :=                       \
	$(MY_NM)/shared/adm/adm.c \
	$(MY_NM)/shared/adm/adm_agent.c \
	$(MY_NM)/shared/adm/adm_bp.c \
	$(MY_NM)/shared/adm/adm_sbsp.c \
	$(MY_NM)/shared/adm/adm_ion.c \
	$(MY_NM)/shared/adm/adm_ltp.c \
	$(MY_NM)/shared/msg/msg_admin.c \
	$(MY_NM)/shared/msg/msg_ctrl.c \
	$(MY_NM)/shared/msg/pdu.c \
	$(MY_NM)/shared/primitives/var.c \
	$(MY_NM)/shared/primitives/expr.c \
	$(MY_NM)/shared/primitives/blob.c \
	$(MY_NM)/shared/primitives/admin.c \
	$(MY_NM)/shared/primitives/def.c \
	$(MY_NM)/shared/primitives/mid.c \
	$(MY_NM)/shared/primitives/oid.c \
	$(MY_NM)/shared/primitives/report.c \
	$(MY_NM)/shared/primitives/rules.c \
	$(MY_NM)/shared/primitives/dc.c \
	$(MY_NM)/shared/primitives/value.c \
	$(MY_NM)/shared/primitives/lit.c \
	$(MY_NM)/shared/primitives/nn.c \
	$(MY_NM)/shared/primitives/tdc.c \
	$(MY_NM)/shared/primitives/ctrl.c \
	$(MY_NM)/shared/primitives/table.c \
	$(MY_NM)/shared/utils/ion_if.c \
	$(MY_NM)/shared/utils/utils.c \
	$(MY_NM)/shared/utils/db.c \
	$(MY_NM)/shared/utils/nm_types.c \
	$(MY_NM)/agent/ingest.c \
	$(MY_NM)/agent/instr.c \
	$(MY_NM)/agent/lcc.c \
	$(MY_NM)/agent/ldc.c \
	$(MY_NM)/agent/rda.c \
	$(MY_NM)/agent/agent_db.c \
	$(MY_NM)/agent/adm_ion_priv.c \
	$(MY_NM)/agent/adm_ltp_priv.c \
	$(MY_NM)/agent/adm_agent_impl.c \
	$(MY_NM)/agent/adm_bp_impl.c \
	$(MY_NM)/agent/adm_sbsp_impl.c \
	$(MY_NM)/agent/nmagent.c

MY_BSS		:= $(LOCAL_PATH)/../../../bss

MY_BSSSOURCES :=    \
	$(MY_BSS)/library/libbss.c    \
	$(MY_BSS)/library/libbssP.c

#	MY_TEST		:= $(MY_BP)/test

#	MY_TESTSOURCES =     \
#		$(MY_TEST)/bpsource.c        \
#		$(MY_TEST)/bpsink.c

#	MY_CFDP		:= $(LOCAL_PATH)/../../../cfdp

#	MY_CFDPSOURCES :=    \
#		$(MY_CFDP)/library/libcfdp.c    \
#		$(MY_CFDP)/library/libcfdpP.c   \
#		$(MY_CFDP)/library/libcfdpops.c \
#		$(MY_CFDP)/bp/bputa.c           \
#		$(MY_CFDP)/daemon/cfdpclock.c   \
#		$(MY_CFDP)/utils/cfdpadmin.c    \

LOCAL_C_INCLUDES := $(MY_ICI)/include \
	$(MY_ICI)/library \
	$(MY_ICI)/libbloom-master \
	$(MY_ICI)/libbloom-master/murmur2 \
	$(MY_ICI)/crypto \
	$(MY_ICI)/sdr \
	$(MY_DGR)/include \
	$(MY_BP)/include \
	$(MY_BP)/library \
	$(MY_BP)/ipn \
	$(MY_BP)/imc \
	$(MY_BP)/dtn2 \
	$(MY_BP)/ipnd \
	$(MY_BP)/library/ext \
	$(MY_BP)/library/ext/sbsp \
	$(MY_BP)/library/ext/ecos \
	$(MY_BP)/library/ext/meb \
	$(MY_BP)/library/ext/bae \
	$(MY_BP)/library/ext/phn \
	$(MY_BP)/tcp \
	$(MY_BP)/udp \
	$(MY_BSS)/include \
	$(MY_BSS)/library \
	$(MY_BSSP)/include \
	$(MY_BSSP)/library \
	$(MY_BSSP)/udp \
	$(MY_BSSP)/tcp

#	$(MY_LTP)/include $(MY_LTP)/library $(MY_LTP)/udp
#	$(MY_CFDP)/include $(MY_CFDP)/library

LOCAL_CFLAGS = -g -fsigned-char -Werror -Wall -Wno-unused-variable -Wno-int-to-pointer-cast -Dbionic -DBP_EXTENDED -DGDSSYMTAB -DUSING_SDR_POINTERS -DNO_SDR_TRACE -DNO_PSM_TRACE -DENABLE_IMC -DSBSP -DAGENT_ROLE
#	-DENABLE_ACS -DNO_PROXY -DNO_DIRLIST

LOCAL_LDLIBS :=  -llog

LOCAL_SRC_FILES := iondtn_administration.c iondtn_transmission.c iondtn_listinfo.cpp gdslogger.c $(MY_ICISOURCES) $(MY_DGRSOURCES) $(MY_BPSOURCES) $(MY_SBSPSOURCES) $(MY_NMSOURCES) $(MY_BSSSOURCES)

#	$(MY_RESTARTSOURCE) $(MY_LTPSOURCES) $(MY_TESTSOURCES) $(MY_CFDPSOURCES)

include $(BUILD_SHARED_LIBRARY)
