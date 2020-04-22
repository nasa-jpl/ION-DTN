  /********************************************************
  ** Authors: Marco Bertolazzi, marco.bertolazzi3@studio.unibo.it
  **          Carlo Caini (DTNbox project supervisor), carlo.caini@unibo.it
  **
  ** Copyright 2018 University of Bologna
  ** Released under GPLv3. See LICENSE.txt for details.
  **
  ********************************************************/


/*
 * definitions.h
 */
// dtnbox version
#define DTNBOX_VERSION "0.10.0"
 
#ifndef DTNBOX11_MODEL_DEFINITIONS_H_
#define DTNBOX11_MODEL_DEFINITIONS_H_

#include <linux/limits.h>

//demux token for IPN and DTN scheme

#define DEMUX_TOKEN_IPN_RCV "3000"
#define DEMUX_TOKEN_DTN_RCV "/DTNbox"
#define DEMUX_TOKEN_IPN_SND "3001"
#define DEMUX_TOKEN_DTN_SND "/DTNboxSend"

//paths of the program

#define DTNBOXFOLDER "DTNbox/"
#define FOLDERSTOSYNCFOLDER "foldersToSync/"
#define DTNBOX_FOLDERSTOSYNCFOLDER "DTNbox/foldersToSync/"
#define TEMPDIR ".tempDir/"
#define DBFILENAME "DTNboxDB.db"
#define DBDUMPFILENAME "DTNbox/DTNboxDB.txt"

#define LOGFILENAME "DTNbox.log"


#define OUTFOLDER "out/"
#define INFOLDER "in/"

#define CURRENTPROCESSED "currentProcessed/"
#define WAITINGBUNDLES "waitingBundles/"

#define TEMPCOPY "tempCopy/"

//default values

#define DEFAULT_LIFETIME 600
#define DEFAULT_NUM_RETRY 1

//flag di debug

#define DEBUG 1


//errors while processing receiveCommand()

#define OK 0	//"OK"
#define INTERNALERROR 1	//"INTERNALERROR"

#define NOFOLDER 2	//"NOFOLDER" (-> anche NOSYNC... se non ho la folder non posso avere neanche la sync)
#define NOSYNC 3	//"NOSYNC"

#define BLACKLISTED 4	//"BLACKLISTED"

#define OWNERVIOLATION 5	//"OWNERVIOLATION"

//#define ALREADYSYNC_PUSH 6	//"ALREADYSYNC PUSH"
#define ALREADYSYNC_PULL 7	//"ALREADYSYNC PULL"
#define ALREADYSYNC_PUSHANDPULL 8	//"ALREADYSYNC_PUSHANDPULL"

#define INCOMPLETE 9	//"INCOMPLETE" e concatenato una stringa del tipo 101011...

//mask for inotify
//#define MASK IN_DELETE_SELF | IN_MOVE_SELF | IN_CREATE | IN_MOVED_TO | IN_CLOSE_WRITE | IN_DELETE | IN_MOVED_FROM
//#define MASK IN_DELETE_SELF | IN_MOVE_SELF | IN_CREATE | IN_MOVED_TO | IN_CLOSE_WRITE | IN_DELETE | IN_MOVED_FROM | IN_ATTRIB
#define MASK IN_DELETE_SELF | IN_CREATE | IN_MOVED_TO | IN_CLOSE_WRITE | IN_DELETE | IN_MOVED_FROM | IN_ATTRIB


//monitorAndSendThread defines
#define SLEEP_TIME 5
#define OLDER_THAN_SECONDS 300

//max number of received bundles waiting to be processed
#define MAX_PENDING_RX_BUNDLES 10

#define USER_INPUT_LENGTH 256
#define NODE_EID_LENGTH 256
#define TEMP_STRING_LENGTH 256	//generic temp string, e.g. to sprintf on an integer value


