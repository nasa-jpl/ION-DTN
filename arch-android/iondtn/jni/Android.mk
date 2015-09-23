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

MY_ICI		:= ../../../ici

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
	$(MY_ICI)/utils/ionwarn.c

#	$(MY_ICI)/utils/ionexit.c      \

#	MY_RESTART	:= ../../../restart

#	MY_RESTARTSOURCE :=     \
#		$(MY_RESTART)/utils/ionrestart.c

MY_DGR		:= ../../../dgr

MY_DGRSOURCES :=     \
	$(MY_DGR)/library/libdgr.c    \

#	MY_LTP		:= ../../../ltp

#	MY_LTPSOURCES :=     \
#		$(MY_LTP)/library/libltp.c    \
#		$(MY_LTP)/library/libltpP.c   \
#		$(MY_LTP)/daemon/ltpclock.c   \
#		$(MY_LTP)/daemon/ltpmeter.c   \
#		$(MY_LTP)/udp/udplsi.c        \
#		$(MY_LTP)/udp/udplso.c        \
#		$(MY_LTP)/utils/ltpadmin.c

#	NOTE: can't include BSSP in bionic build until duplication
#	of function and variable names between bp/tcp/libtcpcla.c
#	and bssp/tcp/libtcpbsa.c is resolved.  Best approach is to
#	abstract this common TCP stuff out of BP and move it to ici.

#	MY_BSSP		:= ../../../bssp

#	MY_BSSPSOURCES :=     \
#		$(MY_BSSP)/library/libbssp.c    \
#		$(MY_BSSP)/library/libbsspP.c   \
#		$(MY_BSSP)/daemon/bsspclock.c   \
#		$(MY_BSSP)/udp/udpbsi.c         \
#		$(MY_BSSP)/udp/udpbso.c         \
#		$(MY_BSSP)/tcp/tcpbsi.c         \
#		$(MY_BSSP)/tcp/tcpbso.c         \
#		$(MY_BSSP)/tcp/libtcpbsa.c      \
#		$(MY_BSSP)/utils/bsspadmin.c

MY_BP		:= ../../../bp

MY_BPSOURCES :=      \
	$(MY_BP)/library/libbp.c      \
	$(MY_BP)/library/libbpP.c     \
	$(MY_BP)/daemon/bpclock.c     \
	$(MY_BP)/daemon/bptransit.c   \
	$(MY_BP)/utils/bpadmin.c      \
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
	$(MY_BP)/tcp/libtcpcla.c      \
	$(MY_BP)/dgr/dgrcla.c         \
	$(MY_BP)/library/eureka.c     \
	$(MY_BP)/library/bei.c        \
	$(MY_BP)/library/ext/phn/phn.c \
	$(MY_BP)/library/ext/ecos/ecos.c \
	$(MY_BP)/library/ext/meb/meb.c \
	$(MY_BP)/library/ext/bae/bae.c

#	$(MY_BP)/ltp/ltpcli.c         \
#	$(MY_BP)/ltp/ltpclo.c         \

MY_BSP		:= $(MY_BP)/library/ext/bsp

MY_BSPSOURCES :=                     \
	$(MY_BSP)/ciphersuites.c     \
	$(MY_BSP)/ciphersuites/bab_hmac_sha1.c    \
	$(MY_BSP)/ciphersuites/bib_hmac_sha256.c  \
	$(MY_BSP)/ciphersuites/bcb_arc4.c         \
	$(MY_BSP)/bsputil.c          \
	$(MY_BSP)/bspbab.c           \
	$(MY_BSP)/bspbib.c           \
	$(MY_BSP)/bspbcb.c

MY_BSS		:= ../../../bss

MY_BSSSOURCES :=    \
	$(MY_BSS)/library/libbss.c    \
	$(MY_BSS)/library/libbssP.c

#	MY_TEST		:= $(MY_BP)/test

#	MY_TESTSOURCES =     \
#		$(MY_TEST)/bpsource.c        \
#		$(MY_TEST)/bpsink.c

#	MY_CFDP		:= ../../../cfdp

#	MY_CFDPSOURCES :=    \
#		$(MY_CFDP)/library/libcfdp.c    \
#		$(MY_CFDP)/library/libcfdpP.c   \
#		$(MY_CFDP)/library/libcfdpops.c \
#		$(MY_CFDP)/bp/bputa.c           \
#		$(MY_CFDP)/daemon/cfdpclock.c   \
#		$(MY_CFDP)/utils/cfdpadmin.c    \

LOCAL_C_INCLUDES := $(MY_ICI)/include $(MY_ICI)/library $(MY_ICI)/libbloom-master $(MY_ICI)/libbloom-master/murmur2 $(MY_DGR)/include $(MY_BP)/include $(MY_BP)/library $(MY_BP)/ipn $(MY_BP)/imc $(MY_BP)/dtn2 $(MY_BP)/ipnd $(MY_BP)/library/ext $(MY_BP)/library/ext/bsp $(MY_BP)/library/ext/bsp/ciphersuites $(MY_BP)/library/ext/ecos $(MY_BP)/library/ext/meb $(MY_BP)/library/ext/bae $(MY_BP)/library/ext/phn $(MY_BP)/tcp $(MY_BP)/udp $(MY_BSS)/include $(MY_BSS)/library

#	$(MY_BSSP)/include $(MY_BSSP)/library $(MY_BSSP)/udp $(MY_BSSP)/tcp
#	$(MY_LTP)/include $(MY_LTP)/library $(MY_LTP)/udp 
#	$(MY_CFDP)/include $(MY_CFDP)/library

LOCAL_CFLAGS = -g -Wall -Werror -Dbionic -DBP_EXTENDED -DGDSSYMTAB -DGDSLOGGER -DUSING_SDR_POINTERS -DNO_SDR_TRACE -DNO_PSM_TRACE -DENABLE_IMC
#	-DENABLE_ACS -DNO_PROXY -DNO_DIRLIST

LOCAL_SRC_FILES := iondtn.c $(MY_ICISOURCES) $(MY_DGRSOURCES) $(MY_BPSOURCES) $(MY_BSPSOURCES) $(MY_BSSSOURCES)

#	$(MY_RESTARTSOURCE) $(MY_LTPSOURCES) $(MY_TESTSOURCES) $(MY_CFDPSOURCES)

include $(BUILD_SHARED_LIBRARY)
