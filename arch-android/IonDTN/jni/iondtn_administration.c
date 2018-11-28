/*
 *  ION JNI interface.
 */
#include <jni.h>
#include "ionsec.h"
#include <android/log.h>
#include "platform_sm.h"
#include <bpP.h>


extern void	executeDelete(int tokenCount, char **tokens);
extern void	executeAdd(int tokenCount, char **tokens);

static int ipnd_pid = -1;

#define	ION_NODE_NBR	19
#define LOG_ADMIN_TAG "ION_ADM"

JNIEnv *local_env;

JNIEXPORT jint JNICALL
Java_gov_nasa_jpl_demogui_agents_NodeAdministratorAgent_addPibRule(JNIEnv *env, jclass type,
                                                           jstring sourceEID_,
                                                           jstring destinationEID_,
                                                           jint blockTypeNumber,
                                                           jstring ciphersuiteName_,
                                                           jstring keyName_) {
    int return_val;
    const char *sourceEID = (*env)->GetStringUTFChars(env, sourceEID_, 0);
    const char *destinationEID = (*env)->GetStringUTFChars(env, destinationEID_, 0);
    const char *ciphersuiteName = (*env)->GetStringUTFChars(env, ciphersuiteName_, 0);
    const char *keyName = (*env)->GetStringUTFChars(env, keyName_, 0);

    return_val = sec_addBspBibRule((char *) sourceEID,
                                   (char *) destinationEID,
                                   blockTypeNumber,
                                   (char *) ciphersuiteName,
                                   (char *) keyName);

    __android_log_print(ANDROID_LOG_DEBUG, LOG_ADMIN_TAG, "Added BSP PIB rule with "
            "return code %d", return_val);

    (*env)->ReleaseStringUTFChars(env, sourceEID_, sourceEID);
    (*env)->ReleaseStringUTFChars(env, destinationEID_, destinationEID);
    (*env)->ReleaseStringUTFChars(env, ciphersuiteName_, ciphersuiteName);
    (*env)->ReleaseStringUTFChars(env, keyName_, keyName);

    return return_val;
}

JNIEXPORT jint JNICALL
Java_gov_nasa_jpl_demogui_agents_NodeAdministratorAgent_removePibRule(JNIEnv *env, jclass type,
                                                              jstring sourceEID_,
                                                              jstring destinationEID_,
                                                              jint blockTypeNumber) {
    int return_val;
    const char *sourceEID = (*env)->GetStringUTFChars(env, sourceEID_, 0);
    const char *destinationEID = (*env)->GetStringUTFChars(env, destinationEID_, 0);

    return_val = sec_removeBspBibRule((char *) sourceEID,
                                   (char *) destinationEID,
                                   blockTypeNumber);

    __android_log_print(ANDROID_LOG_DEBUG, LOG_ADMIN_TAG, "Removed BSP PIB rule with "
            "return code %d", return_val);

    (*env)->ReleaseStringUTFChars(env, sourceEID_, sourceEID);
    (*env)->ReleaseStringUTFChars(env, destinationEID_, destinationEID);

    return return_val;
}

