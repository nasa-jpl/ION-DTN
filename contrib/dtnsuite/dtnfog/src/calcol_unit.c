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
 * calcol_unit.c
 *
 * Source code of the elaboration thread in dtnfog architecture
 * */


#include "proxy_thread.h"
#include "debugger.h"
#include "utility.h"

/* ---------------------------
 *      Global variables
 * --------------------------- */
pthread_t processor_thread[N_PROCESSORS];
processor_info_t threads_memory[N_PROCESSORS];

/* -------------------------------
 *       Function interfaces
 * ------------------------------- */
void startProcessingThread(processor_info_t info);
void setAddressToPrint(calcol_unit_info_t * info,circular_buffer_item item);
//static void criticalError(void *arg);


void * calcolUnitThread(void * arg){
	calcol_unit_info_t * info=(calcol_unit_info_t *) arg;
	opendir(CLC_DIR);
	if(errno == ENOENT) {
		debug_print(info->debug_level, "[DEBUG] Creating %s\n", CLC_DIR);
		mkdir(CLC_DIR, 0700);
	}

	if(info->cloud==1) printf("Calcol unit: dtnfog in cloud mode\n");
	else printf("Calcol unit: dtnfog in fog mode\n");

	while(1==1){
		//Getting an incoming calcol requestc
		circular_buffer_item item=circular_buffer_pop(&(info->calcol_unit_buffer));
		pthread_mutex_unlock(&(info->calcol_unit_buffer.mutex));
		//starting new processor thread
		processor_info_t toProcess;
		toProcess.bp_tosend_buffer=info->bp_tosend_buffer;
		toProcess.tcp_tosend_buffer=info->tcp_tosend_buffer;
		toProcess.debug_level=info->debug_level;
		toProcess.options=info->options;
		toProcess.item=item;
		toProcess.cloud=info->cloud;
		strcpy(toProcess.allowed_comand,info->allowed_comand);
		strcpy(toProcess.mycloud,info->mycloud);
		setAddressToPrint(info,item);
		strcpy(toProcess.addresso_to_print,info->addresso_to_print);
		strcpy(toProcess.comunication_address,info->communication_address);
		toProcess.numberOfCommand=info->numberOfCommand;
		startProcessingThread(toProcess);
	}
	return NULL;
}

void startProcessingThread(processor_info_t toProcess) {
	int i=0;
	while(1==1){
		for(i=0;i<N_PROCESSORS;i++){
			if(!processor_thread[i]||(pthread_kill(processor_thread[i], 0 )) != 0){
				threads_memory[i]=toProcess;
				pthread_create(&(processor_thread[i]), NULL, processorThread, (void *)&(threads_memory[i]));
				printf("%s Asseggnato al processor thread %d\n",toProcess.item.fileName,i);
				return;
			}
		}
		printf("All processor threads are busy retry in 1 sec\n");
		sleep(1);
	}
}

void setAddressToPrint(calcol_unit_info_t * info,circular_buffer_item item){
	if(item.protocol==0){ //BP
		if(info->report==1) strcpy(info->addresso_to_print,info->bp_address);
				else{
					if(strstr(info->mycloud, "ipn:") != NULL ) strcpy(info->addresso_to_print,"ipn:*.*");
					if(strstr(info->mycloud, "dtn://") != NULL) strcpy(info->addresso_to_print,"dtn://*");
				}
	}else{ //TCP
		if(info->report==1) strcpy(info->addresso_to_print,info->tcp_address);
				else strcpy(info->addresso_to_print,"*.*.*.*:****");
	}
}
