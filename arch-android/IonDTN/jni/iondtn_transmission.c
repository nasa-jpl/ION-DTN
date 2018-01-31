/*
 *  ION JNI interface.
 */
#include <jni.h>
#include "ionsec.h"
#include <android/log.h>
#include <bp.h>
#include <ion.h>
#include <bpP.h>

#define LOG_ADMIN_TAG "ION_TM"


/**
 * Attach to to the running ION instance
 */
JNIEXPORT jint JNICALL
Java_gov_nasa_jpl_iondtn_services_BundleService_attachION(JNIEnv *env,
                                                       jobject instance) {
    // Try attaching to a running ION instance
    if (bp_attach() < 0)
    {
        putErrmsg("Can't attach to BP.", NULL);
        return -1;
    }
    __android_log_print(ANDROID_LOG_DEBUG, "JNI_T", "Attached to ION");
    return 0;

}

/**
 * Detach from the running ION instance
 */
JNIEXPORT void JNICALL
Java_gov_nasa_jpl_iondtn_services_BundleService_detachION(JNIEnv *env,
                                                       jobject instance) {
    // Detach from a running ION instance
    bp_detach();
    __android_log_print(ANDROID_LOG_DEBUG, "JNI_T", "Detached from ION");
}

/**
 * Data structure that holds all necessary information after initializing a
 * specific receiver
 *
 * This data structure is used for keeping relevant information while making
 * the actual functions entirely stateless (to allow multiple receivers using
 * these functions at the same time)
 */
typedef struct {
    BpSAP	sap;
    Sdr		sdr;
} receiverData;

/**
 * Perform a bundle transmission via ION. The payload is the provided Java
 * byte array
 */
JNIEXPORT jint JNICALL
Java_gov_nasa_jpl_iondtn_services_BundleService_sendBundleION(JNIEnv *env,
                                                             jobject instance,
                                                              jstring dest_eid_,
                                                              jint qos,
                                                              jint ttl,
                                                              jbyteArray payload_,
                                                              jlong sapPtr) {

    Sdr		sdr;
    Object	extent;
    Object	bundleZco;
    ReqAttendant attendant;
    Object		newBundle;


    // Get all required parameters from the JNI interface
    const char *dest_EID = (*env)->GetStringUTFChars(env, dest_eid_, 0);
    char* payload = (char*) (*env)->GetByteArrayElements(env, payload_, 0);
    const size_t payloadLength = (size_t) (*env)->GetArrayLength(env,
                                                                 payload_);

    // Allocate and copy the EIDs to omit the cont specifier
    char* destEID = malloc(strlen(dest_EID)+1);
    istrcpy(destEID, dest_EID, strlen(dest_EID)+1);

    // Cast the long parameter back to a pointer to the receiverData structure
    BpSAP sap;
    if (sapPtr != 0) {
        sap = ((receiverData*) sapPtr)->sap;
    }
    else {
        sap = NULL;
    }

    if (bp_attach() < 0)
    {
        putErrmsg("Can't attach to BP.", NULL);

        // Properly free all temporarily allocated resources
        free(destEID);

        (*env)->ReleaseStringUTFChars(env, dest_eid_, dest_EID);
        (*env)->ReleaseByteArrayElements(env, payload_, (jbyte*)payload, 0);

        return -1;
    }

    // Start the attendant to initialize a blocking transmission
    if (ionStartAttendant(&attendant))
    {
        putErrmsg("Can't initialize blocking transmission.", NULL);

        // Properly free all temporarily allocated resources
        free(destEID);

        (*env)->ReleaseStringUTFChars(env, dest_eid_, dest_EID);
        (*env)->ReleaseByteArrayElements(env, payload_, (jbyte*)payload, 0);

        return -1;
    }

    // Assign the attendant and get the SDR
    //_attendant(&attendant);
    sdr = bp_get_sdr();

    // Ensure that there is actual payload to send
    if (payloadLength == 0)
    {
        return -1;
    }

    // Perform bundle transmission by copying the data to the sdr and then
    // initiate transmission by ION
    CHKZERO(sdr_begin_xn(sdr));
    extent = sdr_malloc(sdr, payloadLength);
    if (extent)
    {
        sdr_write(sdr, extent, payload, payloadLength);
    }

    if (sdr_end_xn(sdr) < 0)
    {
        putErrmsg("No space for ZCO extent.", NULL);

        // Properly free all temporarily allocated resources
        free(destEID);

        (*env)->ReleaseStringUTFChars(env, dest_eid_, dest_EID);
        (*env)->ReleaseByteArrayElements(env, payload_, (jbyte*)payload, 0);

        return -1;
    }

    bundleZco = ionCreateZco(ZcoSdrSource, extent, 0, (vast)payloadLength,
                             qos, 0, ZcoOutbound, &attendant);


    if (bundleZco == 0 || bundleZco == (Object) ERROR)
    {
        putErrmsg("Can't create ZCO extent.", NULL);

        // Properly free all temporarily allocated resources
        free(destEID);

        (*env)->ReleaseStringUTFChars(env, dest_eid_, dest_EID);
        (*env)->ReleaseByteArrayElements(env, payload_, (jbyte*)payload, 0);

        return -1;
    }

    if (bp_send(sap, destEID, NULL, ttl, qos,
                NoCustodyRequested, 0, 0, NULL, bundleZco,
                &newBundle) < 1)
    {
        putErrmsg("Can't send ADU.", NULL);

        // Properly free all temporarily allocated resources
        free(destEID);

        (*env)->ReleaseStringUTFChars(env, dest_eid_, dest_EID);
        (*env)->ReleaseByteArrayElements(env, payload_, (jbyte*)payload, 0);

        return -1;
    }


    ionStopAttendant(&attendant);

    bp_detach();

    __android_log_print(ANDROID_LOG_DEBUG, "JNI_T", "Sent bundle with payload"
            " length of %zu", payloadLength);

    // Properly free all temporarily allocated resources
    free(destEID);

    (*env)->ReleaseStringUTFChars(env, dest_eid_, dest_EID);
    (*env)->ReleaseByteArrayElements(env, payload_, (jbyte*)payload, 0);


    return 0;
}