JNIEXPORT jboolean JNICALL
Java_gov_nasa_jpl_iondtn_backend_NativeAdapter_initION(JNIEnv *env, jobject
instance, jstring absoluteConfigPath_) {
    const char *absoluteConfigPath = (*env)->GetStringUTFChars(env, absoluteConfigPath_, 0);
    int nodenbr = 1;
    char	cmd[80];
    int	count;

    /* Change working directory to sdcard/ion where config files should
      * be stored.
     */
    int c = chdir(absoluteConfigPath);
    if(c < 0)
    {
        __android_log_print(ANDROID_LOG_ERROR, "IonConfig",
                            "Didn't change dir to %s",
                            absoluteConfigPath);
    }

    sm_ipc_init();

    __android_log_write(ANDROID_LOG_DEBUG, "INIT", "Starting init.");

    /* Run ionadmin. */
    isprintf(cmd, sizeof cmd, "ionadmin %s/node.ionrc", absoluteConfigPath);
    pseudoshell(cmd);
    snooze(5);

    count = 5;
    while (rfx_system_is_started() == 0)
    {
        snooze(1);
        count--;
        if (count == 0)
        {
            __android_log_write(ANDROID_LOG_ERROR, "INIT", "Could not start. RFX hang up.");
            return 0;
        }
    }

    __android_log_write(ANDROID_LOG_DEBUG, "INIT", "ionadmin finished");

    // Check if ionsecrc file exists, if yes, execute ionsecadmin
    isprintf(cmd, sizeof cmd, "%s/node.ionsecrc", absoluteConfigPath);
    if(access(cmd, F_OK) != -1) {
        /* Run ionsecadmin. */
        isprintf(cmd, sizeof cmd, "ionsecadmin %s/node.ionsecrc", absoluteConfigPath);
        pseudoshell(cmd);
        snooze(5);
    }

    __android_log_write(ANDROID_LOG_DEBUG, "INIT", "ionsecadmin finished");


    /*	Now start the Bundle Protocol agent.			*/

    isprintf(cmd, sizeof cmd, "bpadmin %s/node.bprc", absoluteConfigPath);
    pseudoshell(cmd);
    count = 5;
    snooze(5);
    while (bp_agent_is_started() == 0)
    {
        snooze(1);
        count--;
        if (count == 0)
        {
            __android_log_write(ANDROID_LOG_ERROR, "INIT", "BP start hang up.");
            return 0;
        }
    }

    __android_log_write(ANDROID_LOG_DEBUG, "INIT", "bpadmin finished");

    // Check if ltprc file exists, if yes, execute ltpadmin
    isprintf(cmd, sizeof cmd, "%s/node.ltprc", absoluteConfigPath);
    if(access(cmd, F_OK) != -1) {
        /* Run ltpadmin. */
        isprintf(cmd, sizeof cmd, "ltpadmin %s/node.ltprc",
                 absoluteConfigPath);
        pseudoshell(cmd);
        snooze(1);
    }

    // Check if cfdprc file exists, if yes, execute cfdpadmin
    isprintf(cmd, sizeof cmd, "%s/node.cfdprc", absoluteConfigPath);
    if(access(cmd, F_OK) != -1) {
        /* Run cfdpadmin. */
        isprintf(cmd, sizeof cmd, "cfdpadmin %s/node.cfdprc",
                 absoluteConfigPath);
        pseudoshell(cmd);
        snooze(1);
    }

    // Check if dtn2rc file exists, if yes, execute dtn2admin
    isprintf(cmd, sizeof cmd, "%s/node.dtn2rc", absoluteConfigPath);
    if(access(cmd, F_OK) != -1) {
        /* Run dtn2admin. */
        isprintf(cmd, sizeof cmd, "dtn2admin %s/node.dtn2rc",
                 absoluteConfigPath);
        pseudoshell(cmd);
        snooze(1);
    }

    // Check if dtpcrc file exists, if yes, execute dtpcadmin
    isprintf(cmd, sizeof cmd, "%s/node.dtpcrc", absoluteConfigPath);
    if(access(cmd, F_OK) != -1) {
        /* Run ionsecadmin. */
        isprintf(cmd, sizeof cmd, "dtpcadmin %s/node.dtpcrc",
                 absoluteConfigPath);
        pseudoshell(cmd);
        snooze(1);
    }

    // Check if ltprc file exists, if yes, execute acsadmin
    isprintf(cmd, sizeof cmd, "%s/node.ltprc", absoluteConfigPath);
    if(access(cmd, F_OK) != -1) {
        /* Run ionsecadmin. */
        isprintf(cmd, sizeof cmd, "ltpadmin %s/node.ionsecrc",
                 absoluteConfigPath);
        pseudoshell(cmd);
        snooze(1);
    }

    // Check if imcrc file exists, if yes, execute imcadmin
    isprintf(cmd, sizeof cmd, "%s/node.imcrc", absoluteConfigPath);
    if(access(cmd, F_OK) != -1) {
        /* Run imcadmin. */
        isprintf(cmd, sizeof cmd, "imcadmin %s/node.imcrc",
                 absoluteConfigPath);
        pseudoshell(cmd);
        snooze(1);
    }

    // Check if bssrc file exists, if yes, execute bssadmin
    isprintf(cmd, sizeof cmd, "%s/node.bssrc", absoluteConfigPath);
    if(access(cmd, F_OK) != -1) {
        /* Run bssadmin. */
        isprintf(cmd, sizeof cmd, "bssadmin %s/node.bssrc",
                 absoluteConfigPath);
        pseudoshell(cmd);
        snooze(1);
    }

    // Check if ipnrc file exists, if yes, execute ipnadmin
    isprintf(cmd, sizeof cmd, "%s/node.ipnrc", absoluteConfigPath);
    if(access(cmd, F_OK) != -1) {
        /* Run ipnadmin. */
        isprintf(cmd, sizeof cmd, "ipnadmin %s/node.ipnrc",
                 absoluteConfigPath);
        pseudoshell(cmd);
        snooze(1);
    }

    // Check if dtn2rc file exists, if yes, execute dtn2admin
    isprintf(cmd, sizeof cmd, "%s/node.dtn2rc", absoluteConfigPath);
    if(access(cmd, F_OK) != -1) {
        /* Run dtn2admin. */
        isprintf(cmd, sizeof cmd, "dtn2admin %s/node.dtn2rc",
                 absoluteConfigPath);
        pseudoshell(cmd);
        snooze(1);
    }

    // Check if ipndrc file exists, if yes, execute ipndadmin
    isprintf(cmd, sizeof cmd, "%s/node.ipndrc", absoluteConfigPath);
    if(access(cmd, F_OK) != -1) {
        /* Run ipndadmin. */
        isprintf(cmd, sizeof cmd, "ipndadmin %s/node.ipndrc",
                 absoluteConfigPath);
        ipnd_pid = pseudoshell(cmd);
        snooze(1);
    }

    return 1;
}

/*
 * Stops running nodes.
 */
JNIEXPORT jstring JNICALL
Java_gov_nasa_jpl_iondtn_backend_NativeAdapter_stopION(JNIEnv
                                                                    *env,
                                                            jobject this)
{

    char	cmd[80];
    char	*result = "Node stopped";
    int 	count;

    // Ensure that all receiving threads that are using ION functionality got
    // notice of the shutdown and are no longer dependent on it --> prevents
    // crashes
    snooze(5);

    if (ipnd_pid != -1) {
        sm_TaskKill(ipnd_pid, SIGTERM);
        __android_log_write(ANDROID_LOG_DEBUG, "STOP", "Stopped the IPND threads");
        ipnd_pid = -1;
    }


    pseudoshell("bpadmin .");

    count = 10;
    while (bp_agent_is_started() == 1)
    {
        snooze(1);
        count--;
        if (count == 0)
        {
            __android_log_write(ANDROID_LOG_ERROR, "STOP", "BP didn't close "
                    "properly");
            break;
        }
    }

    snooze(1);
    pseudoshell("ionadmin .");
    count = 5;
    while (rfx_system_is_started() == 1)
    {
        snooze(1);
        count--;
        if (count == 0)
        {
            __android_log_write(ANDROID_LOG_ERROR, "STOP", "RFX didn't close "
                    "properly");
            break;
        }
    }

    snooze(1);

    sm_TasksClear();

    return (*env)->NewStringUTF(env, result);

}

