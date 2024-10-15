#
#    Copyright 2004-2006 Intel Corporation
# 
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
# 
#        http://www.apache.org/licenses/LICENSE-2.0
# 
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.
#

#
# Makefile for DTNME/servlib
#

#
# Source and object rules
#

include $(CURDIR)/../servlib/dtpc.make

BUNDLING_SRCS := 												\
	bundling/AcsExpirationTimer.cc          					\
	bundling/AggregateCustodySignal.cc      					\
	bundling/BIBEExtractor.cc									\
	bundling/BlockInfo.cc										\
	bundling/BlockProcessor.cc									\
	bundling/BP6_APIBlockProcessor.cc							\
	bundling/BP6_BlockProcessor.cc								\
	bundling/BP6_CustodyTransferEnhancementBlockProcessor.cc	\
	bundling/BP6_ExtendedClassOfServiceBlockProcessor.cc		\
	bundling/BP6_MetadataBlockProcessor.cc						\
	bundling/BP6_PayloadBlockProcessor.cc						\
	bundling/BP6_PreviousHopBlockProcessor.cc					\
	bundling/BP6_PrimaryBlockProcessor.cc						\
	bundling/BP6_UnknownBlockProcessor.cc						\
	bundling/BP6_BundleStatusReport.cc							\
	bundling/BP7_BlockProcessor.cc								\
	bundling/BP7_BundleAgeBlockProcessor.cc						\
	bundling/BP7_HopCountBlockProcessor.cc						\
	bundling/BP7_PayloadBlockProcessor.cc						\
	bundling/BP7_PreviousNodeBlockProcessor.cc					\
	bundling/BP7_PrimaryBlockProcessor.cc						\
	bundling/BP7_UnknownBlockProcessor.cc						\
	bundling/Bundle.cc					\
	bundling/BundleActions.cc			\
	bundling/BundleDaemon.cc			\
	bundling/BundleDaemonACS.cc			\
	bundling/BundleDaemonInput.cc		\
	bundling/BundleDaemonOutput.cc		\
	bundling/BundleDaemonStorage.cc		\
	bundling/BundleDaemonCleanup.cc		\
	bundling/BundleDetail.cc			\
	bundling/BundleEventHandler.cc		\
	bundling/BundleInfoCache.cc			\
	bundling/BundleList.cc				\
	bundling/BundleListBase.cc			\
	bundling/BundleListIntMap.cc		\
	bundling/BundleListStrMap.cc		\
	bundling/BundleListStrMultiMap.cc	\
	bundling/BundleMappings.cc			\
	bundling/BundlePayload.cc			\
	bundling/BundleProtocol.cc			\
	bundling/BundleProtocolVersion6.cc	\
	bundling/BundleProtocolVersion7.cc	\
	bundling/BundleStatusReport.cc		\
	bundling/BundleTimestamp.cc			\
	bundling/CborUtil.cc				\
	bundling/CborUtilBP7.cc				\
	bundling/CustodyList.cc				\
	bundling/CustodySignal.cc			\
	bundling/CustodyTimer.cc			\
	bundling/Dictionary.cc          	\
	bundling/ForwardingInfo.cc			\
	bundling/ForwardingLog.cc			\
	bundling/FragmentManager.cc			\
	bundling/FragmentState.cc			\
	bundling/ExpirationTimer.cc			\
	bundling/GbofId.cc  		        \
	bundling/MetadataBlock.cc			\
	bundling/SDNV.cc					\

CONTACT_SRCS :=						\
	contacts/AlwaysOnLink.cc		\
	contacts/Contact.cc				\
	contacts/ContactManager.cc		\
        contacts/ContactPlanner.cc              \
	contacts/Interface.cc			\
	contacts/InterfaceTable.cc		\
	contacts/Link.cc				\
	contacts/OndemandLink.cc		\
	contacts/OpportunisticLink.cc	\
	contacts/ScheduledLink.cc		\

