/*	bpcp.c:	bpcp, a remote copy utility that utilizes CFDP
 *
 *	Copyright (c) 2012, California Institute of Technology.
 *	All rights reserved.
 *	Author: Samuel Jero <sj323707@ohio.edu>, Ohio University
 */
#include "bpcp.h"



char * remote_path(char *cp);
int is_dir(char *cp);
int open_remote_dir(char *host, char *dir);
char* read_remote_dir(int dir, int index, char* buf, int size);
int read_remote_dir_v2(int dir, int index, BpcpDirEntry *out);
int close_remote_dir(int dir);
void toremote(char *targ, int argc, char **argv);
void tolocal(int argc, char **argv);
void ion_cfdp_init(void);
int ion_cfdp_put(struct transfer *t);
int ion_cfdp_get(struct transfer *t);
int ion_cfdp_rput(struct transfer *t);
int local_cp(struct transfer* t);
static int do_local_cmd(char *cmdln);
void manage_src(struct transfer *t);
void manage_dest(struct transfer* t);
void transfer(struct transfer *t);
void* rcv_msg_thread(void* param);
void dbgprintf(int level, const char *fmt, ...);
void usage(void);
void version(void);
void print_parsed(struct transfer* t);
void exit_nicely(int val);
void prog_start_cpy(struct transfer *t);
void prog_end_cpy(struct transfer *t);
void prog_start_dir(struct transfer *t);
void prog_end_dir(struct transfer *t);
int setscreensize(void);
static void parseDirectoryListingResponse(unsigned char *text, int bytesRemaining,
		CfdpDirListingResponse *opsData);
#ifdef SIG_HANDLER
static void handle_sigterm(int signum);
#endif

/*Command line flags*/
int showprogress = 1;		/* Set to zero to disable progress meter */
int debug = 0;			/*Set to non-zero to enable debug output. */
int iamrecursive;		/*Copy Recursively*/
int follow_symlinks = 0;	/*-F: follow symlinks during -r*/

/*Set to 1 if Target is a Directory*/
int targetshouldbedirectory = 0;

/*Counters used to surface silent-failure modes at exit.*/
int incomplete_listings_seen = 0;
int truncated_entries_skipped = 0;
int symlinks_skipped = 0;
int non_regular_skipped = 0;
int file_fetch_failures = 0;

/*State of the most recent directory listing response.  Set by the
 *receiver thread; consumed by open_remote_dir() and the caller
 *deciding which manifest reader to use.*/
int dirlist_was_v2 = 0;
int dirlist_incomplete = 0;

/*CFDP general request information structure*/
CfdpReqParms	parms;

/*Event waiting stuff*/
sm_SemId	events_sem;	/*Semaphore to wait for events on*/
enum wait_status 	current_wait_status;	/*Wait Status*/
CfdpTransactionId* 	event_wait_id;	/*Pointer to transaction ID to wait for*/
CfdpDirListTask 	dirlst;		/*Directory Listing info*/
char 		tmp_files[NUM_TMP_FILES][255];		/*tmp filename array*/

/*Receiver Thread variables*/
int recv_running;		/*Thread running flag*/
pthread_t rcv_thread;		/*Pthread variable*/