JNIEXPORT jboolean JNICALL
Java_gov_nasa_jpl_iondtn_gui_AddEditDialogFragments_ContactDialogFragment_deleteContactION(
        JNIEnv *env, jobject instance, jstring timeFrom_, jstring nodeFrom_,
        jstring nodeTo_) {
    const char *timeFrom = (*env)->GetStringUTFChars(env, timeFrom_, 0);
    const char *nodeFrom = (*env)->GetStringUTFChars(env, nodeFrom_, 0);
    const char *nodeTo = (*env)->GetStringUTFChars(env, nodeTo_, 0);

    static const int tokenCount = 5;

    char* token[tokenCount] = {"r", "contact", (char*)timeFrom,
                               (char*)nodeFrom, (char*)nodeTo};

    executeDelete(tokenCount, (char**)&token);

    (*env)->ReleaseStringUTFChars(env, timeFrom_, timeFrom);
    (*env)->ReleaseStringUTFChars(env, nodeFrom_, nodeFrom);
    (*env)->ReleaseStringUTFChars(env, nodeTo_, nodeTo);

    return 1;
}

JNIEXPORT jboolean JNICALL
Java_gov_nasa_jpl_iondtn_gui_AddEditDialogFragments_ContactDialogFragment_addContactION(
        JNIEnv *env, jobject instance, jstring nodeFrom_, jstring nodeTo_,
        jstring timeFrom_, jstring timeTo_, jstring confidence_,
        jstring rate_) {
    const char *nodeFrom = (*env)->GetStringUTFChars(env, nodeFrom_, 0);
    const char *nodeTo = (*env)->GetStringUTFChars(env, nodeTo_, 0);
    const char *timeFrom = (*env)->GetStringUTFChars(env, timeFrom_, 0);
    const char *timeTo = (*env)->GetStringUTFChars(env, timeTo_, 0);
    const char *confidence = (*env)->GetStringUTFChars(env, confidence_, 0);
    const char *rate = (*env)->GetStringUTFChars(env, rate_, 0);

    static const int tokenCount = 8;

    char* token[tokenCount] = {"a", "contact", (char*)timeFrom, (char*)timeTo,
                               (char*)nodeFrom, (char*)nodeTo, (char*)rate,
                               (char*)confidence};

    executeAdd(tokenCount, (char**)&token);

    (*env)->ReleaseStringUTFChars(env, nodeFrom_, nodeFrom);
    (*env)->ReleaseStringUTFChars(env, nodeTo_, nodeTo);
    (*env)->ReleaseStringUTFChars(env, timeFrom_, timeFrom);
    (*env)->ReleaseStringUTFChars(env, timeTo_, timeTo);
    (*env)->ReleaseStringUTFChars(env, confidence_, confidence);
    (*env)->ReleaseStringUTFChars(env, rate_, rate);

    return 1;
}

JNIEXPORT jboolean JNICALL
Java_gov_nasa_jpl_iondtn_gui_AddEditDialogFragments_RangeDialogFragment_deleteRangeION(
        JNIEnv *env, jobject instance, jstring timeFrom_, jstring nodeFrom_,
        jstring nodeTo_) {
    const char *timeFrom = (*env)->GetStringUTFChars(env, timeFrom_, 0);
    const char *nodeFrom = (*env)->GetStringUTFChars(env, nodeFrom_, 0);
    const char *nodeTo = (*env)->GetStringUTFChars(env, nodeTo_, 0);

    static const int tokenCount = 5;

    char* token[tokenCount] = {"r", "range", (char*)timeFrom,
                               (char*)nodeFrom, (char*)nodeTo};

    executeDelete(tokenCount, (char**)&token);

    (*env)->ReleaseStringUTFChars(env, timeFrom_, timeFrom);
    (*env)->ReleaseStringUTFChars(env, nodeFrom_, nodeFrom);
    (*env)->ReleaseStringUTFChars(env, nodeTo_, nodeTo);

    return 1;
}

JNIEXPORT jboolean JNICALL
Java_gov_nasa_jpl_iondtn_gui_AddEditDialogFragments_RangeDialogFragment_addRangeION(
        JNIEnv *env, jobject instance, jstring nodeFrom_, jstring nodeTo_,
        jstring timeFrom_, jstring timeTo_, jstring owlt_) {
    const char *nodeFrom = (*env)->GetStringUTFChars(env, nodeFrom_, 0);
    const char *nodeTo = (*env)->GetStringUTFChars(env, nodeTo_, 0);
    const char *timeFrom = (*env)->GetStringUTFChars(env, timeFrom_, 0);
    const char *timeTo = (*env)->GetStringUTFChars(env, timeTo_, 0);
    const char *owlt = (*env)->GetStringUTFChars(env, owlt_, 0);

    static const int tokenCount = 7;

    char* token[tokenCount] = {"a", "range", (char*)timeFrom, (char*)timeTo,
                               (char*)nodeFrom, (char*)nodeTo, (char*)owlt};

    executeAdd(tokenCount, (char**)&token);

    (*env)->ReleaseStringUTFChars(env, nodeFrom_, nodeFrom);
    (*env)->ReleaseStringUTFChars(env, nodeTo_, nodeTo);
    (*env)->ReleaseStringUTFChars(env, timeFrom_, timeFrom);
    (*env)->ReleaseStringUTFChars(env, timeTo_, timeTo);
    (*env)->ReleaseStringUTFChars(env, owlt_, owlt);

    return 1;
}

