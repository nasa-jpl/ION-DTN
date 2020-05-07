#include <stdio.h>
#include <stdlib.h>
#include <jni.h>
#include <string.h>

#include <al_bp_extB.h>

//#define DEBUG

#define JNIALBPENGINE "it/unibo/dtn/JAL/JALEngine"
#define JNIALBPSOCKET "it/unibo/dtn/JAL/BPSocket"
#define JNIALBPBUNDLE "it/unibo/dtn/JAL/Bundle"

static void addNewJNIClass(const char* javaClassName, JNINativeMethod* funcArray, size_t size);
static void initJNIArray();
static void destroyJNIArray();

#define CHECKFIDNOTNULL(f) {if(f==NULL){return FALSE;}}
#define CHECKMIDNOTNULL(m) {if(m==NULL){return FALSE;}}
static boolean_t copy_bundle_to_java(JNIEnv *env, jobject ALBPBundle_, al_bp_bundle_object_t bundle, al_bp_bundle_payload_location_t payloadLocation);
static boolean_t copy_bundle_from_java(JNIEnv *env, jobject ALBPBundle_, al_bp_bundle_object_t* bundle);

#ifdef DEBUG
static void debug_print_timestamp(al_bp_timestamp_t ts);
static void debug_print_bundle_object(al_bp_bundle_object_t b);
#endif

// ***** START ALBPENGINE REGION *****
static jint c_init(JNIEnv *env, jobject obj, jchar force_eid, jint ipn_node_forDTN2)
{
	return al_bp_extB_init(force_eid, ipn_node_forDTN2);
}

static void c_destroy(JNIEnv *env, jobject obj) {
	al_bp_extB_destroy();
}

static jchar c_get_eid_format(JNIEnv *env, jobject obj)
{
	return al_bp_extB_get_eid_format();
}

static JNINativeMethod ALBPEngineJNIFuncs[] = {
	{ "c_init", "(CI)I", (void *)&c_init },
	{ "c_destroy", "()V", (void *)&c_destroy },
	{ "c_get_eid_format", "()C", (void *)&c_get_eid_format }
};
// ***** END ALBPENGINE REGION *****

// ***** START ALBPSOCKET REGION *****
static jint c_unregister(JNIEnv *env, jobject obj, jint registrationDescriptor) {
	return al_bp_extB_unregister(registrationDescriptor);
}

static jint c_register(JNIEnv *env, jobject obj, jobject ALBPSocket_, jstring dtnDemuxString, jint IPNDemux) {
	char* dtnDemux = NULL;
	al_bp_extB_registration_descriptor RD;
	jclass ALBPSocket = (*env)->GetObjectClass(env, ALBPSocket_);
	
    dtnDemux = (char*) (*env)->GetStringUTFChars(env, dtnDemuxString , NULL);
	
	al_bp_extB_error_t result = al_bp_extB_register(&RD, dtnDemux, IPNDemux);
	
	// Debug
	#ifdef DEBUG
	printf("C: valore RD=%d\n", (int)RD);
	fflush(stdout);
	#endif
	
	jmethodID mid = (*env)->GetMethodID(env, ALBPSocket, "setRegistrationDescriptor_C", "(I)V");
	(*env)->CallVoidMethod(env, ALBPSocket_, mid, (jint)RD);

	return (jint) result;
}

#define CleanFree(p) {if (p!=NULL) {free(p);p=NULL;}}

static jint c_send(JNIEnv *env, jobject obj, jint registrationDescriptor, jobject ALBPBundle_) {
	al_bp_bundle_object_t bundle_object;
	al_bp_bundle_create(&bundle_object);
	
	// Convert bundle from Java to C format
	copy_bundle_from_java(env, ALBPBundle_, &bundle_object);
	
	al_bp_extB_error_t result = al_bp_extB_send((int)registrationDescriptor, bundle_object, bundle_object.spec->dest, bundle_object.spec->replyto);
	
	if (result == BP_EXTB_SUCCESS) {   // Set the creation timestamp back to Java
		jclass ALBPBundle = (*env)->GetObjectClass(env, ALBPBundle_);
		jmethodID mid;
		mid = (*env)->GetMethodID(env, ALBPBundle, "setCreationTimestamp_C", "(II)V");
		CHECKMIDNOTNULL(mid);
		(*env)->CallVoidMethod(env, ALBPBundle_, mid, (jint) bundle_object.id->creation_ts.secs, (jint) bundle_object.id->creation_ts.seqno);
	}
	
	CleanFree(bundle_object.payload->buf.buf_val);
	CleanFree(bundle_object.payload->filename.filename_val);
	if (bundle_object.spec->metadata.metadata_len > 0) {
		int i;
		for (i = 0; i < bundle_object.spec->metadata.metadata_len; i++) {
			CleanFree(bundle_object.spec->metadata.metadata_val[i].data.data_val);
		}
		CleanFree(bundle_object.spec->metadata.metadata_val);
		bundle_object.spec->metadata.metadata_len = 0;
	}
	if (bundle_object.spec->blocks.blocks_len > 0) {
		int i;
		for (i = 0; i < bundle_object.spec->blocks.blocks_len; i++) {
			CleanFree(bundle_object.spec->blocks.blocks_val[i].data.data_val);
		}
		CleanFree(bundle_object.spec->blocks.blocks_val);
		bundle_object.spec->blocks.blocks_len = 0;
	}
	
	al_bp_bundle_free(&bundle_object);
	return (jint) result;
}