CONV_LAYER_SRCS :=								\
	conv_layers/BIBEConvergenceLayer.cc  		\
	conv_layers/ConnectionConvergenceLayer.cc 	\
	conv_layers/CLConnection.cc 				\
	conv_layers/ConvergenceLayer.cc				\
	conv_layers/IPConvergenceLayer.cc			\
	conv_layers/IPConvergenceLayerUtils.cc		\
	conv_layers/NullConvergenceLayer.cc			\
	conv_layers/StreamConvergenceLayer.cc   	\
	conv_layers/TCPConvergenceLayer.cc			\
	conv_layers/TCPConvergenceLayerV3.cc		\
	conv_layers/UDPConvergenceLayer.cc			\
	conv_layers/LTPUDPConvergenceLayer.cc		\
	ltp/LTPCommon.cc	        				\
	ltp/LTPEngine.cc	        				\
	conv_layers/LTPUDPReplayConvergenceLayer.cc	\
	conv_layers/LTPUDPReplayCommon.cc			\
	conv_layers/STCPConvergenceLayer.cc     	\
	conv_layers/RecvRawConvergenceLayer.cc		\
	conv_layers/MinimalTCPConvergenceLayer.cc	\


CMD_SRCS :=						\
	cmd/AcsCommand.cc			\
	cmd/APICommand.cc			\
	cmd/BPCommand.cc			\
	cmd/BundleCommand.cc		\
	cmd/CompletionNotifier.cc	\
	cmd/InterfaceCommand.cc		\
	cmd/LinkCommand.cc          \
	cmd/ParamCommand.cc			\
	cmd/RegistrationCommand.cc	\
	cmd/RouteCommand.cc			\
	cmd/ShutdownCommand.cc		\
	cmd/StorageCommand.cc		\
	cmd/TestCommand.cc			\
        cmd/ContactCommand.cc           \


NAMING_SRCS :=					\
	naming/EndpointID.cc		\
	naming/EndpointIDOpt.cc		\
	naming/Scheme.cc			\
	naming/SchemeTable.cc		\
	naming/DTNScheme.cc			\
	naming/IPNScheme.cc			\
	naming/IMCScheme.cc			\
	naming/WildcardScheme.cc	\

REGISTRATION_SRCS :=						\
	reg/AdminRegistration.cc				\
	reg/AdminRegistrationIpn.cc				\
	reg/APIRegistration.cc					\
	reg/LoggingRegistration.cc				\
	reg/Registration.cc						\
	reg/RegistrationInitialLoadThread.cc	\
	reg/RegistrationTable.cc				\
	reg/PingRegistration.cc					\
	reg/IpnEchoRegistration.cc				\
	reg/TclRegistration.cc					\

ROUTING_SRCS :=							\
	routing/ExternalRouter.cc			\
	routing/ExternalRouterIF.cc			\
	routing/ExternalRouterClientIF.cc	\
	routing/ExternalRouterServerIF.cc	\
	routing/BundleRouter.cc				\
	routing/RouteEntry.cc				\
	routing/RouteTable.cc				\
	routing/RouterInfo.cc				\
	routing/TableBasedRouter.cc			\
	routing/UniboCGRBundleRouter.cc			\

STORAGE_SRCS :=						\
	storage/BundleStore.cc			\
	storage/LinkStore.cc			\
	storage/GlobalStore.cc			\
	storage/RegistrationStore.cc	\
	storage/PendingAcsStore.cc		\