/*Start Here*/
#if defined (ION_LWT)
int	bpcp(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	int	t;
	char*	argv[5];
	int	argc;

	/*Initialize CFDP*/
	ion_cfdp_init();

	/*Recursive flag is a1*/
	iamrecursive = atoi((char*)a1);
	if(iamrecursive!=0 && iamrecursive!=1)
	{
		iamrecursive=0;
	}

	/*Pretty progress meter always disabled*/
	showprogress=0;

	/*Lifetime is a2. a2=0 results in default lifetime.*/
	t=strtol((char*)a2, NULL, 10);
	if (t > 0)
	{
		parms.utParms.lifespan=t;
	}

	/*Custody Switch is a3. 1=ON, 0=OFF*/
	t = atoi((char*)a3);
	if(t==1)
	{
		parms.utParms.custodySwitch = SourceCustodyRequired;
	}
	else
	{
		if(t==0)
		{
			parms.utParms.custodySwitch = NoCustodyRequested;
		}
	}

	/*Class of Service is a4.*/
	t=strtol((char*)a4, NULL, 10);
	if (t>=0 && t <= 2)
	{
		parms.utParms.classOfService=t;
	}

	/*Debug flag is a5.*/
	debug=atoi((char*)a5);
	if(debug>0)
	{
		version();
	}

	/*a6-a10 are files to copy/destinations*/
	argc=0;
	if((char*)a6!=NULL)
	{
		argv[argc]=(char*)a6;
		argc++;
	}
	if((char*)a7!=NULL)
	{
		argv[argc]=(char*)a7;
		argc++;
	}
	if((char*)a8!=NULL)
	{
		argv[argc]=(char*)a8;
		argc++;
	}
	if((char*)a9!=NULL)
	{
		argv[argc]=(char*)a9;
		argc++;
	}
	if((char*)a10!=NULL)
	{
		argv[argc]=(char*)a10;
		argc++;
	}

#else
int main(int argc, char **argv)
{
	int ch;
	extern char *optarg;
	extern int optind;
	int tmpoption;


	/*Initialize CFDP*/
	ion_cfdp_init();

	/*Parse commandline options*/
	while ((ch = getopt(argc, argv, "dqrFL:C:S:v")) != -1)
	{
		switch (ch)
		{
			case 'r':
				/*Recursive*/
				iamrecursive = 1;
				break;
			case 'F':
				/*Follow symlinks during recursive copy*/
				follow_symlinks = 1;
				break;
			case 'd':
				/*Debug*/
				debug++;
				break;
			case 'v':
				/*Print Version info*/
				version();
				break;
			case 'q':
				/*Quiet*/
				showprogress = 0;
				break;
			case 'L':
				/*Lifetime*/
				tmpoption=-1;
				tmpoption=strtol(optarg, NULL, 10);
				if (tmpoption > 0)
				{
					parms.utParms.lifespan=tmpoption;
				}
				else
				{
					dbgprintf(0,
						"Error: Invalid BP Lifetime\n");
					exit_nicely(1);
				}
				break;
			case 'C':
				/*Custody Transfer*/
				if (strcmp(optarg, "Yes")==0 ||
					strcmp(optarg, "YES")==0 ||
					strcmp(optarg, "yes")==0 ||
					strcmp(optarg, "y")==0 ||
					strcmp(optarg, "On")==0 ||
					strcmp(optarg, "ON")==0 ||
					strcmp(optarg, "on")==0 ||
					strcmp(optarg, "1")==0)
				{
					parms.utParms.custodySwitch
						= SourceCustodyRequired;
				}
				else
				{
					if (strcmp(optarg, "No")==0 ||
						strcmp(optarg, "NO")==0 ||
						strcmp(optarg, "yes")==0 ||
						strcmp(optarg, "n")==0 ||
						strcmp(optarg, "Off")==0 ||
						strcmp(optarg, "OFF")==0 ||
						strcmp(optarg, "off")==0 ||
						strcmp(optarg, "0")==0)
					{
						parms.utParms.custodySwitch
							= NoCustodyRequested;
					}
					else
					{
						dbgprintf(0, "Error: Invalid \
Custody Transfer Setting\n");
					}
				}

				break;
			case 'S':
				/*Class of Service*/
				tmpoption=-1;
				tmpoption=strtol(optarg, NULL, 10);
				if (tmpoption>=0 && tmpoption <= 2)
				{
					parms.utParms.classOfService=tmpoption;
				}
				else
				{
					dbgprintf(0, "Error: Invalid BP Class \
of Service\n");
					exit_nicely(1);
				}
				break;
			default:
				usage();
		}
	}

	argc -= optind;
	argv += optind;
#endif
	char *targ;

	/*Initialize tmp file array*/
	memset(tmp_files,0, NUM_TMP_FILES*255);

#ifdef SIG_HANDLER
	/*Set SIGTERM and SIGINT handlers*/
	isignal(SIGTERM, handle_sigterm);
	isignal(SIGINT, handle_sigterm);
#endif

	/*Additional argument checks*/
	if (!isatty(STDOUT_FILENO))
	{
		showprogress = 0;
	}

	if (argc < 2)
	{
		usage();
	}

	if (argc > 2)
	{
		/*	We are moving multiple files, destination must
		 *	be a directory.					*/

		targetshouldbedirectory = 1;
	}

	/*Connect to CFDP*/
	if (cfdp_attach() < 0)
	{
		dbgprintf(0, "Error: Can't initialize CFDP. Is ION running?\n");
		exit(1);
	}

	/*Create receiver thread*/
	events_sem=sm_SemCreate(SM_NO_KEY, SM_SEM_FIFO);
	if (events_sem==SM_SEM_NONE || sm_SemTake(events_sem)<0)
	{
		dbgprintf(0, "Error: Can't create semaphore\n");
		exit(1);
	}

	recv_running=1;
	if (pthread_begin(&rcv_thread, NULL, &rcv_msg_thread,
				(void*) &recv_running))
	{
		dbgprintf(0, "Error: Can't start message thread\n");
		sm_SemEnd(events_sem);
		microsnooze(50000);
		sm_SemDelete(events_sem);
		events_sem = SM_SEM_NONE;
		exit(1);
	}

	/*Parse Paths*/
	if ((targ = remote_path(argv[argc - 1])))
	{
		/* Last path is remote path
		 * Destination is remote host*/
		toremote(targ, argc, argv);
	}
	else
	{
		/*Destination is localhost*/
		if (targetshouldbedirectory)
		{
			/*	If we are moving multiple files, check
			 *	that destination is directory.		*/

			if (is_dir(argv[argc - 1]))
			{
				tolocal(argc, argv);
			}
			else
			{
				dbgprintf(0, "Error: Destination is not a \
directory\n");
				exit_nicely(1);
			}
		}
		else
		{
			/*Single file copy*/
			tolocal(argc, argv);
		}
	}


	exit_nicely(0);
	return 0;
}

/*Returns a pointer to the start of a remote path (a colon). Returns null if the
 * path isn't remote (no colon). */
char* remote_path(char *cp)
{
	if (*cp == ':')
	{
		/* Leading colon might be part of filename. */
		return NULL;
	}

	for (; *cp; ++cp)
	{
		if (*cp == ':')
		{
			/* Found colon*/
			return (cp);
		}

		if (*cp == '/')
		{
			/*Path separator. definitely filename*/
			return NULL;
		}
	}

	return NULL;
}

/*Returns non-zero if the given filename is a directory. Returns
 * zero otherwise.*/
int is_dir(char *cp)
{
	struct stat stb;

	if (!stat(cp, &stb))
	{
		if (S_ISDIR(stb.st_mode))
		{
			return 1;
		}
	}

	return 0;
}

/*Opens the given directory on the given host. Returns -1 on error.
 * Returns a handle to the directory (>0) on success.
 * Note: This function my sleep an arbitrary amount of time while the
 * directory information is being fetched from the remote host.*/
int open_remote_dir(char *host, char *dir)
{
	int res;
	uvast entityId;
	char	template[] = "dldfnXXXXXX";
	int	tempfd;
	char* tmp;
	int hndl;

	/*Sanity checks*/
	if (host==NULL || dir==NULL)
	{
		dbgprintf(0, "Warning: Can't copy file: %s\n", dir);
		return -2;
	}

	/*Setup parameters*/
	entityId=getFqn(host);
	cfdp_compress_number(&parms.destinationEntityNbr, entityId);

	/*Pick a temp file name*/
	for (hndl=0; hndl < NUM_TMP_FILES && strlen(tmp_files[hndl]); hndl++)
	{
		/*empty*/
	}

	if (hndl > TMP_MAX || hndl >= NUM_TMP_FILES || strlen(tmp_files[hndl]))
	{
		dbgprintf(0, "Error: Too Many Directories Open\n");
		exit_nicely(1);
	}

	oK(umask(S_IWGRP | S_IWOTH));
	tempfd = mkstemp(template);
	if (tempfd < 0)
	{
		dbgprintf(0, "Error: Can't Get Temporary Filename\n");
		exit_nicely(1);
	}

	unlink(template);	/*	Just need name, not the file.	*/
	tmp = template;
	strncpy(tmp_files[hndl], tmp, 255);

	/*Setup Dir list*/
	dirlst.destFileName=tmp_files[hndl];	/*	Use persistent storage, not stack.	*/
	dirlst.directoryName=dir;

	if(strlen(tmp) + strlen(dir) + strlen("cfdp")+ 1 >= 255)
	{
		dbgprintf(0, "Error: Can't query, filename too long: %s\n", dir);
		return -2;
	}

	dbgprintf(3, "Host: %s\n", host);
	dbgprintf(3, "Dir: %s\n", dir);
	dbgprintf(3, "Tmp: %s\n", tmp);

	/*Make request.  Opt in to the v2 format so we can detect
	 *incomplete listings and per-entry types.  Old responders
	 *ignore the trailing options byte and reply with the legacy
	 *format; the receiver thread distinguishes them by the
	 *response message type (17 vs 19).*/
	current_wait_status=dir_req;
	dirlist_was_v2 = 0;
	dirlist_incomplete = 0;
	res = cfdp_rls_extended(&(parms.destinationEntityNbr),
			sizeof(BpUtParms),
			(unsigned char *) &(parms.utParms),
			NULL,
			NULL, 0,
			parms.faultHandlers, 0, NULL, 0,
			parms.msgsToUser,
			parms.fsRequests,
			&dirlst,
			CFDP_DIRLIST_OPTION_STATUS_BYTES,
			&(parms.transactionId));

	/*Handle Error*/
	if (res<0)
	{
		dbgprintf(0, "Error: CFDP error on %s\n",dir);
		exit_nicely(1);
	}

	dbgprintf(1, "Requested Dir listing for: %s\n", dir);

	/*Cleanup*/
	parms.msgsToUser = 0;
	parms.fsRequests = 0;

	/*Sleep waiting to receive directory listing*/
	if (sm_SemTake(events_sem)<0)
	{
		dbgprintf(0, "Error: Can't take directory semaphore\n");
		current_wait_status=no_req;
		return -2;
	}

	if (current_wait_status==nodir)
	{
		/*No file or file isn't a directory*/
		current_wait_status=no_req;
		return -1;
	}

	if (current_wait_status==dir_exists)
	{
		/*Directory listing received.  If the responder
		 *signaled an incomplete listing, count it now;
		 *the final summary will surface this even if all
		 *individually-returned entries fetch successfully.*/
		if (dirlist_incomplete)
		{
			incomplete_listings_seen++;
			dbgprintf(0, "bpcp: warning: listing of %s is "
					"incomplete (responder dropped "
					"entries; see ion.log on remote)\n",
					dir);
		}
		current_wait_status=no_req;
		return hndl;
	}

	return -2;
}

/*Takes a given directory handle and returns the index(th) directory entry
 * (filename) in buf (upto size characters). Returns buf on success
 * and null on error.*/
char* read_remote_dir(int dir, int index, char* buf, int size){
	static
	int fd;
	char c;
	int eof=0;
	int i=0;

	/*Sanity Checks*/
	if (buf==NULL || dir<0 || dir >= NUM_TMP_FILES)
	{
		return NULL;
	}

	/*Memset buffer*/
	memset(buf, 0, size);

	/*Open temporary directory file*/
	fd=iopen(tmp_files[dir], O_RDONLY, 0);
	if (fd<0)
	{
		dbgprintf(0, "Error: Couldn't get directory listing\n");
		exit_nicely(1);
	}

	/*find requested directory entry*/
	for (i=0; i < index; i++) {
		while (1)
		{
			if(read(fd, &c, 1)<=0){
				eof=1;
				break;
			}
			if (c==0)
			{
				break;
			}
		}
		if(eof){
			break;
		}
	}

	/*Read requested directory entry*/
	if (!eof)
	{
		for (i=0; i < size && !eof; i++)
		{
			if(read(fd, &c, 1)<=0){
				eof=1;
				break;
			}
			if (c==0)
			{
				buf[i]=0;
				break;
			}
			else
			{
				buf[i]=c;
			}
		}
	}
	else
	{
		/*End of Directory.*/
		close(fd);
		return NULL;
	}

	/*	The read loop above can exit with i == size and no
	 *	NUL written into buf (when an entry is at least as
	 *	long as the caller's buffer).  Guarantee a terminator
	 *	before any strlen() walks past the buffer.		*/
	buf[size - 1] = '\0';

	if(strlen(buf)==0)
	{
		return NULL;
	}

	/*Return entry*/
	close(fd);
	return buf;
}

/*Reads the index'th entry from a v2 manifest into *out.  Returns:
 *  0 on success (entry filled),
 *  1 at end of listing,
 * -1 on read error.
 * The buffer in BpcpDirEntry is sized so an over-buffer truncation
 * is impossible given the responder's CFDP_DIRLIST_MAX_NAME cap.*/
int read_remote_dir_v2(int dir, int index, BpcpDirEntry *out)
{
	int		fd;
	int		i;
	unsigned char	header[2];
	char		c;
	int		eof = 0;
	ssize_t		n;

	if (out == NULL || dir < 0 || dir >= NUM_TMP_FILES)
	{
		return -1;
	}

	memset(out, 0, sizeof *out);

	fd = iopen(tmp_files[dir], O_RDONLY, 0);
	if (fd < 0)
	{
		dbgprintf(0, "Error: Couldn't get directory listing\n");
		return -1;
	}

	/*Skip past the first 'index' records.*/
	for (i = 0; i < index; i++)
	{
		/*Read past 2-byte header.*/
		n = read(fd, header, 2);
		if (n <= 0)
		{
			eof = 1;
			break;
		}
		if (n != 2)
		{
			close(fd);
			return -1;
		}
		/*Skip past name + NUL.*/
		while (1)
		{
			n = read(fd, &c, 1);
			if (n <= 0)
			{
				eof = 1;
				break;
			}
			if (c == '\0')
			{
				break;
			}
		}
		if (eof)
		{
			break;
		}
	}

	if (eof)
	{
		close(fd);
		return 1;
	}

	/*Read this record.*/
	n = read(fd, header, 2);
	if (n <= 0)
	{
		close(fd);
		return 1;	/*	Clean EOF.			*/
	}
	if (n != 2)
	{
		close(fd);
		return -1;
	}

	out->status = header[0];
	out->type = header[1];

	for (i = 0; i < (int) (sizeof out->name) - 1; i++)
	{
		n = read(fd, &c, 1);
		if (n <= 0)
		{
			break;
		}
		if (c == '\0')
		{
			break;
		}
		out->name[i] = c;
	}
	/*Always NUL-terminate, even if the read loop above filled
	 *the buffer without finding a NUL.*/
	out->name[sizeof out->name - 1] = '\0';

	close(fd);
	return 0;
}

/*Closes a remote file given a directory handle. Returns -1 on
 * error and 0 on success.*/
int close_remote_dir(int dir)
{
	if (dir <= 0 || dir >= NUM_TMP_FILES)
	{
		return -1;
	}

	if (unlink(tmp_files[dir])<0)
	{
		memset(tmp_files[dir],0,255);
		return -1;
	}
	memset(tmp_files[dir],0,255);

	return 0;
}

/*Copies files to the (targ) remote directory. Takes argc, argv
 * from command line starting at first path argument.*/
void toremote(char *targ, int argc, char **argv)
{
	char *host, *src, *thost;
	struct transfer t;
	int i;

	/*Skip colon character. If no directory specified, use local directory*/
	*targ++ = 0;
	if (*targ == 0)
	{
		targ = ".";
	}

	/*Set remote host to be beginning of last arg*/
	thost = argv[argc - 1];

	/*Copy all files*/
	for (i = 0; i < argc - 1; i++)
	{
		src = remote_path(argv[i]);
		if (src) {
			/*	If first argument is a remote path,
			 *	this is a remote to remote copy.	*/

			/*	Skip colon character. If no directory
			 *	specified, use local directory.		*/

			*src++ = 0;
			if (*src == 0)
			{
				src = ".";
			}

			/*	Host is the beginning of this argument
			 *	(part prior to colon).			*/

			host = argv[i];

			/*Do copy*/
			t.type=Remote_Remote;
			snprintf(t.sfile, 255, "%.254s", src);
			snprintf(t.shost, 255, "%.254s", host);
			snprintf(t.dfile, 255, "%.254s", targ);
			snprintf(t.dhost, 255, "%.254s", thost);
			manage_src(&t);

		}
		else
		{
			/*local to remote copy*/

			/*Do copy*/
			src=argv[i];
			t.type=Local_Remote;
			snprintf(t.sfile, 255, "%.254s", src);
			memset(t.shost, 0, 256);
			snprintf(t.dfile, 255, "%.254s", targ);
			snprintf(t.dhost, 255, "%.254s", thost);
			manage_src(&t);
		}
	}
}

/*Copies a file from another machine to this one. Takes argc, argv
 * from command line starting at first path argument.*/
void tolocal(int argc, char **argv)
{
	char *host, *src, *targ;
	int i;
	struct transfer t;

	/*Copy all files*/
	for (i = 0; i < argc -1; i++) {
		if (!(src = remote_path(argv[i])))
		{
			/*If first path is not a remote path, this is a
			 * local to local copy. Just call cp.*/
			t.type=Local_Local;
			snprintf(t.sfile, 255, "%.254s", argv[i]);
			memset(t.shost, 0, 256);
			snprintf(t.dfile, 255, "%.254s", argv[argc-1]);
			memset(t.dhost, 0,256);

			/*Do Copy*/
			transfer(&t);

		}
		else
		{
			/*Remote to Local copy*/

			/*	Skip colon character. If no directory
			 *	specified, use local directory.		*/
			*src++ = 0;
			if (*src == 0)
			{
				src = ".";
			}

			/*	Host is the beginning of this argument
			 *	(part prior to colon).			*/

			host = argv[i];

			/*Target is the last argument, a local file*/

			targ=*(argv + argc - 1);

			/*Do Copy*/

			t.type=Remote_Local;
			snprintf(t.sfile, 255, "%.254s", src);
			snprintf(t.shost, 255, "%.254s", host);
			snprintf(t.dfile, 255, "%.254s", targ);
			memset(t.dhost, 0,256);
			manage_src(&t);
		}
	}
}

/*Initialize ION and CFDP. Exits program on error.*/
void ion_cfdp_init(void)
{
	int i=0;
	memset((char *) &parms, 0, sizeof(CfdpReqParms));
	cfdp_compress_number(&parms.destinationEntityNbr, 0);

	/*Set Default BP parameters.*/
	parms.utParms.lifespan = 86400;
	parms.utParms.classOfService = BP_STD_PRIORITY;
	parms.utParms.custodySwitch = NoCustodyRequested;

	/*Setup fault handlers. Required to ensure recursive copy and file
	 * listings work durably.*/
	for (i = 0; i < 16; i++)
	{
		parms.faultHandlers[i] = CfdpIgnore;
	}
	parms.faultHandlers[CfdpFilestoreRejection] = CfdpIgnore;
	parms.faultHandlers[CfdpCheckLimitReached] = CfdpCancel;
	parms.faultHandlers[CfdpChecksumFailure] = CfdpCancel;
	parms.faultHandlers[CfdpInactivityDetected] = CfdpCancel;
	parms.faultHandlers[CfdpFileSizeError] = CfdpCancel;
}

/*Copy a local file to a remote file. Takes a local file,
 * a remote host, and remote file.
 * Exits program on CFDP error. Returns -1 on other errors.*/
int ion_cfdp_put(struct transfer* t)
{
	int res;
	uvast entityId;

	/*Sanity checks*/
	if (t==NULL || t->dfile[0] == 0 || t->dhost[0] == 0 || t->sfile[0] == 0)
	{
		dbgprintf(0, "Warning: Can't copy file: %s\n", t->dfile);
		return -1;
	}
	print_parsed(t);

	/*Setup parameters*/
	entityId=getFqn(t->dhost);
	cfdp_compress_number(&parms.destinationEntityNbr, entityId);
	memset((char*)&parms.transactionId, 0 , sizeof(CfdpTransactionId));
	snprintf(parms.sourceFileNameBuf, 255, "%.254s", t->sfile);
	snprintf(parms.destFileNameBuf, 255, "%.254s",t->dfile);
	parms.sourceFileName=parms.sourceFileNameBuf;
	parms.destFileName=parms.destFileNameBuf;

	/*Make request*/
#ifdef SERIAL
	event_wait_id=&parms.transactionId;
	current_wait_status=snd_wait;
#endif
	res = cfdp_put(&(parms.destinationEntityNbr),
					sizeof(BpUtParms),
					(unsigned char *) &(parms.utParms),
					parms.sourceFileName,
					parms.destFileName, NULL, NULL,
					parms.faultHandlers, 0, NULL, 0,
					parms.msgsToUser,
					parms.fsRequests,
					&(parms.transactionId));


	/*Handle Error*/
	if (res<0) {
		dbgprintf(0, "Error: CFDP error on %s\n",t->dfile);
#ifdef SERIAL
		current_wait_status=no_req;
		event_wait_id=NULL;
#endif
		return -1;
	}

#ifdef SERIAL
	/*Sleep waiting for EOF event*/
	if(current_wait_status==snd_wait)
	{
		if (sm_SemTake(events_sem)<0)
		{
			dbgprintf(0, "Error: Can't take EOF semaphore\n");
			current_wait_status=no_req;
			event_wait_id=NULL;
			return -1;
		}
	}

	if (current_wait_status==sent)
	{
		/*No Error*/
		current_wait_status=no_req;
		event_wait_id=NULL;
	}
	else
	{
		/*Error*/
		dbgprintf(0, "Terminated\n");
		return -1;
	}
#endif

	dbgprintf(1, "Sent: %s\n", parms.sourceFileName);

	/*Cleanup*/
	parms.msgsToUser = 0;
	parms.fsRequests = 0;
	memset((char*)&parms.transactionId, 0 , sizeof(CfdpTransactionId));
	return 0;
}

/*Copies a remote file to a local file. Takes a remote host,
 * a remote file, and a local file.
 * Returns -1 on error.*/
int ion_cfdp_get(struct transfer* t)
{
	int res;
	uvast entityId;

	/*Sanity checks*/
	if (t==NULL || t->dfile[0] == 0 || t->shost[0] == 0 || t->sfile[0] == 0)
	{
		dbgprintf(0, "Warning: Can't copy file: %s\n", t->dfile);
		return -1;
	}
	print_parsed(t);

	/*Length checks*/
	if(strlen(t->sfile) + strlen(t->dfile) +strlen("cfdp")+ 10 >= 255)
	{
			dbgprintf(0, "Error: Can't copy, filename too long: \
%s\n", t->sfile);
			return -1;
	}

	/*Setup parameters*/
	entityId=getFqn(t->shost);
	cfdp_compress_number(&parms.destinationEntityNbr, entityId);
	memset((char*)&parms.transactionId, 0 , sizeof(CfdpTransactionId));
	snprintf(parms.sourceFileNameBuf, 255, "%.254s", t->sfile);
	snprintf(parms.destFileNameBuf, 255, "%.254s",t->dfile);
	parms.sourceFileName=parms.sourceFileNameBuf;
	parms.destFileName=parms.destFileNameBuf;

	/*Setup CFDP proxy structure*/
	parms.proxytask.destFileName=parms.destFileNameBuf;
	parms.proxytask.faultHandlers=parms.faultHandlers;
	parms.proxytask.filestoreRequests=parms.fsRequests;
	parms.proxytask.messagesToUser=0;
	parms.proxytask.recordBoundsRespected=0;
	parms.proxytask.sourceFileName=parms.sourceFileNameBuf;
	parms.proxytask.unacknowledged=1;

	/*Make request*/
#ifdef SERIAL
	event_wait_id=&parms.transactionId;
	current_wait_status=snd_wait;
#endif
	res = cfdp_get(&(parms.destinationEntityNbr),
				sizeof(BpUtParms),
				(unsigned char *) &(parms.utParms),
				NULL,
				NULL, 0,
				parms.faultHandlers, 0, NULL, 0,
				parms.msgsToUser,
				0, &parms.proxytask,
				&(parms.transactionId));

	/*Handle Error*/
	if (res<0)
	{
		dbgprintf(0, "Error: CFDP error on: %s\n",t->sfile);
#ifdef SERIAL
		current_wait_status=no_req;
		event_wait_id=NULL;
#endif
		return -1;
	}

#ifdef SERIAL
	/*Sleep waiting for EOF event*/
	if(current_wait_status==snd_wait)
	{
		if (sm_SemTake(events_sem)<0)
		{
			dbgprintf(0, "Error: Can't take EOF semaphore\n");
			current_wait_status=no_req;
			event_wait_id=NULL;
			return -1;
		}
	}

	if (current_wait_status==sent)
	{
		/*No Error*/
		current_wait_status=no_req;
		event_wait_id=NULL;
	}
	else
	{
		/*Error*/
		dbgprintf(0, "Terminated\n");
		return -1;
	}
#endif

	/*Cleanup*/
	parms.msgsToUser = 0;
	parms.fsRequests = 0;
	memset((char*)&parms.transactionId, 0 , sizeof(CfdpTransactionId));
	return 0;
}

/*Copies a remote file to another host. Takes a source host and file
 * and a destination host and file.
 * Returns -1 on error.*/
int ion_cfdp_rput(struct transfer* t)
{
	int res;
	uvast entityId;
	CfdpNumber src;

	/*Sanity checks*/
	if (t==NULL || t->shost[0] == 0 || t->sfile[0] == 0
	|| t->dhost[0] == 0 || t->dfile[0] == 0)
	{
		dbgprintf(0, "Warning: Can't copy file: %s\n", t->sfile);
		return -1;
	}
	print_parsed(t);

	/*Length checks*/
	if(strlen(t->sfile) + strlen(t->dfile) + strlen("cfdp")+ 10 >= 255)
	{
			dbgprintf(0, "Error: Can't copy, filename too long: \
%s\n", t->sfile);
			return -1;
	}

	/*Setup parameters*/
	entityId=getFqn(t->dhost);
	cfdp_compress_number(&parms.destinationEntityNbr, entityId);
	memset((char*)&parms.transactionId, 0 , sizeof(CfdpTransactionId));
	entityId=getFqn(t->shost);
	cfdp_compress_number(&src, entityId);
	snprintf(parms.sourceFileNameBuf, 255, "%.254s", t->sfile);
	snprintf(parms.destFileNameBuf, 255, "%.254s",t->dfile);
	parms.sourceFileName=parms.sourceFileNameBuf;
	parms.destFileName=parms.destFileNameBuf;

	/*Setup CFDP proxy structure*/
	parms.proxytask.destFileName=parms.destFileNameBuf;
	parms.proxytask.faultHandlers=parms.faultHandlers;
	parms.proxytask.filestoreRequests=parms.fsRequests;
	parms.proxytask.messagesToUser=0;
	parms.proxytask.recordBoundsRespected=0;
	parms.proxytask.sourceFileName=parms.sourceFileNameBuf;
	parms.proxytask.unacknowledged=1;

	/*Make request*/
#ifdef SERIAL
	event_wait_id=&parms.transactionId;
	current_wait_status=snd_wait;
#endif
	res = cfdp_rput(&src, sizeof(BpUtParms),
			(unsigned char *) &(parms.utParms),
			NULL, NULL, 0,
			parms.faultHandlers,
			0, NULL, 0,
			parms.msgsToUser,
			0, &(parms.destinationEntityNbr), &parms.proxytask,
			&(parms.transactionId));

	/*Handle Error*/
	if (res<0) {
		dbgprintf(0, "Error: CFDP error on: %s\n",t->sfile);
#ifdef SERIAL
		current_wait_status=no_req;
		event_wait_id=NULL;
#endif
		return -1;
	}

#ifdef SERIAL
	/*Sleep waiting for EOF event*/
	if(current_wait_status==snd_wait)
	{
		if (sm_SemTake(events_sem)<0)
		{
			dbgprintf(0, "Error: Can't take EOF semaphore\n");
			current_wait_status=no_req;
			event_wait_id=NULL;
			return -1;
		}
	}

	if (current_wait_status==sent)
	{
		/*	EOF sent for proxy request. Now wait for the proxy
		 *	request to be actually processed (handleProxyPutRequest
		 *	called), with a timeout to avoid blocking forever.
		 *	This prevents a race condition where bpcp exits before
		 *	the last proxy request is processed.			*/
		int timeout_ms = 2000;
		int wait_interval_ms = 50;
		int elapsed = 0;

		current_wait_status = proxy_wait;

		while (current_wait_status == proxy_wait && elapsed < timeout_ms)
		{
			microsnooze(wait_interval_ms * 1000);
			elapsed += wait_interval_ms;
		}

		if (current_wait_status == proxy_done)
		{
			/*	Proxy request was processed successfully.	*/
			dbgprintf(3, "Proxy request processed for %s\n", t->sfile);
		}
		else
		{
			/*	Timeout - proxy may or may not have been
			 *	processed. Log warning but continue.		*/
			dbgprintf(1, "Warning: Timeout waiting for proxy \
processing for %s\n", t->sfile);
		}

		current_wait_status = no_req;
		event_wait_id = NULL;
	}
	else
	{
		/*Error*/
		dbgprintf(0, "Terminated\n");
		return -1;
	}
#endif

	/*Cleanup*/
	parms.msgsToUser = 0;
	parms.fsRequests = 0;
	memset((char*)&parms.transactionId, 0 , sizeof(CfdpTransactionId));
	return 0;
}

/*Copy files locally using cp. Let's cp handle recursion.
 * Returns -1 on error.*/
int local_cp(struct transfer* t)
{
	char work[1024];
	char cmd[256];
	char *r;

	/*Sanity checks*/
	if (t==NULL  || t->sfile[0] == 0 || t->dfile[0] == 0)
	{
		dbgprintf(0, "Warning: Can't copy file: %s\n", t->sfile);
		return -1;
	}
	print_parsed(t);

	/*Create arguments to cp*/
	if(iamrecursive)
	{
		r="-r ";
	}else{
		r="";
	}
	memset(work,0,sizeof work);
	snprintf(work,sizeof work,"%s %s%s %s", "cp", r, t->sfile, t->dfile);
	istrcpy(cmd, work, sizeof cmd);

	/*Run CP*/
	if (do_local_cmd(cmd)<0)
	{
		dbgprintf(0, "Error: Can't copy: %s\n", t->sfile);
		return -1;
	}
return 0;
}

/*Execute a local command with argument array cmdln (containing
 * num number of args) and wait for it to exit. Returns -1 or
 * Exits program on error.*/
static int do_local_cmd(char *cmdln)
{
	int status = -1;
	int pid;

	/*Sanity checks*/
	if (cmdln==NULL)
	{
		dbgprintf(0, "do_local_cmd: no arguments\n");
		exit_nicely(1);
	}

	dbgprintf(1, "Executing: %s\n", cmdln);

	/*Execute*/
	pid=pseudoshell(cmdln);
	if(pid < 0)
	{
		dbgprintf(0,"Error executing: %s\n", cmdln);
		exit_nicely(1);
	}

	/*Parent waits for child to exit*/
	while (waitpid(pid, &status, 0) == -1 && recv_running==1)
	{
		if (errno != EINTR)
		{
			dbgprintf(0,"do_local_cmd: waitpid: %s\n",
					system_error_msg());
			exit_nicely(1);
		}
	}

	if(recv_running==0){
		kill(pid, SIGTERM);
		dbgprintf(0,"Terminated\n");
		exit_nicely(1);
	}

//#if defined (VXWORKS) || defined (RTEMS)
//	return 0; /*May be needed. I'm not sure how cross platform WIFEXITED, etc is*/
//#else
	/*Returns -1 on child process error exit*/
	if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
	{
		return (-1);
	}

	return (0);
//#endif
}

/*Normalizes source filenames and recursively copies directories.
 * Note: This function may update both the source filename and the
 * destination filename in order to recursively copy directories.*/
void manage_src(struct transfer *t)
{
	DIR *dirp;
	struct dirent *dp;
	struct transfer tt;
	char work[1024];
	struct stat statbuf;
	int dir;
	char buff[256];
	int i=0;

	if (t==NULL)
	{
		return;
	}

	/*Kill trailing slashes on directories*/
	if (t->sfile[strlen(t->sfile)-1]=='/')
	{
		t->sfile[strlen(t->sfile)-1]=0;
	}
	if (t->dfile[strlen(t->dfile)-1]=='/')
	{
		t->dfile[strlen(t->dfile)-1]=0;
	}


	if (t->type==Local_Local || t->type==Local_Remote)
	{
		/*Source is local*/
		if (is_dir(t->sfile))
		{
			/*Source is a directory*/
			if (iamrecursive)
			{
				if (!(dirp = opendir(t->sfile)))
				{
					dbgprintf(0, "bpcp: %s:%s\n", t->sfile,
							system_error_msg());
					return;
				}

				/*Loop over directory  copying all files in it*/
				while ((dp = readdir(dirp)) != NULL)
				{
					if (dp->d_ino == 0)
					{
						continue;
					}
					if (!strncmp(dp->d_name, ".",_D_EXACT_NAMLEN(dp)) || !strncmp(dp->d_name, "..",_D_EXACT_NAMLEN(dp)))
					{
						continue;
					}
					memset(buff,0,256);
					memcpy(buff, dp->d_name,_D_EXACT_NAMLEN(dp));
					if (strlen(t->sfile) + 1 + _D_EXACT_NAMLEN(dp) >= 255)
					{
						dbgprintf(0,"bpcp: %s/%s: name too long\n",t->sfile, buff);
						continue;
					}
					/*Ignore all non-regular, non-directory files*/
					if(stat(buff,&statbuf)<0)
					{
						dbgprintf(0, "bpcp: %s/%s--%s\n", t->sfile, buff, strerror(errno));
						continue;
					}
					if (!S_ISDIR(statbuf.st_mode) && !S_ISREG(statbuf.st_mode))
					{
						dbgprintf(0, "Warning: Ignoring non-regular file %s\n", buff);
						continue;
					}

					/*Update both source and destination so that recursive copy works*/
					memset(work, 0, sizeof work);
					snprintf(work, sizeof work, "%s/%s", t->sfile, dp->d_name);
					istrcpy(tt.sfile, work, 255);
					snprintf(tt.shost, 255, "%.254s", t->shost);
					memset(work, 0, sizeof work);
					snprintf(work, sizeof work, "%s/%s", t->dfile, dp->d_name);
					istrcpy(tt.dfile, work, 255);
					snprintf(tt.dhost, 255, "%.254s", t->dhost);
					tt.type=t->type;

					manage_src(&tt);
				}

				closedir(dirp);

			}
			else
			{
				dbgprintf(0, "bpcp: omitting directory %s\n", t->sfile);
				return;
			}
		}
		else
		{
			/*Source is a single file*/
			manage_dest(t);
		}
	}
	else
	{
		/*Source is remote*/
		prog_start_dir(t);
		dir=open_remote_dir(t->shost, t->sfile);
		if (dir>=0)
		{
			prog_end_dir(t);
			/*Source is directory*/
			if (iamrecursive)
			{
				int		v2 = dirlist_was_v2;
				BpcpDirEntry	v2entry;
				const char	*entryName;
				unsigned char	entryType;
				unsigned char	entryStatus;
				int		rc;

				/*Loop over directory copying all files in it.
				 *Use the v2 reader if the responder gave us a
				 *v2 manifest; otherwise fall back to the
				 *legacy name-only reader (today's behavior).*/
				for (i=0; ; i++)
				{
					if (v2)
					{
						rc = read_remote_dir_v2(dir, i, &v2entry);
						if (rc != 0)
						{
							break;
						}
						entryName = v2entry.name;
						entryType = v2entry.type;
						entryStatus = v2entry.status;
					}
					else
					{
						if (read_remote_dir(dir, i, buff, 256) == NULL)
						{
							break;
						}
						entryName = buff;
						entryType = CFDP_DIRENT_UNKNOWN;
						entryStatus = 0;
					}

					if (!strcmp(entryName, ".") || !strcmp(entryName, ".."))
					{
						continue;
					}
					if (strlen(t->sfile) + 1 + strlen(entryName) >= 255)
					{
						dbgprintf(0,"bpcp: %s/%s: name too long\n",t->sfile, entryName);
						continue;
					}

					/*Per-entry classification (v2 only;
					 *for legacy responses, type is UNKNOWN
					 *and we fall through to the existing
					 *manage_src round-trip).*/
					if (v2)
					{
						if (entryStatus & CFDP_DIRENT_NAME_TRUNCATED)
						{
							dbgprintf(0, "bpcp: skipping unreliable "
								"entry in %s: name truncated by "
								"remote\n", t->sfile);
							truncated_entries_skipped++;
							continue;
						}

						if (entryType == CFDP_DIRENT_SYMLINK
								&& !follow_symlinks)
						{
							dbgprintf(0, "bpcp: skipping symlink %s/%s "
								"(use -F to follow)\n",
								t->sfile, entryName);
							symlinks_skipped++;
							continue;
						}

						if (entryType == CFDP_DIRENT_OTHER
								|| entryType == CFDP_DIRENT_UNKNOWN)
						{
							dbgprintf(0, "bpcp: skipping non-regular "
								"entry %s/%s (type=%u)\n",
								t->sfile, entryName,
								(unsigned) entryType);
							non_regular_skipped++;
							continue;
						}
					}

					/*Update both source and destination so that recursive copy works*/
					memset(work, 0, sizeof work);
					snprintf(work, sizeof work, "%s/%s", t->sfile, entryName);
					istrcpy(tt.sfile, work, 255);
					snprintf(tt.shost, 255, "%.254s", t->shost);
					memset(work, 0, sizeof work);
					snprintf(work, sizeof work, "%s/%s", t->dfile, entryName);
					istrcpy(tt.dfile, work, 255);
					snprintf(tt.dhost, 255, "%.254s", t->dhost);
					tt.type=t->type;

					manage_src(&tt);
				}
				close_remote_dir(dir);
			}
			else
			{
				close_remote_dir(dir);
				dbgprintf(0, "bpcp: omitting directory %s\n", t->sfile);
				return;
			}
		}
		else
		{
			if (dir==-1)
			{
				prog_end_dir(t);
				/*Source is single file*/
				manage_dest(t);
			}
			else
			{
				dbgprintf(0, "Warning: Skipping %s\n", t->sfile);
			}
		}
	}

}

/*Normalize destination names and handle recursive copying*/
void manage_dest(struct transfer* t)
{
	struct transfer tt;
	char work[1024];
	char cwd[256];

	if (t==NULL)
	{
		return;
	}

	/*Warn about ~ in path name*/
	if (t->dfile[0]=='~')
	{
		dbgprintf(0, "Error: CFDP does not interpret ~ as referring to the user's home directory.\n \
\tUse ./~ if you really want a directory called ~.\n");

		exit_nicely(1);
	}

	if (t->type==Remote_Local || t->type==Local_Local)
	{
		/*Destination is Local*/
		if (t->dfile[0]=='/')
		{
			/*Given an absolute path*/
			transfer(t);
		}
		else
		{
			/*Relative Path. Prepend CWD*/
			if (igetcwd(cwd, sizeof(cwd))==NULL)
			{
				dbgprintf(0, "Error: Can't get current directory\n");
				return;
			}
			memcpy(&tt, t, sizeof(struct transfer));
			if (strlen(t->dfile) + 1 + strlen(cwd+1) >= 255)
			{
					dbgprintf(0,"bpcp: %s/%s: name too long\n",cwd,t->dfile);
					return;
			}
			memset(work, 0, sizeof work);
			snprintf(work, sizeof work, "%s/%s", cwd,t->dfile);
			istrcpy(tt.dfile, work, 255);

			transfer(&tt);
		}
	}
	else
	{
		/*Destination is Remote*/
		transfer(t);
	}
}

/* Execute file transfer based on type of transfer requested*/
void transfer(struct transfer *t)
{
	int res=0;

	prog_start_cpy(t);
	switch(t->type)
	{
		case Local_Local:
				res=local_cp(t);
				break;
		case Local_Remote:
				res=ion_cfdp_put(t);
				break;
		case Remote_Local:
				res=ion_cfdp_get(t);
				break;
		case Remote_Remote:
				res=ion_cfdp_rput(t);
				break;
	}
	if (res==0)
	{
		prog_end_cpy(t);
	}
}

/*CFDP Event Reception Thread. Takes a boolean int as a parameter.
 * Exits when param becomes 0.*/
void* rcv_msg_thread(void* param)
{
	int			*running=(int*)param;

	char			*eventTypes[] =	{
					"no event",
					"transaction started",
					"EOF sent",
					"transaction finished",
					"metadata received",
					"file data segment received",
					"EOF received",
					"suspended",
					"resumed",
					"transaction report",
					"fault",
					"abandoned"
						};
	CfdpEventType		type;
	time_t			time;
	int			reqNbr;
	CfdpTransactionId	transactionId;
	char			sourceFileNameBuf[256];
	char			destFileNameBuf[256];
	uvast			fileSize;
	MetadataList		messagesToUser;
	uvast			offset;
	unsigned int		length;
	unsigned int		recordBoundsRespected;
	CfdpContinuationState	continuationState;
	unsigned int		segMetadataLength;
	char			segMetadata[63];
	CfdpCondition		condition;
	uvast			progress;
	CfdpFileStatus		fileStatus;
	CfdpDeliveryCode	deliveryCode;
	CfdpTransactionId	originatingTransactionId;

	memset(&transactionId, 0, sizeof(CfdpTransactionId));
	memset(&originatingTransactionId, 0, sizeof(CfdpTransactionId));
	char			statusReportBuf[256];
	MetadataList		filestoreResponses;
	unsigned int	closureRequested;
	unsigned char		usrmsgBuf[256];
	CfdpDirListingResponse	dir_list_rsp;
	uvast 			TID11;
	uvast			TID12;
	uvast			TID21=0;
	uvast			TID22=0;
	char			nbrBuf[FQN_MAX_LENGTH];

	/*Main Event loop*/
	while (*running)
	{
		/*Grab a CFDP event*/
		int cfdp_result = cfdp_get_event(&type, &time, &reqNbr, &transactionId,
				sourceFileNameBuf, destFileNameBuf,
				&fileSize, &messagesToUser, &offset, &length,
				&recordBoundsRespected, &continuationState,
				&segMetadataLength, segMetadata,
				&condition, &progress, &fileStatus,
				&deliveryCode, &originatingTransactionId,
				statusReportBuf, &filestoreResponses, &closureRequested);
		if (cfdp_result < 0)
		{
			dbgprintf(0, "Error: Failed getting CFDP event.", NULL);
			exit(1);
		}

		if (type == CfdpNoEvent)
		{
			continue;	/*	Interrupted.		*/
		}

		/*Decompress transaction ID*/
		cfdp_decompress_number(&TID11,&transactionId.sourceEntityNbr);
		cfdp_decompress_number(&TID12,&transactionId.transactionNbr);
		putFqn(nbrBuf, TID11);

		/*Print Event type if debugging*/
		dbgprintf(4,"\nEvent: type %d, '%s', From Node: %s, Transaction ID: %s." UVAST_FIELDSPEC ".\n", type,
				(type > 0 && type < 12) ? eventTypes[type]
				: "(unknown)", nbrBuf, nbrBuf, TID12);

		/*Check for and Handle directory listing events.*/
		if (current_wait_status==dir_req || current_wait_status==dir_exists ||
				current_wait_status==nodir)
		{
			/*Only check for directory events if a directory lookup is pending*/

			/*Parse Messages to User to get directory information*/
			while (messagesToUser)
			{

				/*Get user message*/
				memset(usrmsgBuf, 0, 256);
				if (cfdp_get_usrmsg(&messagesToUser, usrmsgBuf,
						(int *) &length) < 0)
				{
					putErrmsg("Failed getting user msg.", NULL);
					continue;
				}

				/*Set Null character at end of string*/
				if (length > 0)
				{
					usrmsgBuf[length] = '\0';
					dbgprintf(2,"\tUser Message '%s'\n", usrmsgBuf);
				}

				/*Directory listings must be longer than 5 bytes*/
				if (length < 5)
				{
					continue;
				}

				/*Check that this is a std user message*/
				if (strncmp((char*)&usrmsgBuf, "cfdp", 4) != 0)
				{
					continue;
				}

				/*Check if this is a directory listing response.
				 *Type 17 is the legacy format; type 19 is v2
				 *with per-entry status/type bytes and a
				 *listing-level incomplete flag.*/
				if (usrmsgBuf[4]==17 || usrmsgBuf[4]==19)
				{
					memset(&dir_list_rsp, 0, sizeof(CfdpDirListingResponse));
					dir_list_rsp.isV2 = (usrmsgBuf[4] == 19);
					parseDirectoryListingResponse((usrmsgBuf + 5), length -5 , &dir_list_rsp);

					/*Check if this is the directory I'm waiting for*/
					if(strcmp(dir_list_rsp.directoryName, dirlst.directoryName)==0 &&
						strcmp(dir_list_rsp.directoryDestFileName, dirlst.destFileName)==0)
					{
						cfdp_decompress_number(&TID21,&transactionId.sourceEntityNbr);
						cfdp_decompress_number(&TID22,&transactionId.transactionNbr);
					}
					else
					{
						break;
					}

					/*Publish v2 / incomplete state to the
					 *waiter via the globals.*/
					dirlist_was_v2 = dir_list_rsp.isV2;
					dirlist_incomplete =
						dir_list_rsp.directoryListingIncomplete;

					/*Check response code*/
					if (dir_list_rsp.directoryListingResponseCode<128)
					{
						/*Success!*/
						dbgprintf(1, "Directory Exists: %s%s\n",
							dir_list_rsp.directoryName,
							dirlist_incomplete ? " (incomplete)" : "");
						dbgprintf(3, "Transaction ID: " UVAST_FIELDSPEC "\n", TID22);
						current_wait_status=dir_exists;
						break;
					}
					else
					{
						/*Failure*/
						dbgprintf(1, "No Directory: %s\n", dir_list_rsp.directoryName);
						dbgprintf(3, "Transaction ID: " UVAST_FIELDSPEC "\n", TID22);
						current_wait_status=nodir;
						break;
					}
				}
			}

			/*Check for completion of Transfer*/
			if (TID21==TID11 && TID22==TID12 && type==6)
			{
					/*Wake up main thread to continue copy operations*/
					TID21=TID22=0;
					sm_SemGive(events_sem);
			}
		}

		/*Check for and handle transaction EOF sent events*/
		if (current_wait_status==snd_wait || current_wait_status==sent)
		{
			/*Only check  for EOF events if we are waiting for one*/

			if(type==CfdpEofSentInd)
			{
				/*This is an EOF Sent event*/

				if (event_wait_id==NULL)
				{
					dbgprintf(0, "Error: event_wait_id NULL!!\n");
					continue;
				}

				cfdp_decompress_number(&TID21,&event_wait_id->sourceEntityNbr);
				cfdp_decompress_number(&TID22,&event_wait_id->transactionNbr);

				/*Compare transaction IDs*/
				if (TID21==TID11 && TID22==TID12)
				{
						/*Wake up main thread to continue copy operations*/
						TID21=TID22=0;
						dbgprintf(3, "EOF Sent\n");
						dbgprintf(3, "Transaction ID: " UVAST_FIELDSPEC "\n", TID22);
						current_wait_status=sent;
						sm_SemGive(events_sem);
				}
			}

		}

		/*	Check for proxy request processing completion.
		 *	When we see a CfdpMetadataRecvInd that matches our
		 *	proxy request transaction, it means handleProxyPutRequest
		 *	was called inside cfdp_get_event, signaling completion.	*/
		if (current_wait_status == proxy_wait)
		{
			if (type == CfdpMetadataRecvInd)
			{
				if (event_wait_id == NULL)
				{
					continue;
				}

				cfdp_decompress_number(&TID21,
						&event_wait_id->sourceEntityNbr);
				cfdp_decompress_number(&TID22,
						&event_wait_id->transactionNbr);

				/*	Check if this metadata event is for our
				 *	proxy request (same TID as the FDU we sent).	*/
				if (TID21 == TID11 && TID22 == TID12)
				{
					dbgprintf(3, "Proxy request processed\n");
					current_wait_status = proxy_done;
				}
			}
		}
	}
	return NULL;
}

/*Parse a CFDP directory listing response and pull out the status code,
 * the directory name, and the output file name*/
static void parseDirectoryListingResponse(unsigned char *text, int bytesRemaining,
		CfdpDirListingResponse *opsData)
{
	int	length;

	if (bytesRemaining < 1)
	{
		return;
	}

	opsData->directoryListingResponseCode = *text;
	text++;
	bytesRemaining--;

	/*	v2 only: one-byte incomplete flag follows the
	 *	response code.  isV2 must already be set by the
	 *	caller based on the user-message type seen on the
	 *	wire (17 vs 19).				*/

	if (opsData->isV2)
	{
		if (bytesRemaining < 1)
		{
			return;
		}
		opsData->directoryListingIncomplete = (unsigned char) *text;
		text++;
		bytesRemaining--;
	}

	/*	Get directory name.					*/
	if (bytesRemaining < 1)
	{
		return;
	}
	length = (unsigned char) *text;
	text++;
	bytesRemaining--;
	if (length > bytesRemaining)
	{
		return;
	}
	memcpy(opsData->directoryName, text, length);
	(opsData->directoryName)[length] = '\0';
	text += length;
	bytesRemaining -= length;

	/*	Get destination file name.				*/
	if (bytesRemaining < 1)
	{
		return;
	}
	length = (unsigned char) *text;
	text++;
	bytesRemaining--;
	if (length > bytesRemaining)
	{
		return;
	}
	memcpy(opsData->directoryDestFileName, text, length);
	(opsData->directoryDestFileName)[length] = '\0';
}

/*Debug Printf*/
void dbgprintf(int level, const char *fmt, ...)
{
	va_list args;
	if (debug>=level) {
		va_start(args, fmt);
		vfprintf(stderr, fmt, args);
		va_end(args);
	}
}

/*Print Command usage to stderr and exit*/
void usage(void)
{
	(void) fprintf(stderr, "usage: bpcp [-dqrF | -v] [-L Bundle Lifetime] [-C custody on/off]\n"
			"	[-S Class of Service] [host1:]file1 ... [host2:]file2\n"
			"	-F  follow symlinks during recursive copy\n");
	exit(1);
}

/*Print Version Information*/
void version(void)
{
	dbgprintf(0, BPCP_VERSION_STRING);
	exit(1);
}

/*Print debug output containing source/dest hosts and files*/
void print_parsed(struct transfer* t)
{
	if (debug >=2)
	{

		/*Print Source Information*/
		if (strlen(t->shost)==0)
		{
			fprintf(stderr, "Source Host: Local\n");
		}
		else
		{
			fprintf(stderr, "Source Host: %s\n", t->shost);
		}
		fprintf(stderr, "Source File: %s\n", t->sfile);

		/*Print Destination Information*/
		if (strlen(t->dhost)==0)
		{
			fprintf(stderr, "Destination Host: Local\n");
		}
		else
		{
			fprintf(stderr, "Destination Host: %s\n", t->dhost);
		}
		fprintf(stderr, "Destination File: %s\n", t->dfile);
	}
}

/*Exit program nicely ending receiver thread and deleting semaphore.
 * Return code is val, but is forced non-zero if any directory walk
 * surfaced an issue (incomplete listing, unreliable name, fetch
 * failure).  Skipped symlinks and non-regular entries are reported
 * in the summary but do not by themselves change the exit code.*/
void exit_nicely(int val)
{
	int i=0;
	int has_issues = (val != 0)
			|| incomplete_listings_seen
			|| truncated_entries_skipped
			|| file_fetch_failures;
	int has_skips  = symlinks_skipped || non_regular_skipped;

	if (has_issues || has_skips)
	{
		fprintf(stderr, "bpcp: tree copy completed with issues:\n");
		if (incomplete_listings_seen)
			fprintf(stderr, "  %d listings were incomplete "
				"(responder dropped entries; see ion.log "
				"on remote)\n", incomplete_listings_seen);
		if (truncated_entries_skipped)
			fprintf(stderr, "  %d entries skipped: unreliable "
				"name (truncated by responder)\n",
				truncated_entries_skipped);
		if (symlinks_skipped)
			fprintf(stderr, "  %d symlinks skipped "
				"(use -F to follow)\n", symlinks_skipped);
		if (non_regular_skipped)
			fprintf(stderr, "  %d non-regular entries skipped\n",
				non_regular_skipped);
		if (file_fetch_failures)
			fprintf(stderr, "  %d file fetch failures\n",
				file_fetch_failures);
	}

	if (has_issues && val == 0)
	{
		val = 1;
	}

	/*Delete any and all temporary files*/
	for (i=0; i < NUM_TMP_FILES; i++)
	{
		if (tmp_files[i][0] != 0)
		{
			unlink(tmp_files[i]);
		}
	}

	/*End receiver thread BEFORE deleting semaphore*/
	recv_running=0;
	cfdp_interrupt();

	/*End the semaphore BEFORE pthread_join so the receiver thread sees the
	 *ended flag when it wakes from cfdp_get_event and can exit gracefully.
	 *Also give the semaphore to wake up the thread if it's blocked on it.*/
	sm_SemEnd(events_sem);
	sm_SemGive(events_sem);

	/*Wait for receiver thread to exit*/
	pthread_join(rcv_thread, NULL);

	/*Now it's safe to delete the semaphore*/
	sm_SemDelete(events_sem);

	exit(val);
}


/*Outputs progress information for the start of a copy
 * if the progress meter is enabled.*/
void prog_start_cpy(struct transfer *t)
{
	char* topname;

	if (showprogress)
	{

		/*Cut filename out of full path*/
		topname=strrchr(t->sfile, '/');
		if(topname==NULL)
		{
			topname=t->sfile;
		}
		else
		{
			topname++;
		}

		printf("Starting Copy: %-*s", setscreensize()- 50, topname);
		if (!debug)
		{
				fflush(stdout);
		}
	}
}

/*Outputs progress information for the end of a copy
 * if the progress meter is enabled.*/
void prog_end_cpy(struct transfer *t)
{
	/* Parameter intentionally unused. */
	(void)t;

	if (showprogress)
	{
		printf(" Complete\n");
	}
}

/*Outputs progress information for the start of directory request
 * if the progress meter is enabled.*/
void prog_start_dir(struct transfer *t)
{
	char* topname;

	if (showprogress)
	{

		/*Cut filename out of full path*/
		topname=strrchr(t->sfile, '/');
		if(topname==NULL)
		{
			topname=t->sfile;
		}
		else
		{
			topname++;
		}

		printf("Querying: %-*s", setscreensize()- 45, topname);
		if (!debug)
		{
			fflush(stdout);
		}
	}
}

/*Outputs progress information for the end of a directory request
 * if the progress meter is enabled.*/
void prog_end_dir(struct transfer *t)
{
	/* Parameter intentionally unused. */
	(void)t;

	if (showprogress)
	{
		printf(" Complete\n");
	}
}

/*Determine the size of the terminal in use*/
int setscreensize(void)
{
#if defined (VXWORKS) || defined (RTEMS) || !defined(TIOCGWINSZ)
	return 80;
#else
	int win_size;
	struct winsize winsize;
	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &winsize) != -1 && winsize.ws_col != 0)
	{
		if (winsize.ws_col > 512)
		{
			win_size = 512;
		}
		else
		{
			win_size = winsize.ws_col;
		}
	}
	else
	{
		win_size = 80;
	}
	win_size += 1;					/* trailing \0 */
	return win_size;
#endif
}

#ifdef SIG_HANDLER
/*Perform some simple cleanup on SIGTERM*/
static void handle_sigterm(int signum)
{
	/* Tell the compiler that we are not using 'signum' */
	(void)signum;

	/*Reset signal handlers for portability*/
	isignal(SIGTERM, handle_sigterm);
	isignal(SIGINT, handle_sigterm);

	/*Tell receiver thread to exit*/
	recv_running=0;

	/*Give remote directory listing semaphore to allow main thread to exit*/
	sm_SemGive(events_sem);

	/*Interrupt the CFDP processing to allow receiver thread to exit*/
	cfdp_interrupt();
}
#endif