#define ReadAndAssignIntFromMethodCalling(variable, methodName) { \
	jmethodID mid = (*env)->GetMethodID(env, ALBPBundle, methodName, "()I");\
	CHECKMIDNOTNULL(mid);\
	variable = (*env)->CallIntMethod(env, ALBPBundle_, mid);\
}

#define ReadAndAssignIntFromMethodCallingForMetadataAndBlocks(variable, value, methodName) { \
	jmethodID mid = (*env)->GetMethodID(env, ALBPBundle, methodName, "(I)I");\
	CHECKMIDNOTNULL(mid);\
	variable = (*env)->CallIntMethod(env, ALBPBundle_, mid, (jint) value);\
}

#define ReadAndAssignStringFromMethodCalling(variable, methodName) {\
	jmethodID mid = (*env)->GetMethodID(env, ALBPBundle, methodName, "()Ljava/lang/String;");\
	CHECKMIDNOTNULL(mid);\
	jstring jstr = (*env)->CallObjectMethod(env, ALBPBundle_, mid);\
	const char* str = (*env)->GetStringUTFChars(env, jstr, 0);\
	strcpy(variable, str);\
	(*env)->ReleaseStringUTFChars(env, jstr, str);\
}

static boolean_t copy_bundle_from_java(JNIEnv *env, jobject ALBPBundle_, al_bp_bundle_object_t* bundle) {
	jclass ALBPBundle = (*env)->GetObjectClass(env, ALBPBundle_);
	int i;
	
	// Bundle ID
	ReadAndAssignStringFromMethodCalling(&(bundle->id->source.uri[0]), "getSourceAsString");
	ReadAndAssignIntFromMethodCalling(bundle->id->creation_ts.secs, "getCreationTimestampSeconds");
	ReadAndAssignIntFromMethodCalling(bundle->id->creation_ts.seqno, "getCreationTimestampSequenceNumber");
	ReadAndAssignIntFromMethodCalling(bundle->id->frag_offset, "getFragmentOffset");
	ReadAndAssignIntFromMethodCalling(bundle->id->orig_length, "getOrigLength");
	
	// Bundle SPEC
	ReadAndAssignStringFromMethodCalling(&(bundle->spec->source.uri[0]), "getSourceAsString");
	ReadAndAssignStringFromMethodCalling(&(bundle->spec->dest.uri[0]), "getDestinationAsString");
	ReadAndAssignStringFromMethodCalling(&(bundle->spec->replyto.uri[0]), "getReplyToAsString");
	ReadAndAssignIntFromMethodCalling(bundle->spec->priority.priority, "getPriorityVal");
	ReadAndAssignIntFromMethodCalling(bundle->spec->priority.ordinal, "getPriorityOrdinal");
	ReadAndAssignIntFromMethodCalling(bundle->spec->dopts, "getDeliveryOptionsVal");
	ReadAndAssignIntFromMethodCalling(bundle->spec->expiration, "getExpiration");
	ReadAndAssignIntFromMethodCalling(bundle->spec->creation_ts.secs, "getCreationTimestampSeconds");
	ReadAndAssignIntFromMethodCalling(bundle->spec->creation_ts.seqno, "getCreationTimestampSequenceNumber");
	ReadAndAssignIntFromMethodCalling(bundle->spec->delivery_regid, "getDeliveryRegID");
	ReadAndAssignIntFromMethodCalling(bundle->spec->unreliable, "getUnreliableAsVal");
	ReadAndAssignIntFromMethodCalling(bundle->spec->critical, "getCriticalAsVal");
	ReadAndAssignIntFromMethodCalling(bundle->spec->flow_label, "getFlowLabel");

	// Metadata
	ReadAndAssignIntFromMethodCalling(bundle->spec->metadata.metadata_len, "getMetadataSize");
	if (bundle->spec->metadata.metadata_len > 0)
		bundle->spec->metadata.metadata_val = malloc(sizeof(al_bp_extension_block_t) * bundle->spec->metadata.metadata_len);
	for (i = 0; i < bundle->spec->metadata.metadata_len; i++) {
		ReadAndAssignIntFromMethodCallingForMetadataAndBlocks(bundle->spec->metadata.metadata_val[i].type, i, "getMetadataType");
		ReadAndAssignIntFromMethodCallingForMetadataAndBlocks(bundle->spec->metadata.metadata_val[i].flags, i, "getMetadataFlags");
		ReadAndAssignIntFromMethodCallingForMetadataAndBlocks(bundle->spec->metadata.metadata_val[i].data.data_len, i, "getMetadataDataSize");
		jmethodID mid = (*env)->GetMethodID(env, ALBPBundle, "getMetadataData", "(I)[B");
		CHECKMIDNOTNULL(mid);
		jbyteArray buffer = (*env)->CallObjectMethod(env, ALBPBundle_, mid, (jint) i);
		jbyte* bytes = (*env)->GetByteArrayElements(env, buffer, NULL);
		bundle->spec->metadata.metadata_val[i].data.data_val = malloc(bundle->spec->metadata.metadata_val[i].data.data_len);
		memcpy(bundle->spec->metadata.metadata_val[i].data.data_val, bytes, bundle->spec->metadata.metadata_val[i].data.data_len);
		(*env)->ReleaseByteArrayElements(env, buffer, bytes, JNI_ABORT);
	}
	
	// Blocks
	ReadAndAssignIntFromMethodCalling(bundle->spec->blocks.blocks_len, "getBlocksSize");
	if (bundle->spec->blocks.blocks_len > 0)
		bundle->spec->blocks.blocks_val = malloc(sizeof(al_bp_extension_block_t) * bundle->spec->blocks.blocks_len);
	for (i = 0; i < bundle->spec->blocks.blocks_len; i++) {
		ReadAndAssignIntFromMethodCallingForMetadataAndBlocks(bundle->spec->blocks.blocks_val[i].type, i, "getBlocksType");
		ReadAndAssignIntFromMethodCallingForMetadataAndBlocks(bundle->spec->blocks.blocks_val[i].flags, i, "getBlocksFlags");
		ReadAndAssignIntFromMethodCallingForMetadataAndBlocks(bundle->spec->blocks.blocks_val[i].data.data_len, i, "getBlocksDataSize");
		jmethodID mid = (*env)->GetMethodID(env, ALBPBundle, "getBlocksData", "(I)[B");
		CHECKMIDNOTNULL(mid);
		jbyteArray buffer = (*env)->CallObjectMethod(env, ALBPBundle_, mid, (jint) i);
		jbyte* bytes = (*env)->GetByteArrayElements(env, buffer, NULL);
		bundle->spec->blocks.blocks_val[i].data.data_val = malloc(bundle->spec->blocks.blocks_val[i].data.data_len);
		memcpy(bundle->spec->blocks.blocks_val[i].data.data_val, bytes, bundle->spec->blocks.blocks_val[i].data.data_len);
		(*env)->ReleaseByteArrayElements(env, buffer, bytes, JNI_ABORT);
	}
	
	// Bundle PAYLOAD
	ReadAndAssignIntFromMethodCalling(bundle->payload->location, "getPayloadLocationAsVal");
	switch(bundle->payload->location) {
		case BP_PAYLOAD_FILE:
		case BP_PAYLOAD_TEMP_FILE:
			ReadAndAssignIntFromMethodCalling(bundle->payload->filename.filename_len, "getPayloadFileNameSize");
			bundle->payload->filename.filename_len++;
			bundle->payload->filename.filename_val = malloc(bundle->payload->filename.filename_len + 1);
			ReadAndAssignStringFromMethodCalling(bundle->payload->filename.filename_val, "getPayloadFileName");
			bundle->payload->filename.filename_val[bundle->payload->filename.filename_len] = '\0';
			break;
			
		case BP_PAYLOAD_MEM:
			ReadAndAssignIntFromMethodCalling(bundle->payload->buf.buf_len, "getPayloadMemorySize");
			jmethodID mid = (*env)->GetMethodID(env, ALBPBundle, "getPayloadMemoryData", "()[B");
			CHECKMIDNOTNULL(mid);
			jbyteArray buffer = (*env)->CallObjectMethod(env, ALBPBundle_, mid);
			jbyte* bytes = (*env)->GetByteArrayElements(env, buffer, NULL);
			bundle->payload->buf.buf_val = malloc(bundle->payload->buf.buf_len);
			memcpy(bundle->payload->buf.buf_val, bytes, bundle->payload->buf.buf_len);
			(*env)->ReleaseByteArrayElements(env, buffer, bytes, JNI_ABORT);
			
			break;
			
		default: return FALSE;
	}
	bundle->payload->status_report = NULL;
	
	#ifdef DEBUG
	debug_print_bundle_object(*bundle);
	#endif

	return TRUE;
}

