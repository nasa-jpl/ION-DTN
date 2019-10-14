/********************************************************
    Authors:
    Lorenzo Tullini, lorenzo.tullini@studio.unibo.it
    Carlo Caini (DTNfog project supervisor), carlo.caini@unibo.it

    License:
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Copyright (c) 2018, Alma Mater Studiorum, University of Bologna

 ********************************************************/

/*
 * processorUnit.c
 *
 * Source code for calcol unit processing
 *
 *
 *					Execution request
 *					+
 * 					|
 *					v
 *					Extract tar
 *					+
 *					|
 *					v
 *					Search result.rs +-------------------->Result.rs not founded
 *					+                                      +
 *					|                                      |
 *					v                                      v
 *					Result.rs founded                      Try to execute command.cmd+---------->Command not allowed
 *					+                                      +                                     +
 *					|                                      |                                     |
 *					v                                      v                                     v
 *					Get last route.rt address              Command successful executed           I'am cloud +---------------------->Push my address in route.rt
 *					+                                      +                                     +                                  +
 *					|                                      |                                     |                                  |
 *					v                                      v                                     v                                  v
 *					Sand tar                               Get last route.rt address             Report error in result.rs          Set next address cloud address
 *														   +                                     +                                  +
 *														   |                                     |                                  |
 *														   v                                     v                                  v
 *														   Sand tar                              Sand tar                           Sand tar
 *
 *
 * */


#include "proxy_thread.h"
#include "debugger.h"
#include "utility.h"

/* -------------------------------
 *       Function interfaces
 * ------------------------------- */
int extractTar(processor_info_t * info);
void getDirName(char * dirName,char * filename);
int countHops(char *filename);
int thereIsResult();
void popNextHop(char *routeFile,char * nextHop);
int sendFile(processor_info_t * info);
int tarFileMaker(processor_info_t * info);
int executeCommand(processor_info_t * info);
void rmDirectory();
int pushMyHop(char *routeFile,char * myhop,char mycloud[IP_LENGTH]);
int printReaportFile(char * toPrint);

/* ---------------------------
 *      Global variables
 * --------------------------- */
static char dirName[DIR_NAME];
static char routeFileName[FILE_NAME];
static char nextHop[DIR_NAME];
static int nOfFiles;
static char reportFile[DIR_NAME+FILE_NAME];
static int printReport = 0;
static char message[256];