JNIEXPORT jboolean JNICALL
Java_gov_nasa_jpl_iondtn_gui_AddEditDialogFragments_SchemeDialogFragment_deleteSchemeION(
        JNIEnv *env, jobject instance, jstring scheme_) {
    const char *scheme = (*env)->GetStringUTFChars(env, scheme_, 0);

    if (bp_attach() < 0)
    {
        return 0;
    }

    jboolean retVal = (jboolean)removeScheme((char*)scheme);

    (*env)->ReleaseStringUTFChars(env, scheme_, scheme);

    return retVal;
}

JNIEXPORT jboolean JNICALL
Java_gov_nasa_jpl_iondtn_gui_AddEditDialogFragments_SchemeDialogFragment_addSchemeION(
        JNIEnv *env, jobject instance, jstring name_, jstring fwdCmd_,
        jstring admAppCmd_) {
    const char *name = (*env)->GetStringUTFChars(env, name_, 0);
    const char *fwdCmd = (*env)->GetStringUTFChars(env, fwdCmd_, 0);
    const char *admAppCmd = (*env)->GetStringUTFChars(env, admAppCmd_, 0);

    if (bp_attach() < 0)
    {
        return 1;
    }

    jboolean retVal = (jboolean)addScheme((char*)name,
                                          (char*)fwdCmd,
                                          (char*)admAppCmd);

    (*env)->ReleaseStringUTFChars(env, name_, name);
    (*env)->ReleaseStringUTFChars(env, fwdCmd_, fwdCmd);
    (*env)->ReleaseStringUTFChars(env, admAppCmd_, admAppCmd);

    return retVal;
}

JNIEXPORT jboolean JNICALL
Java_gov_nasa_jpl_iondtn_gui_AddEditDialogFragments_SchemeDialogFragment_updateSchemeION(
        JNIEnv *env, jobject instance, jstring name_, jstring fwdCmd_,
        jstring admAppCmd_) {
    const char *name = (*env)->GetStringUTFChars(env, name_, 0);
    const char *fwdCmd = (*env)->GetStringUTFChars(env, fwdCmd_, 0);
    const char *admAppCmd = (*env)->GetStringUTFChars(env, admAppCmd_, 0);

    if (bp_attach() < 0)
    {
        return 1;
    }

    jboolean retVal = (jboolean)updateScheme((char*)name,
                                             (char*)fwdCmd,
                                             (char*) admAppCmd);

    (*env)->ReleaseStringUTFChars(env, name_, name);
    (*env)->ReleaseStringUTFChars(env, fwdCmd_, fwdCmd);
    (*env)->ReleaseStringUTFChars(env, admAppCmd_, admAppCmd);

    return retVal;
}

JNIEXPORT jboolean JNICALL
Java_gov_nasa_jpl_iondtn_gui_AddEditDialogFragments_EidDialogFragment_deleteEidION(
        JNIEnv *env, jobject instance, jstring eid_) {
    const char *eid = (*env)->GetStringUTFChars(env, eid_, 0);

    if (bp_attach() < 0)
    {
        return 1;
    }

    jboolean retVal = (jboolean)removeEndpoint((char*)eid);

    (*env)->ReleaseStringUTFChars(env, eid_, eid);

    return retVal;
}

JNIEXPORT jboolean JNICALL
Java_gov_nasa_jpl_iondtn_gui_AddEditDialogFragments_EidDialogFragment_addEidION(
        JNIEnv *env, jobject instance, jstring eid_, jboolean behavior) {
    const char *eid = (*env)->GetStringUTFChars(env, eid_, 0);

    BpRecvRule	rule;


    if (bp_attach() < 0)
    {
        return 1;
    }

    if (behavior)
    {
        rule = DiscardBundle;
    }
    else
    {
        rule = EnqueueBundle;
    }

    jboolean retVal = (jboolean)addEndpoint((char*)eid, rule, NULL);

    (*env)->ReleaseStringUTFChars(env, eid_, eid);

    return retVal;
}

JNIEXPORT jboolean JNICALL
Java_gov_nasa_jpl_iondtn_gui_AddEditDialogFragments_EidDialogFragment_updateEidION(
        JNIEnv *env, jobject instance, jstring eid_, jboolean behavior) {
    const char *eid = (*env)->GetStringUTFChars(env, eid_, 0);

    BpRecvRule	rule;


    if (bp_attach() < 0)
    {
        return 1;
    }

    if (behavior)
    {
        rule = DiscardBundle;
    }
    else
    {
        rule = EnqueueBundle;
    }

    jboolean retVal = (jboolean)updateEndpoint((char*)eid, rule, NULL);

    (*env)->ReleaseStringUTFChars(env, eid_, eid);

    return retVal;
}

JNIEXPORT jboolean JNICALL
Java_gov_nasa_jpl_iondtn_gui_AddEditDialogFragments_ProtocolDialogFragment_deleteProtocolION(
        JNIEnv *env, jobject instance, jstring identifier_) {
    const char *identifier = (*env)->GetStringUTFChars(env, identifier_, 0);

    if (bp_attach() < 0)
    {
        return 0;
    }

    jboolean retVal = (jboolean)removeProtocol((char*)identifier);

    (*env)->ReleaseStringUTFChars(env, identifier_, identifier);

    return retVal;
}