static jint c_receive(JNIEnv *env, jobject obj, jint registrationDescriptor, jobject ALBPBundle_, jint payloadLocation, jint timeout) {
	al_bp_bundle_object_t bundle_object;
	al_bp_bundle_create(&bundle_object);
	
	al_bp_extB_error_t result = al_bp_extB_receive((al_bp_extB_registration_descriptor) registrationDescriptor, &bundle_object, (al_bp_bundle_payload_location_t) payloadLocation, (al_bp_timeval_t) timeout);
	
	#ifdef DEBUG
	printf("Exited from al_bp_extB_receive.\n\tResult=%d\n", result);
	if (result != 12)
		debug_print_bundle_object(bundle_object);
	fflush(stdout);
	#endif
	
	if (result == BP_EXTB_SUCCESS) {
		boolean_t copied = copy_bundle_to_java(env, ALBPBundle_, bundle_object, (al_bp_bundle_payload_location_t) payloadLocation);

		if (!copied) {
			#ifdef DEBUG
			printf("Error on copying bundle to Java.\n");
			fflush(stdout);
			#endif
			result = BP_EXTB_ERRRECEIVE;
		}

		if (payloadLocation == BP_PAYLOAD_FILE || payloadLocation == BP_PAYLOAD_TEMP_FILE) {
			bundle_object.payload->filename.filename_val = NULL; // Avoid the deletion of the bundle file (removing file). Java will do it.
		}
	}
	
	al_bp_bundle_free(&bundle_object);
	
	return (jint) result;
}