void * processorThread(void * arg){
	int utility_error;
	processor_info_t * info=(processor_info_t *) arg;
	debug_print(info->debug_level,"[DEBUG] \t%lu Processing Thread says: processing item %s\n",(long) pthread_self(),info->item.fileName);
	printf("\t%lu Processing Thread says: processing item %s\n",(long) pthread_self(),info->item.fileName);

	//Extract file
	utility_error=extractTar(info);
	if(utility_error!=RETURN_OK) return NULL;
	debug_print(info->debug_level,"[DEBUG] \t%lu Processing Thread says: successful extracted %s\n",(long) pthread_self(),info->item.fileName);
	printf("\t%lu Processing Thread says: successful extracted %s\n",(long) pthread_self(),info->item.fileName);
	//End file extraction

	if(printReport==1){
		strcpy(message,"Processed by ");
		strcat(message,info->addresso_to_print);
		if(info->cloud==1) strcat(message," cloud ");
		else strcat(message," fog ");
		printReaportFile(message);
	}

	//Searching result.rs file if it's in tar dtnfog only work like a gateway and forward tar to last line in route.rt
	if(thereIsResult()==RESULT_FOUND){ //send result to dest
		strcpy(routeFileName,dirName);
		strcat(routeFileName,"/route.rt");
		popNextHop(routeFileName,nextHop);
		printf("\t%lu Processing Thread says: result found send to %s\n",(long) pthread_self(),nextHop);
		if(printReport==1){
			strcpy(message,info->addresso_to_print);
			strcat(message,": result.rs found, send back to previous node");
			printReaportFile(message);
		}
		tarFileMaker(info);
		sendFile(info);
		//file tar sanded

	}else{// there isn't result.rs file in tar
		printf("\t%lu Processing Thread says: result not found tries to execute command\n",(long) pthread_self());

		if(printReport==1){
				strcpy(message,info->addresso_to_print);
				strcat(message,": result.rs not found, tries to execute command.cmd");
				printReaportFile(message);
		}
		//Dtnfog tries to execute command.cmd
		if((executeCommand(info))==RETURN_OK){ //Dtnfog runs successful command.cmd so it send result tar to last line in route.rt
			strcpy(routeFileName,dirName);
			strcat(routeFileName,"/route.rt");
			popNextHop(routeFileName,nextHop);

			if(printReport==1){
				strcpy(message,info->addresso_to_print);
				strcat(message,": command.cmd successful executed, send back to previous node");
				printReaportFile(message);
			}

			tarFileMaker(info);
			sendFile(info);
			//file tar sanded
		}else{ //Dtnfog can not runs command.cmd
			printf("\t%lu Processing Thread says: command not allowed, ",(long) pthread_self());
			if(info->cloud==1){ //Dtnfog runs in cloud mode so it report execution error in result.rs
				printf("cloud returns error\n");
				char resutlFileName[DIR_NAME+FILE_NAME];
				strcpy(resutlFileName,dirName);
				strcat(resutlFileName,"/result.rs");
				FILE *rs=fopen(resutlFileName,"w");
				fprintf(rs,"Cloud says: command not found\n");
				fclose(rs);
				strcpy(routeFileName,dirName);
				strcat(routeFileName,"/route.rt");
				popNextHop(routeFileName,nextHop);
				if(printReport==1){
					strcpy(message,info->addresso_to_print);
					strcat(message,": cloud says command.cmd not allowed, send back to: ");
					strcat(message,nextHop);
					printReaportFile(message);
				}
				tarFileMaker(info);
				sendFile(info);
				//file tar sanded
			}else{ //dtnfog runs in fog mode so it's forwards tar to its cloud address defined in dtnfog.conf
				printf("forwards to cloud %s\n",info->mycloud);
				strcpy(nextHop,info->mycloud);
				strcpy(routeFileName,dirName);
				strcat(routeFileName,"/route.rt");
				if(pushMyHop(routeFileName,info->comunication_address,info->mycloud)==RETURN_OK){
					if(printReport==1){
						strcpy(message,info->addresso_to_print);
						strcat(message,": says command.com not found, send next hop");
						printReaportFile(message);
					}
					tarFileMaker(info);
					sendFile(info);
					//file tar sanded
				}else printf("\t%lu Processing Thread says: error in forwarding\n",(long) pthread_self());
			}
		}
	}
	return NULL;
}

/*
 * Function to extract tar file
 * it's also  check if there is report.rp indeed to write report of execution
 * */
int extractTar(processor_info_t * info){
	//int utility_error;
	char filename[FILE_NAME];
	char extractcommand[256];
	char returnLog[256];

	strcpy(filename,info->item.fileName);

	//Prepare log file for extraction
	strcpy(returnLog,".");
	strcpy(returnLog,filename);
	strcat(returnLog,".log");

	//Extracting file
	strcpy(extractcommand,"tar -xvf "); //tar -xvf
	strcat(extractcommand,filename);  //tar -xvf <nomeAssolutoFile>
	strcat(extractcommand," -C "); //tar -xvf <nomeAssolutoFile> -C
	strcat(extractcommand,CLC_DIR); //tar -xvf <nomeAssolutoFile> -C CLC_DIR
	strcat(extractcommand," | wc -l > "); //tar -xvf <nomeAssolutoFile> -C CLC_DIR | wc -l >
	strcat(extractcommand,returnLog); //tar -xvf <nomeAssolutoFile> -C CLC_DIR | wc -l > <nomeAssolutFile>.log
	strcat(extractcommand," ; "); //tar -xvf <nomeAssolutoFile> -C CLC_DIR | wc -l > <nomeAssolutFile>.log ;
	strcat(extractcommand,"rm "); //tar -xvf <nomeAssolutoFile> -C CLC_DIR | wc -l > <nomeAssolutFile>.log ; rm
	strcat(extractcommand,filename); //tar -xvf <nomeAssolutoFile> -C CLC_DIR | wc -l > <nomeAssolutFile>.log ; rm filename.tar
	system(extractcommand);

	//Reading command return value for test correct extraction
	FILE *log=fopen(returnLog,"r");
	if(log==NULL){
		error_print("\t%lu Processing Thread says: error opening log file %s\n",(long) pthread_self(),returnLog);
		return RETURNVALUE_ERROR;
	}

	int returnValute;
	fscanf(log,"%d",&returnValute);
	if(returnValute < MIN_NUMBER_OF_FILES_IN_TAR){
		printf("\tReturn value: %d\n",returnValute);
		error_print("\t%lu Processing Thread says: error in extraction of file %s, invalid tar for dtnfog\n",(long) pthread_self(),filename);
		return  VALIDTAR_ERROR;
	}
	fclose(log);

	if(remove(returnLog)!=0){
		error_print("\t%lu Processing Thread says: error removing file %s\n",(long) pthread_self(),returnLog);
		return DELETE_ERROR;
	}

	/*if(remove(filename)!=0){
		error_print("\t%lu Processing Thread says: error removing file %s\n",(long) pthread_self(),filename);
		return DELETE_ERROR;
	}*/

	nOfFiles=returnValute-3;
	strcpy(info->item.fileName,filename);

	getDirName(dirName,info->item.fileName);
	printf("\tDir name: %s\n",dirName);
	//End of extraction

	struct dirent *entry;
	DIR *dir=opendir(dirName);
	while( (entry=readdir(dir)) ){
		 if((strcmp(entry->d_name,"report.rp"))==0){
			 strcpy(reportFile,dirName);
			 strcat(reportFile,"/report.rp");
			 printReport=1;
			 closedir(dir);
			 return RETURN_OK;
		 }
	}
	printReport=0;

	closedir(dir);
	return RETURN_OK;
}