JNIEXPORT jboolean JNICALL
Java_gov_nasa_jpl_iondtn_gui_AddEditDialogFragments_ProtocolDialogFragment_addProtocolION(
        JNIEnv *env, jobject instance, jstring identifier_, jint payload,
        jint overhead, jint protocolClass) {
    const char *identifier = (*env)->GetStringUTFChars(env, identifier_, 0);

    if (bp_attach() < 0)
    {
        return 0;
    }

    jboolean retVal = (jboolean)addProtocol((char*)identifier, payload,
                                            overhead, protocolClass);

    (*env)->ReleaseStringUTFChars(env, identifier_, identifier);

    return retVal;
}

JNIEXPORT jboolean JNICALL
Java_gov_nasa_jpl_iondtn_gui_AddEditDialogFragments_ProtocolDialogFragment_updateProtocolION(
        JNIEnv *env, jobject instance, jstring identifier_, jint payload,
        jint overhead, jint protocolClass) {
    const char *identifier = (*env)->GetStringUTFChars(env, identifier_, 0);

    if (bp_attach() < 0)
    {
        return 0;
    }

    if (!removeProtocol((char*)identifier)) {
        (*env)->ReleaseStringUTFChars(env, identifier_, identifier);
        return 0;
    }

    jboolean retVal = (jboolean)addProtocol((char*)identifier, payload,
                                            overhead, protocolClass);

    (*env)->ReleaseStringUTFChars(env, identifier_, identifier);

    return retVal;
}

JNIEXPORT jboolean JNICALL
Java_gov_nasa_jpl_iondtn_gui_AddEditDialogFragments_InductDialogFragment_addInductION(
        JNIEnv *env, jobject instance, jstring identifier_, jstring protocol_,
        jstring cmd_) {
    const char *identifier = (*env)->GetStringUTFChars(env, identifier_, 0);
    const char *protocol = (*env)->GetStringUTFChars(env, protocol_, 0);
    const char *cmd = (*env)->GetStringUTFChars(env, cmd_, 0);

    if (bp_attach() < 0)
    {
        return 0;
    }

    jboolean retVal = (jboolean)addInduct((char*)protocol,
                                          (char*)identifier,
                                          (char*)cmd);

    (*env)->ReleaseStringUTFChars(env, identifier_, identifier);
    (*env)->ReleaseStringUTFChars(env, protocol_, protocol);
    (*env)->ReleaseStringUTFChars(env, cmd_, cmd);

    return retVal;
}

JNIEXPORT jboolean JNICALL
Java_gov_nasa_jpl_iondtn_gui_AddEditDialogFragments_InductDialogFragment_deleteInductION(
        JNIEnv *env, jobject instance, jstring identifier_, jstring protocol_) {
    const char *identifier = (*env)->GetStringUTFChars(env, identifier_, 0);
    const char *protocol = (*env)->GetStringUTFChars(env, protocol_, 0);

    if (bp_attach() < 0)
    {
        return 0;
    }

    jboolean retVal = (jboolean)removeInduct((char*)protocol,
                                          (char*)identifier);

    (*env)->ReleaseStringUTFChars(env, identifier_, identifier);
    (*env)->ReleaseStringUTFChars(env, protocol_, protocol);

    return retVal;
}

JNIEXPORT jboolean JNICALL
Java_gov_nasa_jpl_iondtn_gui_AddEditDialogFragments_InductDialogFragment_updateInductION(
        JNIEnv *env, jobject instance, jstring identifier_, jstring protocol_,
        jstring cmd_) {
    const char *identifier = (*env)->GetStringUTFChars(env, identifier_, 0);
    const char *protocol = (*env)->GetStringUTFChars(env, protocol_, 0);
    const char *cmd = (*env)->GetStringUTFChars(env, cmd_, 0);

    if (bp_attach() < 0)
    {
        return 0;
    }

    jboolean retVal = (jboolean)updateInduct((char*)protocol,
                                          (char*)identifier,
                                          (char*)cmd);

    (*env)->ReleaseStringUTFChars(env, identifier_, identifier);
    (*env)->ReleaseStringUTFChars(env, protocol_, protocol);
    (*env)->ReleaseStringUTFChars(env, cmd_, cmd);

    return retVal;
}

JNIEXPORT jboolean JNICALL
Java_gov_nasa_jpl_iondtn_gui_AddEditDialogFragments_OutductDialogFragment_deleteOutductION(
        JNIEnv *env, jobject instance, jstring identifier_, jstring protocol_) {
    const char *identifier = (*env)->GetStringUTFChars(env, identifier_, 0);
    const char *protocol = (*env)->GetStringUTFChars(env, protocol_, 0);

    if (bp_attach() < 0)
    {
        return 0;
    }

    jboolean retVal = (jboolean)removeOutduct((char*)protocol,
                                             (char*)identifier);

    (*env)->ReleaseStringUTFChars(env, identifier_, identifier);
    (*env)->ReleaseStringUTFChars(env, protocol_, protocol);

    return retVal;
}