static boolean_t copy_bundle_to_java(JNIEnv *env, jobject ALBPBundle_, al_bp_bundle_object_t bundle, al_bp_bundle_payload_location_t payloadLocation) {
	jclass ALBPBundle = (*env)->GetObjectClass(env, ALBPBundle_);
	jmethodID mid;
	jbyteArray buffer;
	int i;
	
	mid = (*env)->GetMethodID(env, ALBPBundle, "setSource_C", "(Ljava/lang/String;)V");
	CHECKMIDNOTNULL(mid);
	(*env)->CallVoidMethod(env, ALBPBundle_, mid, (*env)->NewStringUTF(env, bundle.spec->source.uri));
	
	mid = (*env)->GetMethodID(env, ALBPBundle, "setCreationTimestamp_C", "(II)V");
	CHECKMIDNOTNULL(mid);
	(*env)->CallVoidMethod(env, ALBPBundle_, mid, (jint) bundle.spec->creation_ts.secs, (jint) bundle.spec->creation_ts.seqno);
	
	mid = (*env)->GetMethodID(env, ALBPBundle, "setFragmentOffset_C", "(I)V");
	CHECKMIDNOTNULL(mid);
	(*env)->CallVoidMethod(env, ALBPBundle_, mid, (jint) bundle.id->frag_offset);
	
	mid = (*env)->GetMethodID(env, ALBPBundle, "setOrigLength_C", "(I)V");
	CHECKMIDNOTNULL(mid);
	(*env)->CallVoidMethod(env, ALBPBundle_, mid, (jint) bundle.id->orig_length);
	
	mid = (*env)->GetMethodID(env, ALBPBundle, "setDestination_C", "(Ljava/lang/String;)V");
	CHECKMIDNOTNULL(mid);
	(*env)->CallVoidMethod(env, ALBPBundle_, mid, (*env)->NewStringUTF(env, bundle.spec->dest.uri));
	
	mid = (*env)->GetMethodID(env, ALBPBundle, "setReplyTo_C", "(Ljava/lang/String;)V");
	CHECKMIDNOTNULL(mid);
	(*env)->CallVoidMethod(env, ALBPBundle_, mid, (*env)->NewStringUTF(env, bundle.spec->replyto.uri));
	
	mid = (*env)->GetMethodID(env, ALBPBundle, "setPriority_C", "(II)V");
	CHECKMIDNOTNULL(mid);
	(*env)->CallVoidMethod(env, ALBPBundle_, mid, (jint) bundle.spec->priority.priority, (jint) bundle.spec->priority.ordinal);
	
	mid = (*env)->GetMethodID(env, ALBPBundle, "setDeliveryOption_C", "(I)V");
	CHECKMIDNOTNULL(mid);
	(*env)->CallVoidMethod(env, ALBPBundle_, mid, (jint) bundle.spec->delivery_regid);
	
	mid = (*env)->GetMethodID(env, ALBPBundle, "setExpiration_C", "(I)V");
	CHECKMIDNOTNULL(mid);
	(*env)->CallVoidMethod(env, ALBPBundle_, mid, (jint) bundle.spec->expiration);
	
	mid = (*env)->GetMethodID(env, ALBPBundle, "setDeliveryRegID_C", "(I)V");
	CHECKMIDNOTNULL(mid);
	(*env)->CallVoidMethod(env, ALBPBundle_, mid, (jint) bundle.spec->delivery_regid);
	
	// METADATA
	mid = (*env)->GetMethodID(env, ALBPBundle, "addMetadata_C", "([BII)V");
	CHECKMIDNOTNULL(mid);
	for(i = 0; i < bundle.spec->metadata.metadata_len; i++) {
		buffer = (*env)->NewByteArray(env, (jint) bundle.spec->metadata.metadata_val->data.data_len);
		(*env)->SetByteArrayRegion(env, buffer, 0, (jint) bundle.spec->metadata.metadata_val->data.data_len, (jbyte*) bundle.spec->metadata.metadata_val->data.data_val);
		(*env)->CallVoidMethod(env, ALBPBundle_, mid, buffer, (jint) bundle.spec->metadata.metadata_val->flags, bundle.spec->metadata.metadata_val->type);	
	}
	
	// BLOCKS
	mid = (*env)->GetMethodID(env, ALBPBundle, "addBlock_C", "([BII)V");
	CHECKMIDNOTNULL(mid);
	for(i = 0; i < bundle.spec->blocks.blocks_len; i++) {
		buffer = (*env)->NewByteArray(env, (jint) bundle.spec->blocks.blocks_val->data.data_len);
		(*env)->SetByteArrayRegion(env, buffer, 0, (jint) bundle.spec->blocks.blocks_val->data.data_len, (jbyte*) bundle.spec->blocks.blocks_val->data.data_val);
		(*env)->CallVoidMethod(env, ALBPBundle_, mid, buffer, (jint) bundle.spec->blocks.blocks_val->flags, bundle.spec->blocks.blocks_val->type);	
	}
	
	mid = (*env)->GetMethodID(env, ALBPBundle, "setUnreliable_C", "(I)V");
	CHECKMIDNOTNULL(mid);
	(*env)->CallVoidMethod(env, ALBPBundle_, mid, (jint) bundle.spec->unreliable);
	
	mid = (*env)->GetMethodID(env, ALBPBundle, "setCritical_C", "(I)V");
	CHECKMIDNOTNULL(mid);
	(*env)->CallVoidMethod(env, ALBPBundle_, mid, (jint) bundle.spec->critical);
	
	mid = (*env)->GetMethodID(env, ALBPBundle, "setFlowLabel_C", "(I)V");
	CHECKMIDNOTNULL(mid);
	(*env)->CallVoidMethod(env, ALBPBundle_, mid, (jint) bundle.spec->flow_label);
	
	// PAYLOAD
	switch (payloadLocation) {
		case BP_PAYLOAD_MEM:
			mid = (*env)->GetMethodID(env, ALBPBundle, "setBuffer_C", "([B)V");
			CHECKMIDNOTNULL(mid);
			buffer = (*env)->NewByteArray(env, (jint) bundle.payload->buf.buf_len);
			(*env)->SetByteArrayRegion(env, buffer, 0, (jint) bundle.payload->buf.buf_len, (jbyte*) bundle.payload->buf.buf_val);
			(*env)->CallVoidMethod(env, ALBPBundle_, mid, buffer);
			break;
		
		case BP_PAYLOAD_FILE:
			mid = (*env)->GetMethodID(env, ALBPBundle, "setPayloadFile_C", "(Ljava/lang/String;)V");
			CHECKMIDNOTNULL(mid);
			(*env)->CallVoidMethod(env, ALBPBundle_, mid, (*env)->NewStringUTF(env, bundle.payload->filename.filename_val));
			break;
			
		case BP_PAYLOAD_TEMP_FILE:
			mid = (*env)->GetMethodID(env, ALBPBundle, "setPayloadTemporaryFile_C", "(Ljava/lang/String;)V");
			CHECKMIDNOTNULL(mid);
			(*env)->CallVoidMethod(env, ALBPBundle_, mid, (*env)->NewStringUTF(env, bundle.payload->filename.filename_val));
			break;
		
		default:
		break;
	}
	
	if (bundle.payload->status_report != NULL) {
			// BUNDLE PAYLOAD -> STATUS REPORT
			mid = (*env)->GetMethodID(env, ALBPBundle, "setSourceStatusReport_C", "(Ljava/lang/String;)V");
			CHECKMIDNOTNULL(mid);
			(*env)->CallVoidMethod(env, ALBPBundle_, mid, (*env)->NewStringUTF(env, bundle.payload->status_report->bundle_id.source.uri));
			
			mid = (*env)->GetMethodID(env, ALBPBundle, "setCreationTimestampStatusReport_C", "(II)V");
			CHECKMIDNOTNULL(mid);
			(*env)->CallVoidMethod(env, ALBPBundle_, mid, bundle.payload->status_report->bundle_id.creation_ts.secs, bundle.payload->status_report->bundle_id.creation_ts.seqno);
			
			mid = (*env)->GetMethodID(env, ALBPBundle, "setFragmentOffsetStatusReport_C", "(I)V");
			CHECKMIDNOTNULL(mid);
			(*env)->CallVoidMethod(env, ALBPBundle_, mid, bundle.payload->status_report->bundle_id.frag_offset);
			
			mid = (*env)->GetMethodID(env, ALBPBundle, "setOrigLengthStatusReport_C", "(I)V");
			CHECKMIDNOTNULL(mid);
			(*env)->CallVoidMethod(env, ALBPBundle_, mid, bundle.payload->status_report->bundle_id.orig_length);
			
			mid = (*env)->GetMethodID(env, ALBPBundle, "setReasonStatusReport_C", "(I)V");
			CHECKMIDNOTNULL(mid);
			(*env)->CallVoidMethod(env, ALBPBundle_, mid, bundle.payload->status_report->reason);
			
			mid = (*env)->GetMethodID(env, ALBPBundle, "setFlagStatusReport_C", "(I)V");
			CHECKMIDNOTNULL(mid);
			(*env)->CallVoidMethod(env, ALBPBundle_, mid, bundle.payload->status_report->flags);
			
			mid = (*env)->GetMethodID(env, ALBPBundle, "setReceiptTimestampStatusReport_C", "(II)V");
			CHECKMIDNOTNULL(mid);
			(*env)->CallVoidMethod(env, ALBPBundle_, mid, bundle.payload->status_report->receipt_ts.secs, bundle.payload->status_report->receipt_ts.seqno);
			
			mid = (*env)->GetMethodID(env, ALBPBundle, "setCustodyTimestampStatusReport_C", "(II)V");
			CHECKMIDNOTNULL(mid);
			(*env)->CallVoidMethod(env, ALBPBundle_, mid, bundle.payload->status_report->custody_ts.secs, bundle.payload->status_report->custody_ts.seqno);
			
			mid = (*env)->GetMethodID(env, ALBPBundle, "setForwardingTimestampStatusReport_C", "(II)V");
			CHECKMIDNOTNULL(mid);
			(*env)->CallVoidMethod(env, ALBPBundle_, mid, bundle.payload->status_report->forwarding_ts.secs, bundle.payload->status_report->forwarding_ts.seqno);
			
			mid = (*env)->GetMethodID(env, ALBPBundle, "setDeliveryTimestampStatusReport_C", "(II)V");
			CHECKMIDNOTNULL(mid);
			(*env)->CallVoidMethod(env, ALBPBundle_, mid, bundle.payload->status_report->delivery_ts.secs, bundle.payload->status_report->delivery_ts.seqno);
			
			mid = (*env)->GetMethodID(env, ALBPBundle, "setDeletionTimestampStatusReport_C", "(II)V");
			CHECKMIDNOTNULL(mid);
			(*env)->CallVoidMethod(env, ALBPBundle_, mid, bundle.payload->status_report->deletion_ts.secs, bundle.payload->status_report->deletion_ts.seqno);
			
			mid = (*env)->GetMethodID(env, ALBPBundle, "setAckByAppTimestampStatusReport_C", "(II)V");
			CHECKMIDNOTNULL(mid);
			(*env)->CallVoidMethod(env, ALBPBundle_, mid, bundle.payload->status_report->ack_by_app_ts.secs, bundle.payload->status_report->ack_by_app_ts.seqno);
		}
	
	return TRUE;
}