/*
 * Function return absolute directory name by an absolute file name
 * */
void getDirName(char * dirName,char * filename){
	int i=0;
	for(i=0;i<strlen(filename)-4;i++){
		dirName[i]=filename[i];
	}
	dirName[i]='\0';
}

/*
 * Function counts hops in route.rt
 * */
int countHops(char *filename){
	FILE* myfile = fopen(filename, "r");
	int ch, number_of_lines = 0;
	ch = fgetc(myfile);
	while (ch != EOF){
		if(ch == '@') number_of_lines++;
		ch = fgetc(myfile);
	}
	fclose(myfile);
	return number_of_lines;
}

/*
 * Function checks if there is result.rs in tar
 * */
int thereIsResult(){
	struct dirent *entry;
	DIR *dir=opendir(dirName);
	while( (entry=readdir(dir)) ){
	   if((strcmp(entry->d_name,"result.rs"))==0){
		   return RESULT_FOUND;
	   }
	}
	closedir(dir);
	return RESULT_NOT_FOUND;
}

/*
 * Function returns last address in route.rt like a pop function in list model but in persistence file
 * */
void popNextHop(char *routeFile,char * nextHop){
	memset(nextHop,0,DIR_NAME);
	int linetodelete=countHops(routeFile);
    FILE *file=fopen(routeFile,"r");
    FILE *filetemp;
    char tempFileName[FILE_NAME];
    char line[255];

    strcpy(tempFileName,routeFile);
    strcat(tempFileName,".tmp");
    filetemp=fopen(tempFileName,"w");
    unsigned int countline=1;

    while(fgets(line,255,file)!=NULL){
        if(countline!=linetodelete) fputs(line,filetemp);
        else break;
        countline++;
    }
    fclose(file);
    remove(routeFile);
    fclose(filetemp);
    rename(tempFileName,routeFile);
    int i=1;
    for(i=1;i<strlen(line)-1;i++){
    	nextHop[i-1]=line[i];
    }
}

/*
 * Function puts file tar in the right circular buffer indeed to be send by senders threads
 * */
int sendFile(processor_info_t * info){
	if(strstr(nextHop, "ipn:") != NULL){//is dtn uri
		circular_buffer_item item;
		char tmp[FILE_NAME];
		strcpy(tmp,DTN_DIR);
		strcat(tmp,basename(info->item.fileName));
		rename(info->item.fileName,tmp);
		strcpy(item.fileName,tmp);
		strcpy(item.eid,nextHop);
		circular_buffer_push(info->bp_tosend_buffer,item);
		printf("\t%lu Processing Thread says: %s send by bp to %s\n",(long) pthread_self(),info->item.fileName,item.eid);
	}else{//is tcp address
		circular_buffer_item item;
		char* token = strtok(nextHop, ":");
		strcpy(item.ip,token);
		token = strtok(NULL, ":");
		item.port=atoi(token);
		char tmp[FILE_NAME];
		strcpy(tmp,TCP_DIR);
		strcat(tmp,basename(info->item.fileName));
		rename(info->item.fileName,tmp);
		strcpy(item.fileName,tmp);
		circular_buffer_push(info->tcp_tosend_buffer,item);
		printf("\t%lu Processing Thread says: %s send by tcp to %s:%d\n",(long) pthread_self(),info->item.fileName,item.ip,item.port);
	}
	return RETURN_OK;
}

