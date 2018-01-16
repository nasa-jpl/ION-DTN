//
// Created by Wiewel, Robert (312B-Affiliate) on 9/29/17.
//

#include <iostream>
#include <jni.h>
#include "ionsec.h"
#include <android/log.h>
#include <bpP.h>
#include <string>
#include <ionsec.h>
#include "rfx.h"
#include "bpP.h"


std::string* buffer;

void setupBuffer() {
    buffer = new std::string();
}

void deleteBuffer() {
    delete(buffer);
}

void appendData(char* str) {
    buffer->append(str);
}

void appendChar(char c) {
    buffer->append(1, c);
}

void appendData(const char* str) {
    buffer->append(str);
}

void appendInt(int val) {
    char cBuffer[50];
    sprintf(cBuffer, "%d", val);
    buffer->append(cBuffer);
}

void appendNewLine() {
    buffer->append("\n");
}

extern "C" {
JNIEXPORT jstring JNICALL
Java_gov_nasa_jpl_iondtn_backend_NativeAdapter_getContactListION(JNIEnv *env,
                                                              jobject instance) {

    if (ionAttach() != 0) {
        __android_log_write(ANDROID_LOG_ERROR, "List", "Attach failed!");
        jstring retVal = (*env).NewStringUTF("");
        deleteBuffer();
        return retVal;
    }

    Sdr		sdr = getIonsdr();
    PsmPartition	ionwm = getIonwm();
    IonVdb		*vdb = getIonVdb();
    PsmAddress	elt;
    PsmAddress	addr;
    char		bbuffer[RFX_NOTE_LEN];

    setupBuffer();
    if (!sdr_begin_xn(sdr)) {
        __android_log_write(ANDROID_LOG_ERROR, "List", "SDR invalid");
        jstring retVal = (*env).NewStringUTF("");
        deleteBuffer();
        return retVal;
    }

    for (elt = sm_rbt_first(ionwm, vdb->contactIndex); elt;
         elt = sm_rbt_next(ionwm, elt))
    {
        addr = sm_rbt_data(ionwm, elt);
        rfx_print_contact(addr, bbuffer);
        appendData(bbuffer);
        appendNewLine();
    }

    sdr_exit_xn(sdr);

    jstring retVal = (*env).NewStringUTF(buffer->c_str());

    deleteBuffer();

    return retVal;
}

JNIEXPORT jstring JNICALL
Java_gov_nasa_jpl_iondtn_backend_NativeAdapter_getRangeListION(JNIEnv *env,
                                                                 jobject instance) {

    if (ionAttach() != 0) {
        __android_log_write(ANDROID_LOG_ERROR, "List", "Attach failed!");
        jstring retVal = (*env).NewStringUTF("");
        deleteBuffer();
        return retVal;
    }

    Sdr		sdr = getIonsdr();
    PsmPartition	ionwm = getIonwm();
    IonVdb		*vdb = getIonVdb();
    PsmAddress	elt;
    PsmAddress	addr;
    char		bbuffer[RFX_NOTE_LEN];

    setupBuffer();
    if (!sdr_begin_xn(sdr)) {
        __android_log_write(ANDROID_LOG_ERROR, "List", "SDR invalid");
        jstring retVal = (*env).NewStringUTF("");
        deleteBuffer();
        return retVal;
    }

    for (elt = sm_rbt_first(ionwm, vdb->rangeIndex); elt;
         elt = sm_rbt_next(ionwm, elt))
    {
        addr = sm_rbt_data(ionwm, elt);
        rfx_print_range(addr, bbuffer);
        appendData(bbuffer);
        appendNewLine();
    }

    sdr_exit_xn(sdr);

    jstring retVal = (*env).NewStringUTF(buffer->c_str());

    deleteBuffer();

    return retVal;
}

JNIEXPORT jstring JNICALL
Java_gov_nasa_jpl_iondtn_backend_NativeAdapter_getSchemeListION(JNIEnv *env,
                                                               jobject instance) {

    if (ionAttach() != 0) {
        __android_log_write(ANDROID_LOG_ERROR, "List", "Attach failed!");
        jstring retVal = (*env).NewStringUTF("");
        deleteBuffer();
        return retVal;
    }

    Sdr		sdr = getIonsdr();
    PsmPartition	ionwm = getIonwm();
    IonVdb		*vdb = getIonVdb();
    PsmAddress	elt;
    PsmAddress	addr;
    char		bbuffer[RFX_NOTE_LEN];
    VScheme		*vscheme;
    char	fwdCmdBuffer[SDRSTRING_BUFSZ];
    char	*fwdCmd;
    char	admAppCmdBuffer[SDRSTRING_BUFSZ];
    char	*admAppCmd;


    setupBuffer();
    if (!sdr_begin_xn(sdr)) {
        __android_log_write(ANDROID_LOG_ERROR, "List", "SDR invalid");
        jstring retVal = (*env).NewStringUTF("");
        deleteBuffer();
        return retVal;
    }

    for (elt = sm_list_first(ionwm, (getBpVdb())->schemes); elt;
         elt = sm_list_next(ionwm, elt))
    {
        vscheme = (VScheme *) psp(ionwm, sm_list_data(ionwm, elt));
        OBJ_POINTER(Scheme, scheme);


        GET_OBJ_POINTER(sdr, Scheme, scheme, sdr_list_data(sdr,
                                                           vscheme->schemeElt));

        appendData(scheme->name);
        appendData(" ");

        if (sdr_string_read(sdr, fwdCmdBuffer, scheme->fwdCmd) < 0)
        {
            appendData("?");
        }
        else
        {
            appendData(fwdCmdBuffer);
        }
        appendData(" ");


        if (sdr_string_read(sdr, admAppCmdBuffer, scheme->admAppCmd) < 0)
        {
            appendData("?");
        }
        else
        {
            appendData(admAppCmdBuffer);
        }
        appendData(" ");

        if (vscheme->admAppPid > 0 && sm_TaskExists(vscheme->admAppPid)) {
            appendData("true");
        }
        else {
            appendData("false");
        }

        appendNewLine();
    }

    sdr_exit_xn(sdr);

    jstring retVal = (*env).NewStringUTF(buffer->c_str());

    deleteBuffer();

    return retVal;
}

JNIEXPORT jstring JNICALL
Java_gov_nasa_jpl_iondtn_backend_NativeAdapter_getEndpointListION(JNIEnv *env,
                                                                jobject instance) {

    if (ionAttach() != 0) {
        __android_log_write(ANDROID_LOG_ERROR, "List", "Attach failed!");
        jstring retVal = (*env).NewStringUTF("");
        deleteBuffer();
        return retVal;
    }

    Sdr		sdr = getIonsdr();
    PsmPartition	ionwm = getIonwm();
    PsmAddress	elt, elt2;
    VScheme		*vscheme;
    VEndpoint	*vpoint;
    char	recvRule;
    char	recvScriptBuffer[SDRSTRING_BUFSZ];
    char	*recvScript = recvScriptBuffer;


    setupBuffer();
    if (!sdr_begin_xn(sdr)) {
        __android_log_write(ANDROID_LOG_ERROR, "List", "SDR invalid");
        jstring retVal = (*env).NewStringUTF("");
        deleteBuffer();
        return retVal;
    }

    for (elt = sm_list_first(ionwm, (getBpVdb())->schemes); elt;
         elt = sm_list_next(ionwm, elt))
    {
        vscheme = (VScheme *) psp(ionwm, sm_list_data(ionwm, elt));
        OBJ_POINTER(Scheme, scheme);

        for (elt2 = sm_list_first(ionwm, vscheme->endpoints); elt2;
             elt2 = sm_list_next(ionwm, elt2))
        {
            vpoint = (VEndpoint *) psp(ionwm, sm_list_data(ionwm, elt2));
            OBJ_POINTER(Endpoint, endpoint);
            GET_OBJ_POINTER(sdr,
                            Endpoint,
                            endpoint,
                            sdr_list_data(sdr, vpoint->endpointElt));
            GET_OBJ_POINTER(sdr, Scheme, scheme, endpoint->scheme);
            if (endpoint->recvRule == EnqueueBundle)
            {
                recvRule = 'q';
            }
            else
            {
                recvRule = 'x';
            }

            if (endpoint->recvScript == 0)
            {
                recvScriptBuffer[0] = '-';
                recvScriptBuffer[1] = '\0';
            }
            else
            {
                if (sdr_string_read(sdr, recvScriptBuffer, endpoint->recvScript)
                    < 0)
                {
                    recvScriptBuffer[0] = '-';
                    recvScriptBuffer[1] = '\0';
                }
            }
            appendData(scheme->name);
            appendData(":");
            appendData(endpoint->nss);
            appendData(" ");
            appendChar(recvRule);
            appendData(" ");
            appendData(recvScript);
            appendNewLine();
        }
    }

    sdr_exit_xn(sdr);

    jstring retVal = (*env).NewStringUTF(buffer->c_str());

    deleteBuffer();

    return retVal;
}

JNIEXPORT jstring JNICALL
Java_gov_nasa_jpl_iondtn_backend_NativeAdapter_getProtocolListION(JNIEnv *env,
                                                                jobject instance) {

    if (ionAttach() != 0) {
        __android_log_write(ANDROID_LOG_ERROR, "List", "Attach failed!");
        jstring retVal = (*env).NewStringUTF("");
        deleteBuffer();
        return retVal;
    }

    Sdr		sdr = getIonsdr();
    PsmAddress	elt;


    setupBuffer();
    if (!sdr_begin_xn(sdr)) {
        __android_log_write(ANDROID_LOG_ERROR, "List", "SDR invalid");
        jstring retVal = (*env).NewStringUTF("");
        deleteBuffer();
        return retVal;
    }

    OBJ_POINTER(ClProtocol, clp);

    for (elt = sdr_list_first(sdr, (getBpConstants())->protocols); elt;
         elt = sdr_list_next(sdr, elt))
    {
        GET_OBJ_POINTER(sdr, ClProtocol, clp, sdr_list_data(sdr, elt));

        appendData(clp->name);
        appendData(" ");
        appendInt(clp->payloadBytesPerFrame);
        appendData(" ");
        appendInt(clp->overheadPerFrame);
        appendData(" ");
        appendInt(clp->protocolClass);
        appendNewLine();
    }

    sdr_exit_xn(sdr);

    jstring retVal = (*env).NewStringUTF(buffer->c_str());

    deleteBuffer();

    return retVal;
}

JNIEXPORT jstring JNICALL
Java_gov_nasa_jpl_iondtn_backend_NativeAdapter_getInductListION(JNIEnv *env,
                                                                jobject instance) {

    if (ionAttach() != 0) {
        __android_log_write(ANDROID_LOG_ERROR, "List", "Attach failed!");
        jstring retVal = (*env).NewStringUTF("");
        deleteBuffer();
        return retVal;
    }

    Sdr		sdr = getIonsdr();
    PsmPartition	ionwm = getIonwm();
    PsmAddress	elt, elt2;
    ClProtocol	clpbuf;
    VInduct		*vduct;


    setupBuffer();
    if (!sdr_begin_xn(sdr)) {
        __android_log_write(ANDROID_LOG_ERROR, "List", "SDR invalid");
        jstring retVal = (*env).NewStringUTF("");
        deleteBuffer();
        return retVal;
    }

    OBJ_POINTER(ClProtocol, clp);

    for (elt = sdr_list_first(sdr, (getBpConstants())->protocols); elt;
         elt = sdr_list_next(sdr, elt))
    {

        char cliCmdBuffer[SDRSTRING_BUFSZ];

        sdr_read(sdr, (char *) &clpbuf,
                 sdr_list_data(sdr, elt), sizeof(ClProtocol));

        for (elt2 = sm_list_first(ionwm, (getBpVdb())->inducts); elt2;
             elt2 = sm_list_next(ionwm, elt2))
        {
            vduct = (VInduct *) psp(ionwm, sm_list_data(ionwm, elt2));
            if (strcmp(vduct->protocolName, clpbuf.name) == 0)
            {
                OBJ_POINTER(Induct, duct);
                OBJ_POINTER(ClProtocol, clp);
                GET_OBJ_POINTER(sdr, Induct, duct, sdr_list_data(sdr, vduct->inductElt));
                GET_OBJ_POINTER(sdr, ClProtocol, clp, duct->protocol);

                if (sdr_string_read(sdr, cliCmdBuffer, duct->cliCmd) < 0)
                {
                    cliCmdBuffer[0] = '-';
                    cliCmdBuffer[1] = '\0';
                }

                appendData(clp->name);
                appendData(" ");
                appendData(duct->name);
                appendData(" ");
                appendData(cliCmdBuffer);
                appendData(" ");
                if (vduct->cliPid > 0 && sm_TaskExists(vduct->cliPid)) {
                    appendData("true");
                }
                else {
                    appendData("false");
                }

                appendNewLine();
            }
        }
    }

    sdr_exit_xn(sdr);

    jstring retVal = (*env).NewStringUTF(buffer->c_str());

    deleteBuffer();

    return retVal;
}

JNIEXPORT jstring JNICALL
Java_gov_nasa_jpl_iondtn_backend_NativeAdapter_getOutductListION(JNIEnv *env,
                                                                jobject instance) {

    if (ionAttach() != 0) {
        __android_log_write(ANDROID_LOG_ERROR, "List", "Attach failed!");
        jstring retVal = (*env).NewStringUTF("");
        deleteBuffer();
        return retVal;
    }

    Sdr		sdr = getIonsdr();
    PsmPartition	ionwm = getIonwm();
    PsmAddress	elt, elt2;
    ClProtocol	clpbuf;
    VOutduct		*vduct;


    setupBuffer();
    if (!sdr_begin_xn(sdr)) {
        __android_log_write(ANDROID_LOG_ERROR, "List", "SDR invalid");
        jstring retVal = (*env).NewStringUTF("");
        deleteBuffer();
        return retVal;
    }

    OBJ_POINTER(ClProtocol, clp);

    for (elt = sdr_list_first(sdr, (getBpConstants())->protocols); elt;
         elt = sdr_list_next(sdr, elt))
    {

        char cloCmdBuffer[SDRSTRING_BUFSZ];

        sdr_read(sdr, (char *) &clpbuf,
                 sdr_list_data(sdr, elt), sizeof(ClProtocol));

        for (elt2 = sm_list_first(ionwm, (getBpVdb())->outducts); elt2;
             elt2 = sm_list_next(ionwm, elt2))
        {
            vduct = (VOutduct *) psp(ionwm, sm_list_data(ionwm, elt2));
            if (strcmp(vduct->protocolName, clpbuf.name) == 0)
            {
                OBJ_POINTER(Outduct, duct);
                OBJ_POINTER(ClProtocol, clp);
                GET_OBJ_POINTER(sdr, Outduct, duct, sdr_list_data(sdr,
                                                                 vduct->outductElt));
                GET_OBJ_POINTER(sdr, ClProtocol, clp, duct->protocol);

                if (sdr_string_read(sdr, cloCmdBuffer, duct->cloCmd) < 0)
                {
                    cloCmdBuffer[0] = '-';
                    cloCmdBuffer[1] = '\0';
                }

                appendData(clp->name);
                appendData(" ");
                appendData(duct->name);
                appendData(" ");
                appendData(cloCmdBuffer);
                appendData(" ");
                appendInt(duct->maxPayloadLen);
                appendData(" ");

                if (vduct->hasThread) {
                    appendData("true");
                }
                else {
                    appendData("false");
                }

                appendNewLine();
            }
        }
    }

    sdr_exit_xn(sdr);

    jstring retVal = (*env).NewStringUTF(buffer->c_str());

    deleteBuffer();

    return retVal;
}

JNIEXPORT jstring JNICALL
Java_gov_nasa_jpl_iondtn_backend_NativeAdapter_getBabRuleListION(JNIEnv *env,
                                                                 jobject instance) {

    if (secAttach() != 0) {
        __android_log_write(ANDROID_LOG_ERROR, "List", "Attach failed!");
        jstring retVal = (*env).NewStringUTF("");
        deleteBuffer();
        return retVal;
    }

    Sdr	sdr = getIonsdr();
    OBJ_POINTER(SecDB, db);
    Object	elt;
    Object	obj;
    char	srcEidBuf[SDRSTRING_BUFSZ], destEidBuf[SDRSTRING_BUFSZ];

    GET_OBJ_POINTER(sdr, SecDB, db, getSecDbObject());

    setupBuffer();
    if (!sdr_begin_xn(sdr)) {
        __android_log_write(ANDROID_LOG_ERROR, "List", "SDR invalid");
        jstring retVal = (*env).NewStringUTF("");
        deleteBuffer();
        return retVal;
    }

    for (elt = sdr_list_first(sdr, db->bspBabRules); elt;
         elt = sdr_list_next(sdr, elt))
    {
        obj = sdr_list_data(sdr, elt);
        OBJ_POINTER(BspBabRule, rule);
        GET_OBJ_POINTER(sdr, BspBabRule, rule, obj);
        sdr_string_read(sdr, srcEidBuf, rule->senderEid);
        sdr_string_read(sdr, destEidBuf, rule->receiverEid);

        appendData(srcEidBuf);
        appendData(" ");
        appendData(destEidBuf);
        appendData(" ");
        appendData(rule->ciphersuiteName);
        appendData(" ");
        appendData(rule->keyName);
        appendNewLine();
    }

    sdr_exit_xn(sdr);

    jstring retVal = (*env).NewStringUTF(buffer->c_str());

    deleteBuffer();

    return retVal;
}

JNIEXPORT jstring JNICALL
Java_gov_nasa_jpl_iondtn_backend_NativeAdapter_getBibRuleListION(JNIEnv *env,
                                                                 jobject instance) {

    if (secAttach() != 0) {
        __android_log_write(ANDROID_LOG_ERROR, "List", "Attach failed!");
        jstring retVal = (*env).NewStringUTF("");
        deleteBuffer();
        return retVal;
    }

    Sdr	sdr = getIonsdr();
    OBJ_POINTER(SecDB, db);
    Object	elt;
    Object	obj;
    char	srcEidBuf[SDRSTRING_BUFSZ], destEidBuf[SDRSTRING_BUFSZ];

    GET_OBJ_POINTER(sdr, SecDB, db, getSecDbObject());

    setupBuffer();
    if (!sdr_begin_xn(sdr)) {
        __android_log_write(ANDROID_LOG_ERROR, "List", "SDR invalid");
        jstring retVal = (*env).NewStringUTF("");
        deleteBuffer();
        return retVal;
    }

    for (elt = sdr_list_first(sdr, db->bspBibRules); elt;
         elt = sdr_list_next(sdr, elt))
    {
        obj = sdr_list_data(sdr, elt);
        OBJ_POINTER(BspBibRule, rule);
        GET_OBJ_POINTER(sdr, BspBibRule, rule, obj);
        sdr_string_read(sdr, srcEidBuf, rule->securitySrcEid);
        sdr_string_read(sdr, destEidBuf, rule->destEid);

        appendData(srcEidBuf);
        appendData(" ");
        appendData(destEidBuf);
        appendData(" ");
        appendData(rule->ciphersuiteName);
        appendData(" ");
        appendData(rule->keyName);
        appendData(" ");
        appendInt(rule->blockTypeNbr);
        appendNewLine();
    }

    sdr_exit_xn(sdr);

    jstring retVal = (*env).NewStringUTF(buffer->c_str());

    deleteBuffer();

    return retVal;
}

JNIEXPORT jstring JNICALL
Java_gov_nasa_jpl_iondtn_backend_NativeAdapter_getBcbRuleListION(JNIEnv *env,
                                                                 jobject instance) {

    if (secAttach() != 0) {
        __android_log_write(ANDROID_LOG_ERROR, "List", "Attach failed!");
        jstring retVal = (*env).NewStringUTF("");
        deleteBuffer();
        return retVal;
    }

    Sdr	sdr = getIonsdr();
    OBJ_POINTER(SecDB, db);
    Object	elt;
    Object	obj;
    char	srcEidBuf[SDRSTRING_BUFSZ], destEidBuf[SDRSTRING_BUFSZ];

    GET_OBJ_POINTER(sdr, SecDB, db, getSecDbObject());

    setupBuffer();
    if (!sdr_begin_xn(sdr)) {
        __android_log_write(ANDROID_LOG_ERROR, "List", "SDR invalid");
        jstring retVal = (*env).NewStringUTF("");
        deleteBuffer();
        return retVal;
    }

    for (elt = sdr_list_first(sdr, db->bspBcbRules); elt;
         elt = sdr_list_next(sdr, elt))
    {
        obj = sdr_list_data(sdr, elt);
        OBJ_POINTER(BspBcbRule, rule);
        GET_OBJ_POINTER(sdr, BspBcbRule, rule, obj);
        sdr_string_read(sdr, srcEidBuf, rule->securitySrcEid);
        sdr_string_read(sdr, destEidBuf, rule->destEid);

        appendData(srcEidBuf);
        appendData(" ");
        appendData(destEidBuf);
        appendData(" ");
        appendData(rule->ciphersuiteName);
        appendData(" ");
        appendData(rule->keyName);
        appendData(" ");
        appendInt(rule->blockTypeNbr);
        appendNewLine();
    }

    sdr_exit_xn(sdr);

    jstring retVal = (*env).NewStringUTF(buffer->c_str());

    deleteBuffer();

    return retVal;
}


JNIEXPORT jstring JNICALL
Java_gov_nasa_jpl_iondtn_backend_NativeAdapter_getKeyListION(JNIEnv *env,
                                                             jobject instance) {

    if (secAttach() != 0) {
        __android_log_write(ANDROID_LOG_ERROR, "List", "Attach failed!");
        jstring retVal = (*env).NewStringUTF("");
        deleteBuffer();
        return retVal;
    }

    Sdr	sdr = getIonsdr();
    OBJ_POINTER(SecDB, db);
    Object	elt;
    Object	obj;
    char	srcEidBuf[SDRSTRING_BUFSZ], destEidBuf[SDRSTRING_BUFSZ];

    GET_OBJ_POINTER(sdr, SecDB, db, getSecDbObject());

    setupBuffer();
    if (!sdr_begin_xn(sdr)) {
        __android_log_write(ANDROID_LOG_ERROR, "List", "SDR invalid");
        jstring retVal = (*env).NewStringUTF("");
        deleteBuffer();
        return retVal;
    }

    for (elt = sdr_list_first(sdr, db->keys); elt;
         elt = sdr_list_next(sdr, elt))
    {
        obj = sdr_list_data(sdr, elt);
        OBJ_POINTER(SecKey, key);
        GET_OBJ_POINTER(sdr, SecKey, key, obj);

        appendData(key->name);
        appendData(" ");
        appendInt(key->length);
        appendNewLine();
    }

    sdr_exit_xn(sdr);

    jstring retVal = (*env).NewStringUTF(buffer->c_str());

    deleteBuffer();

    return retVal;
}

}