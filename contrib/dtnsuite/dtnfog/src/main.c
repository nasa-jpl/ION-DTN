/********************************************************
    DTNfog Copyright (c) 2018, Alma Mater Studiorum, University of Bologna

    Authors:
    Lorenzo Mustich, lorenzo.mustich@studio.unibo.it
	Lorenzo Tullini, lorenzo.tullini@studio.unibo.it
    Carlo Caini (DTNfog project supervisor), carlo.caini@unibo.it

    License:
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 ********************************************************/

/*
 * main.c
 *
 * This file contains the main of
 * DTNfog
 *
 */

#include "debugger.h"
#include "definitions.h"
#include <ifaddrs.h>
#include "proxy_thread.h"

#define IPN_DEMUX_SENDER 5001
#define IPN_DEMUX_RECEIVER 5000
#define N_PORT 2500

/* ---------------------------
 *      Global variables
 * --------------------------- */

int listenSocket;
al_bp_extB_error_t utility_error;
al_bp_extB_registration_descriptor rd_send;
al_bp_extB_registration_descriptor rd_receive;
pthread_t tcpReceiver;
pthread_t bundleSender;
pthread_t bundleReceiver;
pthread_t tcpSender;
pthread_t calcolUnit;
//sem_t tcpRecv;
//sem_t bundleSnd;
sem_t bundleRecv;
sem_t tcpSnd;
proxy_error_t error;
static tcp_sender_info_t tcp_sender_info;
static bp_sender_info_t bp_sender_info;
static tcp_receiver_info_t tcp_receiver_info;
static bp_receiver_info_t bp_receiver_info;
static calcol_unit_info_t calcol_unit_info;
int level_debug = 0;
char mode;
int cloud=0;
int report=0;
const char * program_name;
//circular_buffer_t tcp_bp_buffer;
//circular_buffer_t bp_tcp_buffer;
al_bp_endpoint_id_t myEid;
char myIp[IP_LENGTH+6];




/** -------------------------------
 *       Function interfaces
 * ------------------------------- */
void proxy_exit(int error);
void print_usage(const char * program_name);
void parse_options(int argc, char * argv[]);
int loadConfigFile(calcol_unit_info_t *info);
void printConfigFile();
void selectCommunicationAddress();
char * getMyIp(char * interface);

/**
 * Main code
 */