/*
 * Function compresses working directory in order to makes tar file
 * */
int tarFileMaker(processor_info_t * info){
	char command[512];
	strcpy(command,"tar -cvf "); //tar -cvf
	strcat(command,info->item.fileName); //tar -cvf <nomeAssolutoFile>
	strcat(command," "); //tar -cvf <nomeAssolutoFile>
	strcat(command,"-C "); //tar -cvf <nomeAssolutoFile> -C
	strcat(command,CLC_DIR); //tar -cvf <nomeAssolutoFile> CLC_DIR
	strcat(command," --remove-files "); //tar -cvf <nomeAssolutoFile> CLC_DIR --remove-files
	strcat(command,basename(dirName)); //tar -cvf <nomeAssolutoFile> CLC_DIR --remove-files dirname
	strcat(command," >> .dtnfog_tar.log"); //tar -cvf <nomeAssolutoFile> CLC_DIR dirname >> dtnfog_tar.log
	//printf("\t%lu Processing Thread says: execute %s\n",(long) pthread_self(),command);
	system(command);
	printf("\t%lu Processing Thread says: created %s tar file\n",(long) pthread_self(),info->item.fileName);

	return RETURN_OK;
}

/*
 * Function executes command.cmd if it's in commands list section in dtnfog.config
 * */
int executeCommand(processor_info_t * info){
	command_t cmd;
	char cmdFileName[DIR_NAME+FILE_NAME];
	char tmp[DIR_NAME+FILE_NAME];
	char tmpFileName[DIR_NAME+FILE_NAME];
	char resutlFileName[DIR_NAME+FILE_NAME];

	int i=0;
	int flag=0;

	strcpy(resutlFileName,dirName);
	strcat(resutlFileName,"/result.rs");

	strcpy(cmdFileName,dirName);
	strcat(cmdFileName,"/command.cmd");
	cmd.number_of_file=nOfFiles;

	//prepare memory for command
	FILE *cmdFile=fopen(cmdFileName,"r");
	//read comand
	fscanf(cmdFile,"%s",tmp);
	strcpy(cmd.cmd,tmp); //comandname

	//test comand
	char *token;
	token = strtok(info->allowed_comand, " ");
	while( token != NULL ) {
	   if(strcmp(token,cmd.cmd)==0){
		   flag=1;
		   break;
	   }
	   token = strtok(NULL, " ");
	}

	if(flag==1){
		//read argument files
		while(fscanf(cmdFile,"%s",tmpFileName) != EOF && i<MIN_NUMBER_OF_FILES_IN_TAR){
			strcpy(cmd.files[i],dirName);
			strcat(cmd.files[i],"/");
			strcat(cmd.files[i],tmpFileName);
			i++;
		}


		for(i=0;i<nOfFiles;i++){
			strcat(cmd.cmd," ");
			strcat(cmd.cmd,cmd.files[i]);
		}

		strcat(cmd.cmd," > ");
		strcat(cmd.cmd,resutlFileName);
		strcat(cmd.cmd," 2> ");
		strcat(cmd.cmd,resutlFileName);

		//execute
		system(cmd.cmd);

		fclose(cmdFile);
		return RETURN_OK;
	}

	fclose(cmdFile);
	return COMMAND_NOT_FOUND;
}

/*
 * Function removes recursively a directory
 * */
void rmDirectory(){
	DIR *d;
	struct dirent *dir;
    d = opendir(dirName);
    if (d){
        while ((dir = readdir(d)) != NULL){
	          remove(dir->d_name);
        }
        closedir(d);
        rmdir(dirName);
	}
}

/*
 * Function pushes in route.rt its address (defined in dtnfog.conf) and set as nextHop cloud address (also defined in dtnfog.conf)
 * */
int pushMyHop(char *routeFile,char * myhop,char mycloud[IP_LENGTH]){
	FILE *rt=fopen(routeFile,"a");
	if(rt==NULL){
		return VALIDTAR_ERROR;
	}
	fprintf(rt,"@%s\n",myhop);
	fclose(rt);
	strcpy(nextHop,mycloud);
	return RETURN_OK;
}

/*
 * Graphic utility function to write report.rp
 * */
int printReaportFile(char * toPrint){
	FILE * report;
	report=fopen(reportFile,"a");
	fprintf(report,"%s\n",toPrint);
	fprintf(report,"\t | \n");
	fprintf(report,"\t | \n");
	fprintf(report,"\t V \n");
	fclose(report);
	return RETURN_OK;
}