/**
 * Perform a bundle transmission via ION. The payload is the provided Java
 * byte array
 */
JNIEXPORT jint JNICALL
Java_gov_nasa_jpl_iondtn_services_BundleService_sendBundleFileION(JNIEnv *env,
                                                              jobject instance,
                                                              jstring dest_eid_,
                                                              jint qos,
                                                              jint ttl,
                                                              jstring absPath_,
                                                              jlong sapPtr) {

    Sdr		sdr;
    Object	bundleZco;
    Object		newBundle;
    BpAncillaryData	ancillaryData = { 0, 0, 0 };
    BpCustodySwitch	custodySwitch = NoCustodyRequested;
    Object		fileRef;
    struct stat	statbuf;
    ReqAttendant attendant;
    vast aduLength;

    // Get all required parameters from the JNI interface
    const char *dest_EID = (*env)->GetStringUTFChars(env, dest_eid_, 0);
    const char *abs_Path = (*env)->GetStringUTFChars(env, absPath_, 0);

    // Allocate and copy the EIDs/Path to omit the cont specifier
    char* destEID = malloc(strlen(dest_EID)+1);
    istrcpy(destEID, dest_EID, strlen(dest_EID)+1);
    char* absPath = malloc(strlen(abs_Path)+1);
    istrcpy(absPath, abs_Path, strlen(abs_Path)+1);

    // Cast the long parameter back to a pointer to the receiverData structure
    BpSAP sap;
    if (sapPtr != 0) {
        sap = ((receiverData*) sapPtr)->sap;
    }
    else {
        sap = NULL;
    }

    if (bp_attach() < 0)
    {
        putErrmsg("Can't attach to BP.", NULL);

        // Properly free all temporarily allocated resources
        free(destEID);
        free(absPath);

        (*env)->ReleaseStringUTFChars(env, dest_eid_, dest_EID);
        (*env)->ReleaseStringUTFChars(env, absPath_, abs_Path);

        return -1;
    }

    // Start the attendant to initialize a blocking transmission
    if (ionStartAttendant(&attendant))
    {
        putErrmsg("Can't initialize blocking transmission.", NULL);

        // Properly free all temporarily allocated resources
        free(destEID);
        free(absPath);

        (*env)->ReleaseStringUTFChars(env, dest_eid_, dest_EID);
        (*env)->ReleaseStringUTFChars(env, absPath_, abs_Path);

        return -1;
    }

    writeMemoNote("Sending destination eid is: ",
                  destEID);
    if (stat(absPath, &statbuf) < 0)
    {
        putSysErrmsg("Can't stat the file", absPath);

        // Properly free all temporarily allocated resources
        free(destEID);
        free(absPath);

        (*env)->ReleaseStringUTFChars(env, dest_eid_, dest_EID);
        (*env)->ReleaseStringUTFChars(env, absPath_, abs_Path);


        return -1;
    }

    aduLength = statbuf.st_size;
    if (aduLength == 0)
    {
        writeMemoNote("[?] bpsendfile can't send file of length zero",
                      absPath);

        // Properly free all temporarily allocated resources
        free(destEID);
        free(absPath);

        (*env)->ReleaseStringUTFChars(env, dest_eid_, dest_EID);
        (*env)->ReleaseStringUTFChars(env, absPath_, abs_Path);

        return -1;
    }

    sdr = bp_get_sdr();
    CHKZERO(sdr_begin_xn(sdr));
    if (sdr_heap_depleted(sdr))
    {
        sdr_exit_xn(sdr);
        putErrmsg("Low on heap space, can't send file.", absPath);

        // Properly free all temporarily allocated resources
        free(destEID);
        free(absPath);

        (*env)->ReleaseStringUTFChars(env, dest_eid_, dest_EID);
        (*env)->ReleaseStringUTFChars(env, absPath_, abs_Path);

        return -1;
    }

    fileRef = zco_create_file_ref(sdr, absPath, NULL, ZcoOutbound);
    if (sdr_end_xn(sdr) < 0 || fileRef == 0)
    {
        putErrmsg("bpsendfile can't create file ref.", NULL);

        // Properly free all temporarily allocated resources
        free(destEID);
        free(absPath);

        (*env)->ReleaseStringUTFChars(env, dest_eid_, dest_EID);
        (*env)->ReleaseStringUTFChars(env, absPath_, abs_Path);

        return -1;
    }

    __android_log_print(ANDROID_LOG_DEBUG, "tag", "adu length is %lld",
                        aduLength);

    bundleZco = ionCreateZco(ZcoFileSource, fileRef, 0, aduLength,
                             (unsigned char)qos, ancillaryData.ordinal,
                             ZcoOutbound, NULL);

    if (bundleZco == 0 || bundleZco == (Object) ERROR)
    {
        putErrmsg("bpsendfile can't create ZCO.", NULL);

        // Properly free all temporarily allocated resources
        free(destEID);
        free(absPath);

        (*env)->ReleaseStringUTFChars(env, dest_eid_, dest_EID);
        (*env)->ReleaseStringUTFChars(env, absPath_, abs_Path);

        return -1;
    }
    else
    {
        if (bp_send(sap, destEID, NULL, ttl, qos, custodySwitch,
                    0, 0, &ancillaryData, bundleZco, &newBundle) <= 0)
        {
            putErrmsg("bpsendfile can't send file in bundle.", NULL);

            // Properly free all temporarily allocated resources
            free(destEID);
            free(absPath);

            (*env)->ReleaseStringUTFChars(env, dest_eid_, dest_EID);
            (*env)->ReleaseStringUTFChars(env, absPath_, abs_Path);

            return -1;
        }
    }

    writeErrmsgMemos();
    CHKZERO(sdr_begin_xn(sdr));
    zco_destroy_file_ref(sdr, fileRef);
    if (sdr_end_xn(sdr) < 0)
    {
        putErrmsg("bpsendfile can't destroy file reference.", NULL);
        // File reference seems broken but bundle has already been send, thus
        // still successful
    }

    ionStopAttendant(&attendant);

    bp_detach();

    __android_log_print(ANDROID_LOG_DEBUG, "JNI_T", "Sent bundle with payload"
            " length of %llu", aduLength);

    // Properly free all temporarily allocated resources
    free(destEID);
    free(absPath);

    (*env)->ReleaseStringUTFChars(env, dest_eid_, dest_EID);
    (*env)->ReleaseStringUTFChars(env, absPath_, abs_Path);

    return 0;
}