#ifdef DEBUG
static void debug_print_timestamp(al_bp_timestamp_t ts) {
	printf("%d.%d", ts.secs,ts.seqno);
}

static void debug_print_bundle_object(al_bp_bundle_object_t b) {
	// BUNDLE ID
	printf("\tid=%p\n",b.id);
	if (b.id != NULL) {
		printf("\t\tsource=%s\n",b.id->source.uri);
		printf("\t\tcreation_ts=%d.%d\n",b.id->creation_ts.secs, b.id->creation_ts.seqno);
		printf("\t\tfrag_offset=%d\n",b.id->frag_offset);
		printf("\t\torig_length=%d\n",b.id->orig_length);
	} else {
		printf("There are no b.id.\n");
	}
	
	// BUNDLE SPEC
	printf("\tspec=%p\n",b.spec);
	if (b.spec != NULL) {
		printf("\t\tsource=%s\n", b.spec->source.uri);
		printf("\t\tdest=%s\n", b.spec->dest.uri);
		printf("\t\treplyto=%s\n", b.spec->replyto.uri);
		printf("\t\tpriority=%d.%d\n", b.spec->priority.priority, b.spec->priority.ordinal);
		printf("\t\tdopts=%d\n", b.spec->dopts);
		printf("\t\texpiration=%d\n", b.spec->expiration);
		printf("\t\tcreation_ts=%d.%d\n", b.spec->creation_ts.secs, b.spec->creation_ts.seqno);
		printf("\t\tdelivery_regid=%d\n", b.spec->delivery_regid);
		printf("\t\tblocks len=%d\n", b.spec->blocks.blocks_len);
		printf("\t\tmetadata len=%d\n", b.spec->metadata.metadata_len);
		printf("\t\tunreliable=%d\n", (int)b.spec->unreliable);
		printf("\t\tcritical=%d\n", (int)b.spec->critical);
		printf("\t\tflow_label=%d\n", b.spec->flow_label);
	} else {
		printf("There are no b.spec.\n");
	}
	
	// BUNDLE PAYLOAD
	printf("\tpayload=%p\n", b.payload);
	if (b.payload != NULL) {
		printf("\t\tpayloadLocation=%d\n", b.payload->location);
		printf("\t\tfilename:\n\t\t\tlen=%d\n\t\t\tval=%s\n", b.payload->filename.filename_len, b.payload->filename.filename_val);
		printf("\t\tbuf:\n\t\t\tlen=%d\n\t\t\tval=%s\n", b.payload->buf.buf_len, b.payload->buf.buf_val);
		printf("\t\tstatus_report=%p\n", b.payload->status_report);
		
		if (b.payload->status_report != NULL) {
			// BUNDLE PAYLOAD -> STATUS REPORT
			printf("\t\t\t\tsource=%s\n", b.payload->status_report->bundle_id.source.uri);
			printf("\t\t\t\tcreation_ts=%d.%d\n",b.payload->status_report->bundle_id.creation_ts.secs, b.payload->status_report->bundle_id.creation_ts.seqno);
			printf("\t\t\t\tfrag_offset=%d\n",b.payload->status_report->bundle_id.frag_offset);
			printf("\t\t\t\torig_length=%d\n",b.payload->status_report->bundle_id.orig_length);
			printf("\t\t\treason=%d\n", b.payload->status_report->reason);
			printf("\t\t\tflags=%d\n", b.payload->status_report->flags);
	
			printf("\t\t\treceipt_ts=");
			debug_print_timestamp(b.payload->status_report->receipt_ts);
			printf("\n");
			printf("\t\t\tcustody_ts=");
			debug_print_timestamp(b.payload->status_report->custody_ts);
			printf("\n");
			printf("\t\t\tforwarding_ts=");
			debug_print_timestamp(b.payload->status_report->forwarding_ts);
			printf("\n");
			printf("\t\t\tdelivery_ts=");
			debug_print_timestamp(b.payload->status_report->delivery_ts);
			printf("\n");
			printf("\t\t\tdeletion_ts=");
			debug_print_timestamp(b.payload->status_report->deletion_ts);
			printf("\n");
			printf("\t\t\tack_by_app_ts=");
			debug_print_timestamp(b.payload->status_report->ack_by_app_ts);
			printf("\n");
		} else {
		printf("There are no b.payload->status_report.\n");
	}
	}  else {
		printf("There are no b.payload.\n");
	}
}
#endif