JNIEXPORT jboolean JNICALL
Java_gov_nasa_jpl_iondtn_gui_AddEditDialogFragments_OutductDialogFragment_addOutductION(
        JNIEnv *env, jobject instance, jstring identifier_, jstring protocol_,
        jstring cmd_, jint maxPayloadLength) {
    const char *identifier = (*env)->GetStringUTFChars(env, identifier_, 0);
    const char *protocol = (*env)->GetStringUTFChars(env, protocol_, 0);
    const char *cmd = (*env)->GetStringUTFChars(env, cmd_, 0);

    if (bp_attach() < 0)
    {
        return 0;
    }

    jboolean retVal = (jboolean)addOutduct((char*)protocol,
                                              (char*)identifier,
                                              (char*)cmd,
                                              maxPayloadLength);

    (*env)->ReleaseStringUTFChars(env, identifier_, identifier);
    (*env)->ReleaseStringUTFChars(env, protocol_, protocol);
    (*env)->ReleaseStringUTFChars(env, cmd_, cmd);

    return retVal;
}

JNIEXPORT jboolean JNICALL
Java_gov_nasa_jpl_iondtn_gui_AddEditDialogFragments_OutductDialogFragment_updateOutductION(
        JNIEnv *env, jobject instance, jstring identifier_, jstring protocol_,
        jstring cmd_, jint maxPayloadLength) {
    const char *identifier = (*env)->GetStringUTFChars(env, identifier_, 0);
    const char *protocol = (*env)->GetStringUTFChars(env, protocol_, 0);
    const char *cmd = (*env)->GetStringUTFChars(env, cmd_, 0);

    if (bp_attach() < 0)
    {
        return 0;
    }

    jboolean retVal = (jboolean)updateOutduct((char*)protocol,
                                             (char*)identifier,
                                             (char*)cmd,
                                                maxPayloadLength);

    (*env)->ReleaseStringUTFChars(env, identifier_, identifier);
    (*env)->ReleaseStringUTFChars(env, protocol_, protocol);
    (*env)->ReleaseStringUTFChars(env, cmd_, cmd);

    return retVal;
}

JNIEXPORT jboolean JNICALL
Java_gov_nasa_jpl_iondtn_gui_AddEditDialogFragments_OutductDialogFragment_startOutductION(
        JNIEnv *env, jobject instance, jstring identifier_, jstring protocol_) {
    const char *identifier = (*env)->GetStringUTFChars(env, identifier_, 0);
    const char *protocol = (*env)->GetStringUTFChars(env, protocol_, 0);

    if (bp_attach() < 0)
    {
        return 0;
    }

    jboolean retVal = (jboolean)bpStartOutduct((char*)identifier,
                                              (char*)protocol);

    Sdr		bpSdr = getIonsdr();
    VOutduct		*vduct;
    PsmAddress	vductElt;

    CHKZERO(sdr_begin_xn(bpSdr));	/*	Just to lock memory.	*/
    findOutduct((char*)identifier, (char*)protocol, &vduct, &vductElt);

    sdr_exit_xn(bpSdr);	/*	Unlock memory.			*/
    (*env)->ReleaseStringUTFChars(env, identifier_, identifier);
    (*env)->ReleaseStringUTFChars(env, protocol_, protocol);

    if (vductElt == 0)	/*	This is an unknown duct.	*/
    {
        return 0;
    }
    else
    {
        if (vduct->cloPid > 0 && sm_TaskExists(vduct->cloPid)) {
            return 1;
        }
        else {
            return 0;
        }
    }
}

JNIEXPORT void JNICALL
Java_gov_nasa_jpl_iondtn_gui_AddEditDialogFragments_OutductDialogFragment_stopOutductION(
        JNIEnv *env, jobject instance, jstring identifier_, jstring protocol_) {
    const char *identifier = (*env)->GetStringUTFChars(env, identifier_, 0);
    const char *protocol = (*env)->GetStringUTFChars(env, protocol_, 0);

    if (bp_attach() < 0)
    {
        return;
    }

    bpStopOutduct((char*)identifier, (char*)protocol);

    (*env)->ReleaseStringUTFChars(env, identifier_, identifier);
    (*env)->ReleaseStringUTFChars(env, protocol_, protocol);
}

JNIEXPORT jboolean JNICALL
Java_gov_nasa_jpl_iondtn_gui_AddEditDialogFragments_InductDialogFragment_startInductION(
        JNIEnv *env, jobject instance, jstring identifier_, jstring protocol_) {
    const char *identifier = (*env)->GetStringUTFChars(env, identifier_, 0);
    const char *protocol = (*env)->GetStringUTFChars(env, protocol_, 0);

    if (bp_attach() < 0)
    {
        return 0;
    }

    bpStartInduct((char*)identifier,
                  (char*)protocol);


    Sdr		bpSdr = getIonsdr();
    VInduct		*vduct;
    PsmAddress	vductElt;

    CHKZERO(sdr_begin_xn(bpSdr));	/*	Just to lock memory.	*/
    findInduct((char*)identifier, (char*)protocol, &vduct, &vductElt);

    (*env)->ReleaseStringUTFChars(env, identifier_, identifier);
    (*env)->ReleaseStringUTFChars(env, protocol_, protocol);

    jboolean retVal;
    if (vductElt == 0)	/*	This is an unknown duct.	*/
    {
        retVal = 0;
    }
    else
    {
        if (vduct->cliPid > 0 && sm_TaskExists(vduct->cliPid)) {
            retVal = 1;
        }
        else {
            retVal = 0;
        }
    }

    sdr_exit_xn(bpSdr);	/*	Unlock memory.			*/

    return retVal;
}