JNIEXPORT jobject JNICALL
Java_gov_nasa_jpl_iondtn_backend_ReceiverRunnable_waitForBundle(JNIEnv *env,
                                                                jobject instance,
                                                                jlong jniDataPtr,
                                                                jint payloadThreshold,
                                                                jstring fdPath_) {
    BpDelivery	dlv;
    vast	contentLength;
    ZcoReader	reader;
    vast		len;
    char* content;

    const char *fdPath = (*env)->GetStringUTFChars(env, fdPath_, 0);

    // Cast the long parameter back to a pointer to the receiverData structure
    receiverData* data = (receiverData*) jniDataPtr;

    if (data == NULL) {
        putErrmsg("Data is null!",
                  NULL);
        return NULL;
    }

    // Modify task ID, only receiver needs access
    data->sap->vpoint->appPid = sm_TaskIdSelf();

    // Receive data with a timeout of 1 second
    if (bp_receive(data->sap, &dlv, 1) < 0)
    {
        return NULL;
    }

    if (data == NULL) {
        putErrmsg("Data is null!",
                  NULL);
        return NULL;
    }

    // Return NULL on error cases
    if (dlv.result != BpPayloadPresent) {
        return NULL;
    }

    // Actual payload has been received --> access ION structures
    CHKZERO(sdr_begin_xn(data->sdr));
    contentLength = zco_source_data_length(data->sdr, dlv.adu);
    content = malloc((size_t)contentLength);
    sdr_exit_xn(data->sdr);
    zco_start_receiving(dlv.adu, &reader);
    CHKZERO(sdr_begin_xn(data->sdr));
    len = zco_receive_source(data->sdr, &reader,
                                      contentLength, content);
    if (sdr_end_xn(data->sdr) < 0 || len < 0)
    {
        putErrmsg("Can't handle delivery.",
                  NULL);
        return NULL;
    }



    jstring eid = (*env)->NewStringUTF(env, dlv.bundleSourceEid);

    jclass bundleClass = (*env)->FindClass(env,
                                           "gov/nasa/jpl/iondtn/DtnBundle");

    jmethodID bundleConstructor = (*env)->GetMethodID(env, bundleClass,
                                                    "<init>", "()V");

    jobject bundleObj = (*env)->NewObject(env, bundleClass, bundleConstructor);

    // Get the method id of an empty constructor in clazz
    jmethodID setEID = (*env)->GetMethodID(env, bundleClass, "setEID", "(Ljava/lang/String;)V");
    jmethodID setCreationTime = (*env)->GetMethodID(env, bundleClass, "setCreationTime", "(J)V");
    jmethodID setTimeToLive = (*env)->GetMethodID(env, bundleClass, "setTimeToLive", "(I)V");

    (*env)->CallVoidMethod(env, bundleObj, setEID, eid);
    (*env)->CallVoidMethod(env, bundleObj, setCreationTime, dlv
            .bundleCreationTime.seconds);
    (*env)->CallVoidMethod(env, bundleObj, setTimeToLive, dlv.timeToLive);

    if (contentLength >= payloadThreshold) {
        // Write received data to temp file
        FILE* ptr = fopen(fdPath,"w");
        fwrite((const void*)content, 1, (size_t)contentLength, ptr);
        fclose(ptr);

        // Set Bundle type to FD so that Java part can set file descriptor
        // accordingly
        jmethodID setPayloadFDType = (*env)->GetMethodID(env, bundleClass,
                                                            "setPayloadFDType", "()V");
        (*env)->CallVoidMethod(env, bundleObj, setPayloadFDType);
    }
    else {
        // Convert the payload from char* to an Java byte array
        jbyteArray ret = (*env)->NewByteArray(env, (jsize)contentLength);
        (*env)->SetByteArrayRegion (env, ret, 0, (jsize)contentLength, (jbyte*)
                content);

        jmethodID setPayloadByteArray = (*env)->GetMethodID(env, bundleClass,
                                                            "setPayloadByteArray", "([B)V");
        (*env)->CallVoidMethod(env, bundleObj, setPayloadByteArray, ret);
    }


    // Clean delivered data structures
    bp_release_delivery(&dlv, 1);

    free(content);

    // Return the Java byte array
    return bundleObj;
}