int main(int argc, char *argv[]){
	printf("DTNfog PID: %d\n",getpid());
	const int on = 1;
	struct sockaddr_in proxyaddr;
	sigset_t set;
	int sig;

	//Parse command line options
	parse_options(argc, argv);
	loadConfigFile(&calcol_unit_info);
	debugger_init(level_debug, TRUE, LOG_FILENAME);



	//Init TCP side
	memset((char *)&proxyaddr, 0, sizeof(proxyaddr));
	proxyaddr.sin_family = AF_INET;
	proxyaddr.sin_addr.s_addr = INADDR_ANY;
	proxyaddr.sin_port = htons(N_PORT);

	listenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if(listenSocket < 0) {
		error_print("Error in creating the socket: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	debug_print(level_debug, "[DEBUG] listen socket create with fd: %d\n", listenSocket);

	if(setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
		error_print("Error in setting socket option: %s\n", strerror(errno));
		error = SOCKET_ERROR;
		proxy_exit(error);
	}

	if(bind(listenSocket, (struct sockaddr *)&proxyaddr, sizeof(proxyaddr)) < 0) {
		error_print("Error in bind(): %s\n", strerror(errno));
		error = SOCKET_ERROR;
		proxy_exit(error);
	}

	if(listen(listenSocket, 5) < 0) {
		error_print("Error in listen(): %s\n", strerror(errno));
		error = SOCKET_ERROR;
		proxy_exit(error);
	}
	debug_print(level_debug, "[DEBUG] init TCP side: success\n");
	//printf("Ip address: %s\n",id->ifa_addr);


	//test if dtnfog dir exists
	opendir(PROGRAM_DIR);
	if(errno == ENOENT) {
		debug_print(level_debug, "[DEBUG] Creating %s\n", PROGRAM_DIR);
		mkdir(PROGRAM_DIR, 0700);
	}

	//Init DTN side
	utility_error = al_bp_extB_init('N',0);
	if (utility_error != BP_EXTB_SUCCESS) {
		error_print("Error in al_bp_extB_init(): (%s)\n", al_bp_strerror(utility_error));
		error = SOCKET_ERROR;
		proxy_exit(error);
	}
	debug_print(level_debug, "[DEBUG] al_bp_extB_init: success\n");

	//Register BundleSender to BP
	utility_error = al_bp_extB_register(&rd_send, "proxy_sender", IPN_DEMUX_SENDER);
	if (utility_error != BP_EXTB_SUCCESS) {
		error_print("Error in al_bp_extB_register() for sending thread (%s)\n", al_bp_strerror(utility_error));
		error = REGISTER_ERROR;
		proxy_exit(error);
	}
	debug_print(level_debug, "[DEBUG] al_bp_extB_register for sending thread, rd: %d\n", rd_send);

	//Register BundleReceiver to BP
	utility_error = al_bp_extB_register(&rd_receive, "proxy_receiver", IPN_DEMUX_RECEIVER);
	if (utility_error != BP_EXTB_SUCCESS) {
		error_print("Error in al_bp_extB_register() for receiving thread (%s)\n", al_bp_strerror(utility_error));
		error = REGISTER_ERROR;
		proxy_exit(error);
	}
	debug_print(level_debug, "[DEBUG] al_bp_extB_register for receiving thread, rd: %d\n", rd_receive);
	debug_print(level_debug, "[DEBUG] init DTN side: success\n");
	printf("TCP/IP address: %s\n",myIp);

	//get eid for calcounit
	myEid=al_bp_extB_get_local_eid(rd_receive);


	//Init data structure for calcol unit
	circular_buffer_init(&(calcol_unit_info.calcol_unit_buffer),MAX_BUFFER_SIZE);
	calcol_unit_info.debug_level=level_debug;
	if (pthread_mutex_init(&(calcol_unit_info.calcol_unit_buffer.mutex), NULL) != 0){
	    printf("\n mutex init failed\n");
	    proxy_exit(SIGINT);
	}
	debug_print(level_debug, "[DEBUG] loaded list of allowed commands\n");

	//Init data structure senders side
	circular_buffer_init(&(tcp_sender_info.tcp_tosend_buffer),MAX_BUFFER_SIZE);
	tcp_sender_info.debug_level=level_debug;
	if (pthread_mutex_init(&(tcp_sender_info.tcp_tosend_buffer.mutex), NULL) != 0){
		printf("\n mutex init failed\n");
		proxy_exit(SIGINT);
	}

	circular_buffer_init(&(bp_sender_info.bp_tosend_buffer),MAX_BUFFER_SIZE);
	bp_sender_info.debug_level=level_debug;
	bp_sender_info.rd_send=rd_send;
	bp_sender_info.error=0;
	if (pthread_mutex_init(&(bp_sender_info.bp_tosend_buffer.mutex), NULL) != 0){
	    printf("\n mutex init failed\n");
	    proxy_exit(SIGINT);
	}

	//Init data structure receivers side
	tcp_receiver_info.calcol_unit_circular_buffer=&(calcol_unit_info.calcol_unit_buffer);
	tcp_receiver_info.listenSocket_fd=listenSocket;
	tcp_receiver_info.debug_level=level_debug;

	bp_receiver_info.calcol_unit_circular_buffer=&(calcol_unit_info.calcol_unit_buffer);
	bp_receiver_info.debug_level=level_debug;
	bp_receiver_info.error=0;
	bp_receiver_info.rd_receive=rd_receive;

	//Set senders circular buffers address to calcol unit
	calcol_unit_info.bp_tosend_buffer=&(bp_sender_info.bp_tosend_buffer);
	calcol_unit_info.tcp_tosend_buffer=&(tcp_sender_info.tcp_tosend_buffer);
	//set address in calcol_unit data structure
	if(cloud==0) selectCommunicationAddress();
	strcpy(calcol_unit_info.tcp_address,myIp);
	strcpy(calcol_unit_info.bp_address,myEid.uri);


	//Init mode
	if(mode == 'n') {
		tcp_sender_info.options = mode;
		bp_sender_info.options = mode;
		tcp_receiver_info.options = mode;
		bp_receiver_info.options = mode;
		calcol_unit_info.options = mode;

		printf("DTNperf compatibility: none\n");
	}
	else {
		tcp_sender_info.options = 'd';
		bp_sender_info.options = 'd';
		tcp_receiver_info.options = 'd';
		bp_receiver_info.options = 'd';
		calcol_unit_info.options = 'd';
		printf("DTNperf compatibility: ok\n");
	}
	calcol_unit_info.cloud=cloud;

	//Assing proxy_exit to SIGINT and init mask to critic error signal from threads
	sigemptyset(&set);
	sigaddset(&set, SIGINT);
	sigprocmask(SIG_BLOCK, &set, NULL);

	//select correct address indeed to colud protocol
	if(cloud==0) selectCommunicationAddress();

	//Init threads
	pthread_create(&calcolUnit, NULL, calcolUnitThread, (void *)&calcol_unit_info);
	pthread_create(&tcpReceiver, NULL, tcpReceiving, (void *)&tcp_receiver_info);
	pthread_create(&bundleSender, NULL, bundleSending, (void *)&bp_sender_info);
	pthread_create(&bundleReceiver, NULL, bundleReceiving, (void *)&bp_receiver_info);
	pthread_create(&tcpSender, NULL, tcpSending, (void *)&tcp_sender_info);

	//Wait for children return,in this case, since the children are daemons, dnproxy ends with an error
	sigwait(&set, &sig);
	//printf("Main had recived a %d from threads\n",sig);
	proxy_exit(SIGINT);
	return EXIT_FAILURE;
}


/**
 * Function for clean exit
 */
void proxy_exit(int error) {
	circular_buffer_free(&(tcp_sender_info.tcp_tosend_buffer));
	circular_buffer_free(&(bp_sender_info.bp_tosend_buffer));
	circular_buffer_free(&(calcol_unit_info.calcol_unit_buffer));
	if((error == THREAD_ERROR) || error == SIGINT) {
		//Close threads
		pthread_cancel(tcpReceiver);
		printf("TCPreceiver: exit\n");
		pthread_cancel(bundleSender);
		printf("BPsender: exit\n");
		pthread_cancel(bundleReceiver);
		printf("BPreceiver: exit\n");
		pthread_cancel(tcpSender);
		printf("TCPsender: exit\n");
		pthread_cancel(calcolUnit);
		printf("Calcol unit: exit\n");
	}

	//Close socket
	if(error==SOCKET_ERROR || error == SIGINT){
		shutdown(listenSocket, 0);
		shutdown(listenSocket, 1);
		close(listenSocket);
	}

	if(error == REGISTER_ERROR) {
		al_bp_extB_destroy();
		exit(EXIT_FAILURE);
	}

	if(((error != SOCKET_ERROR && error != REGISTER_ERROR)) || error == SIGINT) {
		if(bp_sender_info.error!=BP_EXTB_ERRRECEIVE && bp_receiver_info.error!=BP_EXTB_ERRRECEIVE){
			//Unregister BundleSender and BundleReceiver
			utility_error = al_bp_extB_unregister(rd_send);
			if (utility_error != BP_EXTB_SUCCESS) {
				error_print("Error in al_bp_extB_unregister() for sending thread (%s)\n", al_bp_strerror(utility_error));
				exit(EXIT_FAILURE);
			}
			debug_print(level_debug, "[DEBUG] al_bp_extB_unregister for sending thread\n");

			utility_error = al_bp_extB_unregister(rd_receive);
			if (utility_error != BP_EXTB_SUCCESS) {
				error_print("Error in al_bp_extB_unregister() for receiving thread (%s)\n", al_bp_strerror(utility_error));
				exit(EXIT_FAILURE);
			}
			debug_print(level_debug, "[DEBUG] al_bp_extB_unregister for receiving thread\n");

			al_bp_extB_destroy();
		}

		//Destroy semaphores
		if(((error != SOCKET_ERROR && error != REGISTER_ERROR && error != SEMAPHORE_ERROR)) || error == SIGINT) {
			//sem_destroy(&tcpRecv);
			//sem_destroy(&bundleSnd);
			sem_destroy(&bundleRecv);
			sem_destroy(&tcpSnd);
		}
	}

	if(error == SIGINT) {
		printf("Exit...\n");
		exit(EXIT_SUCCESS);
	}


	error_print("Exit...\n");
	exit(EXIT_FAILURE);
}

/**
 * Function for printing program usage
 */
void print_usage(const char* progname){
	fprintf(stderr, "\n");
	fprintf(stderr, "DTNfog\n");
	fprintf(stderr, "SYNTAX: %s [options]\n", progname);
	fprintf(stderr, "\n");
	fprintf(stderr, "options:\n");
	fprintf(stderr, " --no-header             Bundles do not contain DTNperf header\n");
	fprintf(stderr, " --debug                 Debug messages\n");
	fprintf(stderr, " --help                  Print this screen.\n");

	fprintf(stderr, "\n");
}

/**
 * Function for parsing command line options
 */
void parse_options(int argc, char * argv[]) {
	program_name = basename(argv[0]);

	char c, done = 0;

	while(!done) {
		static struct option long_options[] =
		{
				{"help", no_argument, 0, 'h'},
				{"no-header", no_argument, 0, 'n'},
				{"debug", no_argument, 0, 'd'},
				{0, 0, 0, 0}
		};

		int option_index = 0;
		c = getopt_long(argc, argv, "hndc", long_options, &option_index);

		switch(c)
		{
		case 'h':
			print_usage(program_name);
			exit(0);
			return;
		case 'n':
			mode = 'n';
			break;
		case 'd':
			level_debug = DEBUG_ON;
			break;
		case (char) -1:
				done = 1;
		break;
		default:
			print_usage(program_name);
			exit(0);
		}
	}

}

/**
 * Function parses dntfog.conf
 */
int loadConfigFile(calcol_unit_info_t *info){
	int i=0;
	char tmp[COMMAND_SIZE];
	char interface[5];
	char port[6];

	FILE *acmd;
	//va cambiato!!!
	acmd=fopen("./dtnfog.conf","r");
	if(acmd==NULL){
		error_print("dtnfog.conf not found\n");
		printConfigFile();
		error_print("Please compile ./dtnfog.conf\n");
		exit(EXIT_FAILURE);
	}
	printf("--------------------------------\n");
	printf("|   List of allowed commands:  |\n");
	printf("--------------------------------\n");
	strcpy(info->allowed_comand,"");

	//load allowed command
	while(fscanf(acmd,"%s",tmp)!=EOF){
		if(strcmp(tmp,"##list-commands##")==0) break;
	}
	while(fscanf(acmd,"%s",tmp)!=EOF && i<MAX_NUMBER_OF_COMMANDS){
		if(strcmp(tmp,"##end-list-commands##")==0) break;
		if(tmp[0]!='#' && strcmp(tmp,"")!=0 &&  strcmp(tmp,"\n")!=0 &&  strcmp(tmp," ")!=0){
			printf("%d) %s\n",(i+1),tmp);
			strcat(info->allowed_comand,tmp);
			strcat(info->allowed_comand," ");
		}
		i++;
	}
	printf("--------------------------------\n");
	info->numberOfCommand=i;

	//load cloud mode
	char cloudMode[5];
	while(fscanf(acmd,"%s",tmp)!=EOF){
		if(strcmp(tmp,"##cloud-mode##")==0) break;
	}
	fscanf(acmd,"%s",cloudMode);
	while(strcmp(tmp,"##end-cloud-mode##")==0){
		fscanf(acmd,"%s",tmp);
	}
	if(strstr(cloudMode, "true") != NULL) cloud=1;




	//load cloud address if i'm not cloud
	if(cloud==0){
		while(fscanf(acmd,"%s",tmp)!=EOF){
			if(strcmp(tmp,"##next-address##")==0) break;
		}

		fscanf(acmd,"%s",tmp);
		while(strcmp(tmp,"##end-next-address##")==0){
			fscanf(acmd,"%s",tmp);
		}
		if(tmp[0]!='#' && strcmp(tmp,"")!=0 && strcmp(tmp,"\n")!=0 &&  strcmp(tmp," ")!=0) strcpy(info->mycloud,tmp);

		if(strcmp(info->mycloud,"")==0){
			error_print("Error in dtnfog.conf in cloud address section\n");
			exit(EXIT_FAILURE);
		}
		else printf("Next node at: %s\n",info->mycloud);
	}

	while(fscanf(acmd,"%s",tmp)!=EOF){
		if(strcmp(tmp,"##tcp-interface##")==0) break;
	}
	fscanf(acmd,"%s",interface);
	//get my ip
	strcpy(myIp,getMyIp(interface));
	strcat(myIp,":");
	sprintf(port, "%d", N_PORT);
	strcat(myIp,port);

	char allowReports[5];
	while(fscanf(acmd,"%s",tmp)!=EOF){
		if(strcmp(tmp,"##allow-reports##")==0) break;
	}
	fscanf(acmd,"%s",allowReports);
	while(strcmp(tmp,"##end-allow-reports##")==0){
		fscanf(acmd,"%s",tmp);
	}
	if(strstr(allowReports, "true") != NULL) calcol_unit_info.report=1;


	fclose(acmd);
	return 0;
}

/**
 * Function prints a well formated dntfog.conf
 */
void printConfigFile(){
	FILE *conf;
	conf=fopen("./dtnfog.conf","w");
	if(conf==NULL){
		error_print("Impossible to create dtnfog.conf\n");
		proxy_exit(SIGINT);
	}
	fprintf(conf,"##list-commands##\n");
	fprintf(conf,"<wire here allowed commands>\n");
	fprintf(conf,"<command 1>\n");
	fprintf(conf,"<command 2>\n");
	fprintf(conf,"<command 3>\n");
	fprintf(conf,"##end-list-commands##\n");
	fprintf(conf,"\n");
	fprintf(conf,"##cloud-mode##\n");
	fprintf(conf,"<enable cloud mode (true,false)>\n");
	fprintf(conf,"##cloud-mode##\n");
	fprintf(conf,"\n");
	fprintf(conf,"##next-address##\n");
	fprintf(conf,"<wire here address of next node>\n");
	fprintf(conf,"##end-next-address##\n");
	fprintf(conf,"\n");
	fprintf(conf,"##tcp-interface##\n");
	fprintf(conf,"<wire here name of tcp interface es eth0>\n");
	fprintf(conf,"##end-tcp-interface##\n");
	fprintf(conf,"\n");
	fprintf(conf,"##allow-reports## \n");
	fprintf(conf,"<allows to use address as sign in report.rp>\n");
	fprintf(conf,"##end-allow-reports##\n");
	fprintf(conf,"\n");
	fclose(conf);
}

char * getMyIp(char * interface){
	int fd, rc;
	  struct ifreq ifr;
	  fd = socket(AF_INET, SOCK_DGRAM, 0);
	  ifr.ifr_addr.sa_family = AF_INET; //ipv4;
	  strncpy(ifr.ifr_name, interface, IFNAMSIZ-1);
	  rc = ioctl(fd, SIOCGIFADDR, &ifr);
	  close(fd);
	  if(rc != 0) {
	    fprintf(stderr, "Something went wrong. Does the interface exist?\n");
	    exit(0);
	    return NULL;
	  }
	  else
		  //printf("%s\n", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
		  return inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);
}

/*
 * Function select correct protocol to communicate to cloud
 * */
void selectCommunicationAddress(){
	if(strstr(calcol_unit_info.mycloud, "ipn:") != NULL || strstr(calcol_unit_info.mycloud, "dtn://") != NULL){//is dtn eid
		strcpy(calcol_unit_info.communication_address,myEid.uri);
		printf("Next hop uses a dtn eis, use: %s\n",calcol_unit_info.communication_address);
	}else{
		strcpy(calcol_unit_info.communication_address,myIp);
		printf("Next hop a ip address, use: %s\n",calcol_unit_info.communication_address);
	}
}