static jstring c_get_local_eid (JNIEnv *env, jobject obj, jint registrationDescriptor) {
	al_bp_endpoint_id_t local_eid = al_bp_extB_get_local_eid((al_bp_extB_registration_descriptor) registrationDescriptor);
	return (*env)->NewStringUTF(env, &(local_eid.uri[0]));
}

static JNINativeMethod ALBPSocketJNIFuncs[] = {
	{ "c_register", "(L"JNIALBPSOCKET";Ljava/lang/String;I)I", (void *)&c_register },
	{ "c_unregister", "(I)I", (void *)&c_unregister },
	{ "c_receive", "(IL"JNIALBPBUNDLE";II)I", (void*) &c_receive },
	{ "c_get_local_eid", "(I)Ljava/lang/String;", (void*) &c_get_local_eid},
	{ "c_send", "(IL"JNIALBPBUNDLE";)I", (void*) &c_send }
};
// ***** END ALBPSOCKET REGION *****

typedef struct {
	const char* className;
	JNINativeMethod* funcs;
	size_t size;
} ALBPJNIWrapperLinker;

static int albpJNIWrapperLinkerDim = 2;
static ALBPJNIWrapperLinker* albpJNIWrapperLinker = NULL;

static void addNewJNIClass(const char* javaClassName, JNINativeMethod* funcArray, size_t size) {
	static int arrayPosition = 0;
	if (arrayPosition >= albpJNIWrapperLinkerDim)
		return;
	albpJNIWrapperLinker[arrayPosition].className = javaClassName;
	albpJNIWrapperLinker[arrayPosition].funcs = funcArray;
	albpJNIWrapperLinker[arrayPosition].size = size / sizeof(JNINativeMethod);
	arrayPosition++;
}