//see <linux/limits.h>
#define OWNER_LENGTH (NAME_MAX + 1)				//e.g. vm5
#define SYNC_FOLDER_LENGTH (NAME_MAX +1)		//e.g. photos
#define FILETOSYNC_LENGTH (512 + 1) 			//e.g. sea/sardinia/friends/group.jpg, so this is for DTNbox relative file name

#define SYSTEM_COMMAND_LENGTH 256 + 1			//including parameters, e.g. tar -czvf, but excluding arguments, e.g. source file path, etc...


#define DTNBOX_SYSTEM_FILES_ABSOLUTE_PATH_LENGTH (256 + 1)		//all dtnbox system files/folders absolute path
																//e.g. ~/DTNbox/foldersToSync/
																//e.g. ~/DTNbox/.tempDir/out/tempCopy
																//e.g. ~/DTNbox/.tempDir/in/currentProcessed
																//e.g. ~/DTNbox/.tempDir/in/waitingBundles
																//e.g. you understood :)

#define FOLDERTOSYNC_ABSOLUTE_PATH_LENGTH (DTNBOX_SYSTEM_FILES_ABSOLUTE_PATH_LENGTH + OWNER_LENGTH + SYNC_FOLDER_LENGTH)
																// e.g. ~/DTNbox/foldersToSync/owner/folder/

#define FILETOSYNC_ABSOLUTE_PATH_LENGTH (DTNBOX_SYSTEM_FILES_ABSOLUTE_PATH_LENGTH + OWNER_LENGTH + SYNC_FOLDER_LENGTH + FILETOSYNC_LENGTH)
																//e.g.	~/DTNbox/foldersToSync/owner/folder/sea/sardinia/friends/group.jpg

#define LINUX_ABSOLUTE_PATH_LENGTH (PATH_MAX +1)


//note: at the moment ack sizes are note defined as they are about the copy of theese commands... see current ACK protocol definition...
#define SYNC_COMMAND_TEXT_SIZE_LIMIT 1024


#define FREEZE_COMMAND_TEXT_SIZE_LIMIT 1024
#define UNFREEZE_COMMAND_TEXT_SIZE_LIMIT 1024
#define FIN_COMMAND_TEXT_SIZE_LIMIT 1024




#define SYNC_PASSWORD_SIZE_LIMIT 256
#define MODE_WHAT_CHECK_UPDATE_SIZE_LIMIT 128
#define RESPONSE_COMMAND_SIZE_LIMIT 256
#define ERROR_MESSAGE_COMMAND_SIZE_LIMIT 256

//#define ABSOLUTE_PATH (PATH_MAX +1)		//length of a generic absolute file path

#define UPDATE_COMMAND_TEXT_SIZE_LIMIT /*16384*/ 1024
#define CHECKUPDATE_COMMAND_TEXT_SIZE_LIMIT /*16384*/ 1024

#define TAR_COMMAND_SIZE_LIMIT /*32768*/ 2048


//definition of constants used to return the outcome of a function call
#define SUCCESS_VALUE 0
#define WARNING_VALUE 1
#define ERROR_VALUE 2


typedef enum {
	NOT_FROZEN = 1,
	FROZEN = 2
} frozen;

typedef enum {
	NOT_BLOCKEDBY = 4,
	BLOCKEDBY = 8
}blockedBy;

typedef enum {
	WHITELIST = 16,
	BLACKLIST = 32
} blackWhite;


//racchiude tutte le informazioni di un nodo DTN
typedef struct dtnNode {
	char EID[NODE_EID_LENGTH];				//definire meglio di che tipo deve essere EID, vedi al_bp
	int lifetime;
	int numTx;
	blackWhite blackWhite;
	frozen frozen;
	blockedBy blockedBy;
} dtnNode;
//end old source "dtnNode.h"



typedef struct {
	char relativeTarName[NAME_MAX + 1]; //relative to ~/DTNbox/.tempDir/in/waitingBundles/
	dtnNode source;	//source of the related bundle
}pendingReceivedBundleInfo;












#endif /* DTNBOX11_MODEL_DEFINITIONS_H_ */