JNIEXPORT void JNICALL
Java_gov_nasa_jpl_iondtn_gui_AddEditDialogFragments_InductDialogFragment_stopInductION(
        JNIEnv *env, jobject instance, jstring identifier_, jstring protocol_) {
    const char *identifier = (*env)->GetStringUTFChars(env, identifier_, 0);
    const char *protocol = (*env)->GetStringUTFChars(env, protocol_, 0);

    if (bp_attach() < 0)
    {
        return;
    }

    bpStopInduct((char*)identifier, (char*)protocol);

    (*env)->ReleaseStringUTFChars(env, identifier_, identifier);
    (*env)->ReleaseStringUTFChars(env, protocol_, protocol);
}

JNIEXPORT jboolean JNICALL
Java_gov_nasa_jpl_iondtn_gui_AddEditDialogFragments_SchemeDialogFragment_startSchemeION(
        JNIEnv *env, jobject instance, jstring identifier_) {
    const char *identifier = (*env)->GetStringUTFChars(env, identifier_, 0);

    if (bp_attach() < 0)
    {
        return 0;
    }

    bpStartScheme((char*)identifier);


    Sdr		bpSdr = getIonsdr();
    VScheme		*vschem;
    PsmAddress	vschemElt;

    CHKZERO(sdr_begin_xn(bpSdr));	/*	Just to lock memory.	*/
    findScheme((char*)identifier, &vschem, &vschemElt);

    jboolean retVal;
    if (vschemElt == 0)	/*	This is an unknown duct.	*/
    {
        retVal = 0;
    }
    else if (vschem->admAppPid > 0 && sm_TaskExists(vschem->admAppPid)) {
            retVal = 1;
    }
    else {
        retVal = 0;
    }

    (*env)->ReleaseStringUTFChars(env, identifier_, identifier);
    sdr_exit_xn(bpSdr);	/*	Unlock memory.			*/

    return retVal;
}

JNIEXPORT void JNICALL
Java_gov_nasa_jpl_iondtn_gui_AddEditDialogFragments_SchemeDialogFragment_stopSchemeION(
        JNIEnv *env, jobject instance, jstring identifier_) {
    const char *identifier = (*env)->GetStringUTFChars(env, identifier_, 0);

    bpStopScheme((char*)identifier);

    (*env)->ReleaseStringUTFChars(env, identifier_, identifier);
}

JNIEXPORT jboolean JNICALL
Java_gov_nasa_jpl_iondtn_gui_AddEditDialogFragments_BabRuleDialogFragment_deleteBabRuleION(
        JNIEnv *env, jobject instance, jstring srcEID_, jstring destEID_) {
    const char *srcEID = (*env)->GetStringUTFChars(env, srcEID_, 0);
    const char *destEID = (*env)->GetStringUTFChars(env, destEID_, 0);

    jint return_val = sec_removeBspBabRule((char *) srcEID,
                                      (char *) destEID);

    __android_log_print(ANDROID_LOG_DEBUG, LOG_ADMIN_TAG, "Removed BSP BAB rule with "
            "return code %d", return_val);

    (*env)->ReleaseStringUTFChars(env, srcEID_, srcEID);
    (*env)->ReleaseStringUTFChars(env, destEID_, destEID);

    return  (jboolean)(return_val == 1);
}

JNIEXPORT jboolean JNICALL
Java_gov_nasa_jpl_iondtn_gui_AddEditDialogFragments_BabRuleDialogFragment_addBabRuleION(
        JNIEnv *env, jobject instance, jstring srcEID_, jstring destEID_,
        jstring ciphersuiteName_, jstring keyName_) {
    const char *srcEID = (*env)->GetStringUTFChars(env, srcEID_, 0);
    const char *destEID = (*env)->GetStringUTFChars(env, destEID_, 0);
    const char *ciphersuiteName = (*env)->GetStringUTFChars(env,
                                                            ciphersuiteName_,
                                                            0);
    const char *keyName = (*env)->GetStringUTFChars(env, keyName_, 0);

    jint return_val = sec_addBspBabRule((char *) srcEID,
                                   (char *) destEID,
                                   (char *) ciphersuiteName,
                                   (char *) keyName);

    __android_log_print(ANDROID_LOG_DEBUG, LOG_ADMIN_TAG, "Added BSP BAB rule with "
            "return code %d", return_val);

    (*env)->ReleaseStringUTFChars(env, srcEID_, srcEID);
    (*env)->ReleaseStringUTFChars(env, destEID_, destEID);
    (*env)->ReleaseStringUTFChars(env, ciphersuiteName_, ciphersuiteName);
    (*env)->ReleaseStringUTFChars(env, keyName_, keyName);

    return  (jboolean)(return_val == 1);
}

JNIEXPORT jboolean JNICALL
Java_gov_nasa_jpl_iondtn_gui_AddEditDialogFragments_BibRuleDialogFragment_addBibRuleION(
        JNIEnv *env, jobject instance, jstring srcEID_, jstring destEID_,
        jstring ciphersuiteName_, jstring keyName_, jint nodeTypeNumber) {
    const char *srcEID = (*env)->GetStringUTFChars(env, srcEID_, 0);
    const char *destEID = (*env)->GetStringUTFChars(env, destEID_, 0);
    const char *ciphersuiteName = (*env)->GetStringUTFChars(env,
                                                            ciphersuiteName_,
                                                            0);
    const char *keyName = (*env)->GetStringUTFChars(env, keyName_, 0);

    jint return_val = sec_addBspBibRule((char *) srcEID,
                                        (char *) destEID,
                                        nodeTypeNumber,
                                        (char *) ciphersuiteName,
                                        (char *) keyName);

    __android_log_print(ANDROID_LOG_DEBUG, LOG_ADMIN_TAG, "Added BSP BIB rule"
            " with "
            "return code %d", return_val);

    (*env)->ReleaseStringUTFChars(env, srcEID_, srcEID);
    (*env)->ReleaseStringUTFChars(env, destEID_, destEID);
    (*env)->ReleaseStringUTFChars(env, ciphersuiteName_, ciphersuiteName);
    (*env)->ReleaseStringUTFChars(env, keyName_, keyName);

    return  (jboolean)(return_val == 1);
}