JNIEXPORT jlong JNICALL
Java_gov_nasa_jpl_iondtn_services_BundleService_openEndpointION(JNIEnv *env,
                                                                jobject instance,
                                                                jstring src_eid_) {
    const char *src_eid = (*env)->GetStringUTFChars(env, src_eid_, 0);

    // Allocate and copy the EID to omit the cont specifier
    char *eidm = malloc(strlen(src_eid) + 1);
    istrcpy(eidm, src_eid, 50);

    // Allocate and initialize receiver data structure
    // This structure is required to make all receiver functions stateless
    // (and thus allow multiple receivers at the same time)
    receiverData* data = malloc(sizeof(receiverData));
    data->sap = NULL;

    if (src_eid == NULL)
    {
        // empty EID, no initialization possible
        return 0;
    }

    // Open the specified endpoint
    if (bp_open(eidm, &data->sap) < 0)
    {
        putErrmsg("Can't open own endpoint.", eidm);
        return 0;
    }

    // Initialize sap variable and get SDR
    sm_TaskVar((void*)&data->sap);
    data->sdr = bp_get_sdr();

    // Properly release allocated local variables
    (*env)->ReleaseStringUTFChars(env, src_eid_, src_eid);
    free(eidm);

    // Return pointer to the allocated data structure
    // (As the JNI does not support pointers as return values, the cast to
    // long is required. All following functions can simply cast the long
    // parameter to a pointer of the data structure)
    return (long)data;
}

JNIEXPORT jboolean JNICALL
Java_gov_nasa_jpl_iondtn_services_BundleService_closeEndpointION(JNIEnv *env,
                                                                 jobject instance,
                                                                 jlong id) {

    // Cast the long parameter back to a pointer to the receiverData structure
    receiverData *data = (receiverData *) id;

    // Abort if provided pointer was not valid!
    if (data == NULL) {
        return 0;
    }

    // Modify task ID, closing thread is the owner now
    data->sap->vpoint->appPid = sm_TaskIdSelf();

    // Close the receiver in ION
    bp_close(data->sap);

    // Free the data structure allocated in initializeIonReceiver
    // -> no longer needed
    free(data);

    return 1;
}