static void destroyJNIArray() {
	free(albpJNIWrapperLinker);
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved)
{
	JNIEnv *env;
	int i;

	(void)reserved;

	if ((*vm)->GetEnv(vm, (void **)&env, JNI_VERSION_1_6) != JNI_OK)
		return -1;
	
	initJNIArray();
	
	// Registers natives for each class
	for (i = 0; i < albpJNIWrapperLinkerDim; i++) {
		jclass  cls;
		jint    res;
		ALBPJNIWrapperLinker current = albpJNIWrapperLinker[i];
		
		#ifdef DEBUG
		printf("Linking class: %s\n", current.className);
		#endif
		
		cls = (*env)->FindClass(env, current.className);
		if (cls == NULL) {
			return -1;
		}

		res = (*env)->RegisterNatives(env, cls, current.funcs, current.size);
		if (res != 0) {
			return -1;
		}
	}
	
	

	return JNI_VERSION_1_6;
}

JNIEXPORT void JNICALL JNI_OnUnload(JavaVM *vm, void *reserved)
{
	JNIEnv *env;
	int i;
	
	(void)reserved;

	if ((*vm)->GetEnv(vm, (void **)&env, JNI_VERSION_1_6) != JNI_OK)
		return;

	for (i = 0; i < albpJNIWrapperLinkerDim; i++) {
		jclass  cls;
		ALBPJNIWrapperLinker current = albpJNIWrapperLinker[i];
		
		cls = (*env)->FindClass(env, current.className);
		if (cls == NULL)
			return;

		(*env)->UnregisterNatives(env, cls);
	}
	
	destroyJNIArray();
	
	al_bp_extB_destroy(); // To be sure to do it
}

// EDIT THIS FUNCTION TO ADD NEW FUNCTIONALITIES
static void initJNIArray() {
	albpJNIWrapperLinker = calloc(albpJNIWrapperLinkerDim, sizeof(ALBPJNIWrapperLinker));
	
	addNewJNIClass(JNIALBPENGINE, &(ALBPEngineJNIFuncs[0]), sizeof(ALBPEngineJNIFuncs));
	addNewJNIClass(JNIALBPSOCKET, &(ALBPSocketJNIFuncs[0]), sizeof(ALBPSocketJNIFuncs));
}