JNIEXPORT jboolean JNICALL
Java_gov_nasa_jpl_iondtn_gui_AddEditDialogFragments_BibRuleDialogFragment_deleteBibRuleION(
        JNIEnv *env, jobject instance, jstring srcEID_, jstring destEID_,
        jint nodeTypeNumber) {
    const char *srcEID = (*env)->GetStringUTFChars(env, srcEID_, 0);
    const char *destEID = (*env)->GetStringUTFChars(env, destEID_, 0);

    jint return_val = sec_removeBspBibRule((char *) srcEID,
                                           (char *) destEID,
                                            nodeTypeNumber);

    __android_log_print(ANDROID_LOG_DEBUG, LOG_ADMIN_TAG, "Removed BSP BIB "
            "rule with "
            "return code %d", return_val);

    (*env)->ReleaseStringUTFChars(env, srcEID_, srcEID);
    (*env)->ReleaseStringUTFChars(env, destEID_, destEID);

    return  (jboolean)(return_val == 1);
}

JNIEXPORT jboolean JNICALL
Java_gov_nasa_jpl_iondtn_gui_AddEditDialogFragments_BcbRuleDialogFragment_deleteBcbRuleION(
        JNIEnv *env, jobject instance, jstring srcEID_, jstring destEID_,
        jint nodeTypeNumber) {
    const char *srcEID = (*env)->GetStringUTFChars(env, srcEID_, 0);
    const char *destEID = (*env)->GetStringUTFChars(env, destEID_, 0);

    jint return_val = sec_removeBspBcbRule((char *) srcEID,
                                           (char *) destEID,
                                           nodeTypeNumber);

    __android_log_print(ANDROID_LOG_DEBUG, LOG_ADMIN_TAG, "Removed BSP BIB "
            "rule with "
            "return code %d", return_val);

    (*env)->ReleaseStringUTFChars(env, srcEID_, srcEID);
    (*env)->ReleaseStringUTFChars(env, destEID_, destEID);

    return  (jboolean)(return_val == 1);
}

JNIEXPORT jboolean JNICALL
Java_gov_nasa_jpl_iondtn_gui_AddEditDialogFragments_BcbRuleDialogFragment_addBcbRuleION(
        JNIEnv *env, jobject instance, jstring srcEID_, jstring destEID_,
        jstring ciphersuiteName_, jstring keyName_, jint nodeTypeNumber) {
    const char *srcEID = (*env)->GetStringUTFChars(env, srcEID_, 0);
    const char *destEID = (*env)->GetStringUTFChars(env, destEID_, 0);
    const char *ciphersuiteName = (*env)->GetStringUTFChars(env,
                                                            ciphersuiteName_,
                                                            0);
    const char *keyName = (*env)->GetStringUTFChars(env, keyName_, 0);

    jint return_val = sec_addBspBcbRule((char *) srcEID,
                                        (char *) destEID,
                                        nodeTypeNumber,
                                        (char *) ciphersuiteName,
                                        (char *) keyName);

    __android_log_print(ANDROID_LOG_DEBUG, LOG_ADMIN_TAG, "Added BSP BIB rule"
            " with "
            "return code %d", return_val);

    (*env)->ReleaseStringUTFChars(env, srcEID_, srcEID);
    (*env)->ReleaseStringUTFChars(env, destEID_, destEID);
    (*env)->ReleaseStringUTFChars(env, ciphersuiteName_, ciphersuiteName);
    (*env)->ReleaseStringUTFChars(env, keyName_, keyName);

    return  (jboolean)(return_val == 1);
}

JNIEXPORT jboolean JNICALL
Java_gov_nasa_jpl_iondtn_gui_AddEditDialogFragments_KeyDialogFragment_addKeyION(
        JNIEnv *env, jobject instance, jstring name_, jstring path_) {
    const char *name = (*env)->GetStringUTFChars(env, name_, 0);
    const char *path = (*env)->GetStringUTFChars(env, path_, 0);

    jint return_val = sec_addKey((char *) name,
                                 (char *) path);

    __android_log_print(ANDROID_LOG_DEBUG, LOG_ADMIN_TAG, "Added KEY"
            " with "
            "return code %d", return_val);

    (*env)->ReleaseStringUTFChars(env, name_, name);
    (*env)->ReleaseStringUTFChars(env, path_, path);

    return  (jboolean)(return_val == 1);
}

JNIEXPORT jboolean JNICALL
Java_gov_nasa_jpl_iondtn_gui_AddEditDialogFragments_KeyDialogFragment_deleteKeyION(
        JNIEnv *env, jobject instance, jstring name_) {
    const char *name = (*env)->GetStringUTFChars(env, name_, 0);

    jint return_val = sec_removeKey((char *) name);

    __android_log_print(ANDROID_LOG_DEBUG, LOG_ADMIN_TAG, "Removed KEY"
            " with "
            "return code %d", return_val);

    (*env)->ReleaseStringUTFChars(env, name_, name);

    return  (jboolean)(return_val == 1);
}