DTPC_SRCS :=							\
	cmd/DtpcCommand.cc					\
	dtpc/DtpcApplicationDataItem.cc		\
	dtpc/DtpcDaemon.cc					\
	dtpc/DtpcDataPduCollector.cc		\
	dtpc/DtpcDataPduCollectorStore.cc	\
	dtpc/DtpcDeliverPduTimer.cc			\
	dtpc/DtpcPayloadAggregationTimer.cc	\
	dtpc/DtpcPayloadAggregator.cc		\
	dtpc/DtpcPayloadAggregatorStore.cc	\
	dtpc/DtpcProfile.cc					\
	dtpc/DtpcProfileStore.cc			\
	dtpc/DtpcProfileTable.cc			\
	dtpc/DtpcProtocolDataUnit.cc		\
	dtpc/DtpcReceiver.cc				\
	dtpc/DtpcRegistration.cc			\
	dtpc/DtpcRetransmitTimer.cc			\
	dtpc/DtpcTopicAggregator.cc			\
	dtpc/DtpcTopicAggregatorList.cc		\
	dtpc/DtpcTopic.cc					\
	dtpc/DtpcTopicCollector.cc			\
	dtpc/DtpcTopicCollectorList.cc		\
	dtpc/DtpcTopicExpirationTimer.cc	\
	dtpc/DtpcTopicStore.cc				\
	dtpc/DtpcTopicTable.cc				\

UNIBOCGR_SRCS :=				\
	routing/Unibo-CGR/core/bundles/bundles.c	\
	routing/Unibo-CGR/core/cgr/cgr.c		\
	routing/Unibo-CGR/core/cgr/phase_one.c	\
	routing/Unibo-CGR/core/cgr/phase_two.c	\
	routing/Unibo-CGR/core/cgr/phase_three.c	\
	routing/Unibo-CGR/core/contact_plan/contactPlan.c \
	routing/Unibo-CGR/core/contact_plan/ranges/ranges.c \
	routing/Unibo-CGR/core/contact_plan/nodes/nodes.c \
	routing/Unibo-CGR/core/contact_plan/contacts/contacts.c \
	routing/Unibo-CGR/dtnme/interface/interface_unibocgr_dtn2.cc \
	routing/Unibo-CGR/core/library/commonFunctions.c \
	routing/Unibo-CGR/core/library/log/log.c	\
	routing/Unibo-CGR/core/library/list/list.c	\
	routing/Unibo-CGR/core/msr/msr_utils.c		\
	routing/Unibo-CGR/core/msr/msr.c		\
	routing/Unibo-CGR/core/library_from_ion/scalar/scalar.c \
	routing/Unibo-CGR/core/library_from_ion/rbt/rbt.c	\
	routing/Unibo-CGR/core/routes/routes.c	\
	routing/Unibo-CGR/core/time_analysis/time.c \

SERVLIB_SRCS := 				\
	DTNServer.cc				\
	$(BUNDLING_SRCS) 			\
	$(CONTACT_SRCS)				\
	$(CONV_LAYER_SRCS)			\
	$(CMD_SRCS) 				\
	$(NAMING_SRCS) 				\
	$(REGISTRATION_SRCS)		\
	$(ROUTING_SRCS)				\
	$(STORAGE_SRCS)				\
	$(UNIBOCGR_SRCS)			\

# Check if we should include DTPC sources in the SERVLIB sources
ifeq ($(DTPC_ENABLED),1)
SERVLIB_SRCS :=			\
	$(SERVLIB_SRCS)		\
	$(DTPC_SRCS)
endif

SERVLIB_OBJS := $(SERVLIB_SRCS:.cc=.o)
SERVLIB_OBJS := $(SERVLIB_OBJS:.c=.o)
SERVLIB_OBJS := $(SERVLIB_OBJS) ../dtn-version.o

ALLSRCS := $(SERVLIB_SRCS)

#
# Other makefiles include this one so they can link with the object
# files directly and not use the servlib, but in that case, all we
# want are the object lists, not the rules below
#
ifeq ($(SERVLIB_MAKEFILE_INCLUDE),)

#
# Default target is to build the library
#
LIBFILES := libdtnserv.a
all: $(LIBFILES)

servlib: libdtnserv.a
libdtnserv.a: $(SERVLIB_OBJS)
	rm -f $@
	$(AR) ruc $@ $^
	$(RANLIB) $@ || true

#
# Make sure SRCDIR is set (.. by default)
#
ifeq ($(SRCDIR),)
SRCDIR   := ..
BUILDDIR := ..
endif

#
# Include the common rules.
#
include ../Rules.make

endif
