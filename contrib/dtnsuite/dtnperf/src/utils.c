/********************************************************
 **  Authors: Michele Rodolfi, michele.rodolfi@studio.unibo.it
 **           Anna d'Amico, anna.damico@studio.unibo.it
 **           Carlo Caini (DTNperf_3 project supervisor), carlo.caini@unibo.it
 **
 **
 **  Copyright (c) 2013, Alma Mater Studiorum, University of Bologna
 **  All rights reserved.
 ********************************************************/

#include "utils.h"
#include <dirent.h>

/* ------------------------------------------
 * mega2byte
 *
 * Converts MBytes into Bytes
 * ------------------------------------------ */
long mega2byte(double n)
{
    return (n * 1000 *1000);
} // end mega2byte

/* ------------------------------------------
 * kilo2byte
 *
 * Converts KBytes into Bytes
 * ------------------------------------------ */
long kilo2byte(double n)
{
    return (n * 1000);
} // end kilo2byte

/**
 * byte2mega
 * Converts Bytes in MBytes
 */
double byte2mega(long n)
{
	double result;
	result = (double) n / (1000 * 1000);
	return result;
}

/**
 * byte2kilo
 * Converts Bytes in KBytes
 */
double byte2kilo(long n)
{
	double result;
	result = (double) n / (1000);
	return result;
}


/* ------------------------------------------
 * findDataUnit
 *
 * Extracts the data unit from the given string.
 * If no unit is specified, returns 'Z'.
 * ------------------------------------------ */
char find_data_unit(const char *inarg)
{
    // units are B (Bytes), K (KBytes) and M (MBytes)
    const char unitArray[] = "BkKM";
    char * unit;

    if ((unit = strpbrk(inarg, unitArray)) == NULL)
    {
    	return 'Z';
    }

    if (unit[0] == 'K')
    	return 'k';

    return unit[0];
    //return unit[0];
} // end find_data_unit

/* ------------------------------------------
 * findRateUnit
 *
 * Extracts the rate unit from the given string.
 * If no unit is specified, returns kbit/sec.
 * ------------------------------------------ */
char find_rate_unit(const char *inarg)
{
    // units are b (bundles/sec), K (Kbit/sec), and M (Mbit/sec)
    const char unitArray[] = "kKMb";
    char * ptr;
    char unit;

    if ((ptr = strpbrk(inarg, unitArray)) == NULL)
    {
    	printf("\nWARNING: (-R option) invalid rate unit, assuming 'k' (kb/s)\n\n");
    	return 'k';
    }

    unit = ptr[0];

    if (unit == 'K')
    	return 'k';

    return unit;
} // end find_data_unit

char find_forced_eid(const char *inarg)
{
	// value of forced unit are CBHE or URI
	if(strcmp("DTN", inarg) == 0 || strcmp("dtn", inarg) == 0)
		return 'D';
	else if(strcmp("IPN", inarg) == 0 || strcmp("ipn", inarg) == 0)
		return 'I';
	else
		return '?';
} // end find_force_eid


/* ------------------------------------------
 * add_time
 * ------------------------------------------ */
struct timeval add_time(struct timeval * time_1, struct timeval * time_2)
{
	struct timeval result;
    result.tv_sec = time_1->tv_sec + time_2->tv_sec;
    result.tv_usec = time_1->tv_usec + time_2->tv_usec;

    if (result.tv_usec >= 1000000)
    {
        result.tv_sec++;
        result.tv_usec -= 1000000;
    }
    return result;

} // end add_time


/* ------------------------------------------
 * sub_time
 * Subtract time sub from time min and put the result in result
 * Result is set to NULL result is a negative value
 * ------------------------------------------ */
void sub_time(struct timeval min, struct timeval sub, struct timeval * result)
{
	if (result == NULL)
		return;
   if (min.tv_sec < sub.tv_sec)
	   result = NULL;
   else if (min.tv_sec == sub.tv_sec && min.tv_usec < sub.tv_usec)
	   result = NULL;
   else
   {
	   if (min.tv_usec < sub.tv_usec)
	   {
		   min.tv_usec += 1000 * 1000;
		   min.tv_sec -= 1;
	   }
	   result->tv_sec = min.tv_sec - sub.tv_sec;
	   result->tv_usec = min.tv_usec - sub.tv_usec;
   }

} // end add_time


/* --------------------------------------------------
 * csv_time_report
 * -------------------------------------------------- */
void csv_time_report(int b_sent, int payload, struct timeval start, struct timeval end, FILE* csv_log)
{
    const char* time_report_hdr = "BUNDLE_SENT,PAYLOAD (byte),TIME (s),DATA_SENT (Mbyte),GOODPUT (Mbit/s)";


    double g_put, data;

    data = (b_sent * payload)/1000000.0;

    fprintf(csv_log, "\n\n%s\n", time_report_hdr);

    g_put = (data * 8 * 1000) / ((double)(end.tv_sec - start.tv_sec) * 1000.0 +
                          (double)(end.tv_usec - start.tv_usec) / 1000.0);
    
    double time_s =  ((double)(end.tv_sec - start.tv_sec) * 1000.0 + (double)(end.tv_usec - start.tv_usec) / 1000.0) / 1000;
    
    fprintf(csv_log, "%d,%d,%.1f,%E,%.3f\n", b_sent, payload, time_s, data, g_put);

} // end csv_time_report




/* --------------------------------------------------
 * csv_data_report
 * -------------------------------------------------- */
void csv_data_report(int b_id, int payload, struct timeval start, struct timeval end, FILE* csv_log)
{
    const char* data_report_hdr = "BUNDLE_ID,PAYLOAD (byte),TIME (s),GOODPUT (Mbit/s)";
    // const char* time_hdr = "BUNDLES_SENT,PAYLOAD,TIME,GOODPUT";
    double g_put;

    fprintf(csv_log, "\n\n%s\n", data_report_hdr);

    g_put = (payload * 8) / ((double)(end.tv_sec - start.tv_sec) * 1000.0 +
                             (double)(end.tv_usec - start.tv_usec) / 1000.0) / 1000.0;
                             
    double time_s =  ((double)(end.tv_sec - start.tv_sec) * 1000.0 + (double)(end.tv_usec - start.tv_usec) / 1000.0) / 1000;

    fprintf(csv_log, "%d,%d,%.1f,%.3f\n", b_id, payload, time_s, g_put);

} // end csv_data_report



/* -------------------------------------------------------------------
 * pattern
 *
 * Initialize the buffer with a pattern of (index mod 10).
 * ------------------------------------------------------------------- */
void pattern(char *outBuf, int inBytes)
{
    assert (outBuf != NULL);
    while (inBytes-- > 0)
    {
        outBuf[inBytes] = (inBytes % 10) + '0';
    }
} // end pattern



/* -------------------------------------------------------------------
 * Set timestamp to the given seconds
 * ------------------------------------------------------------------- */
struct timeval set( double sec )
{
    struct timeval mTime;

    mTime.tv_sec = (long) sec;
    mTime.tv_usec = (long) ((sec - mTime.tv_sec) * 1000000);

    return mTime;
} // end set


/* -------------------------------------------------------------------
 * Add seconds to my timestamp.
 * ------------------------------------------------------------------- */
struct timeval add( double sec )
{
    struct timeval mTime;

    mTime.tv_sec = (long) sec;
    mTime.tv_usec = (long) ((sec - ((long) sec )) * 1000000);

    // watch for overflow
    if ( mTime.tv_usec >= 1000000 )
    {
        mTime.tv_usec -= 1000000;
        mTime.tv_sec++;
    }
    assert( mTime.tv_usec >= 0 && mTime.tv_usec < 1000000 );

    return mTime;
} // end add


/* --------------------------------------------------
 * show_report
 * -------------------------------------------------- */
void show_report (u_int buf_len, char* eid, struct timeval start, struct timeval end, long data, FILE* output)
{
    double g_put;

    double time_s = ((double)(end.tv_sec - start.tv_sec) * 1000.0 + (double)(end.tv_usec - start.tv_usec) / 1000.0) / 1000.0;

    double data_MB = data / 1000000.0;
    
    if (output == NULL)
        printf("got %d byte report from [%s]: time=%.1f s - %E Mbytes sent", buf_len, eid, time_s, data_MB);
    else
        fprintf(output, "\n total time=%.1f s - %E Mbytes sent", time_s, data_MB);
  
    // report goodput (bits transmitted / time)
    g_put = (data_MB * 8) / time_s;
    if (output == NULL)
        printf(" (goodput = %.3f Mbit/s)\n", g_put);
    else
        fprintf(output, " (goodput = %.3f Mbit/s)\n", g_put);

    // report start - end time
    if (output == NULL)
        printf(" started at %u sec - ended at %u sec\n", (u_int)start.tv_sec, (u_int)end.tv_sec);
    else
        fprintf(output, " started at %u sec - ended at %u sec\n", (u_int)start.tv_sec, (u_int)end.tv_sec);

} // end show_report


char* get_filename(char* s)
{
    int i = 0, k;
    char* temp;
    char c = 'a';
    k = strlen(s) - 1;
    temp = malloc(strlen(s));
    strcpy(temp, s);
    while ((c != '/') && (k >= 0))
    {
        c = temp[k];
        k--;
    }

    if (c == '/')
        k += 2;

    else
        return temp;

    while (k != (int)strlen(temp))
    {
        temp[i] = temp[k];
        i++;
        k++;
    }
    temp[i] = '\0';

    return temp;
} // end get_filename

/* --------------------------------------------------
 * file_exists
 * returns TRUE if file exists, FALSE otherwise
 * -------------------------------------------------- */
boolean_t file_exists(const char * filename)
{
    FILE * file;
    if ((file = fopen(filename, "r")) != NULL)
    {
        fclose(file);
        return TRUE;
    }
    return FALSE;
} // end file_exists

char * correct_dirname(char * dir)
{
	if (dir[0] == '~')
	{
		char * result, buffer[100];
		char * home = getenv("HOME");
		strcpy(buffer, home);
		strcat(buffer, dir + 1);
		result = strdup(buffer);
		return result;
	}
	else return dir;
}

int find_proc(char * cmd)
{
	DIR * proc;
	struct dirent * item;
	char cmdline_file[280];
	char buf[256], cmdline[256];
	char cmd_exe[256], cmdline_exe[256];
	char cmd_args[256], cmdline_args[256];
	char * cmdline_args_ptr;
	int item_len, i, fd, pid, length;
	int result = 0;
	boolean_t found_not_num, found_null;

	// prepare cmd
	memset(cmd_args, 0, sizeof(cmd_args));
	memset(cmd_exe, 0, sizeof(cmd_exe));
	strcpy(cmd_args, strchr(cmd, ' '));
    sprintf(cmd_exe, "%s%s", get_exe_name(strtok(cmd, " ")), cmd_args);
    
	proc = opendir("/proc/");
	while ((item = readdir(proc)) != NULL)
	{
		found_not_num = FALSE;
		item_len = strlen(item->d_name);
		for(i = 0; i < item_len; i++)
		{
			if (item->d_name[i] < '0' || item->d_name[i] > '9')
			{
				found_not_num = TRUE;
				break;
			}
		}
		if (found_not_num)
			continue;

		memset(cmdline_file, 0, sizeof(cmdline_file));
		memset(cmdline, 0, sizeof(cmdline));
		memset(buf, 0, sizeof(buf));

		sprintf(cmdline_file, "/proc/%s/cmdline", item->d_name);
		fd = open(cmdline_file, O_RDONLY);
		length = read(fd, buf, sizeof(buf));
		close(fd);

		found_null = FALSE;
		if(buf[0] == '\0')
			continue;

		for(i = 0; i < length; i++)
		{
			if(buf[i] == '\0')
			{
				if(found_null)
				{
					//reached end of cmdline
					cmdline[i -1] = '\0';
					break;
				}
				else
				{
					found_null = TRUE;
					cmdline[i] = ' ';
				}
			}
			else
			{
				found_null = FALSE;
				cmdline[i] = buf[i];
			}
		}

		memset(cmdline_args, 0, sizeof(cmdline_args));
		memset(cmdline_exe, 0, sizeof(cmdline_exe));
		cmdline_args_ptr = strchr(cmdline, ' ');
		strcpy(cmdline_args, cmdline_args_ptr != NULL ? cmdline_args_ptr : "");
		strcpy(cmdline_exe, get_exe_name(strtok(cmdline, " ")));
		strncat(cmdline_exe, cmdline_args, sizeof(cmdline_exe) - strlen(cmdline_exe) - 2);

		if (strncmp(cmdline_exe, cmd_exe, strlen(cmd_exe)) == 0)
		{
			pid = atoi(item->d_name);
			if (pid == getpid())
				continue;
			else
			{
				result = pid;
				break;
			}
		}

	}
	closedir(proc);
	return result;
}

char * get_exe_name(char * full_name)
{
	char * buf1 = strdup(full_name);
	char * buf2;
	char * result;
	char * token = strtok(full_name, "/");
	do
	{
		buf2 = strdup(token);
	} while ((token = strtok(NULL, "/")) != NULL);
	result = strdup(buf2);
	free (buf1);
	free (buf2);
	return result;
}

void pthread_sleep(double sec)
{
	pthread_mutex_t fakeMutex = PTHREAD_MUTEX_INITIALIZER;
	pthread_cond_t fakeCond = PTHREAD_COND_INITIALIZER;
	struct timespec abs_timespec;
	struct timeval current, time_to_wait;
	struct timeval abs_time;

	gettimeofday(&current,NULL);
	time_to_wait = set(sec);
	abs_time = add_time(&current, &time_to_wait);

	abs_timespec.tv_sec = abs_time.tv_sec;
	abs_timespec.tv_nsec = abs_time.tv_usec*1000;

	pthread_mutex_lock(&fakeMutex);
	pthread_cond_timedwait(&fakeCond, &fakeMutex, &abs_timespec);
	pthread_mutex_unlock(&fakeMutex);
}

uint32_t crc_table[256] = {

    0x00000000U,0x04C11DB7U,0x09823B6EU,0x0D4326D9U,
    0x130476DCU,0x17C56B6BU,0x1A864DB2U,0x1E475005U,
    0x2608EDB8U,0x22C9F00FU,0x2F8AD6D6U,0x2B4BCB61U,
    0x350C9B64U,0x31CD86D3U,0x3C8EA00AU,0x384FBDBDU,
    0x4C11DB70U,0x48D0C6C7U,0x4593E01EU,0x4152FDA9U,
    0x5F15ADACU,0x5BD4B01BU,0x569796C2U,0x52568B75U,
    0x6A1936C8U,0x6ED82B7FU,0x639B0DA6U,0x675A1011U,
    0x791D4014U,0x7DDC5DA3U,0x709F7B7AU,0x745E66CDU,
    0x9823B6E0U,0x9CE2AB57U,0x91A18D8EU,0x95609039U,
    0x8B27C03CU,0x8FE6DD8BU,0x82A5FB52U,0x8664E6E5U,
    0xBE2B5B58U,0xBAEA46EFU,0xB7A96036U,0xB3687D81U,
    0xAD2F2D84U,0xA9EE3033U,0xA4AD16EAU,0xA06C0B5DU,
    0xD4326D90U,0xD0F37027U,0xDDB056FEU,0xD9714B49U,
    0xC7361B4CU,0xC3F706FBU,0xCEB42022U,0xCA753D95U,
    0xF23A8028U,0xF6FB9D9FU,0xFBB8BB46U,0xFF79A6F1U,
    0xE13EF6F4U,0xE5FFEB43U,0xE8BCCD9AU,0xEC7DD02DU,
    0x34867077U,0x30476DC0U,0x3D044B19U,0x39C556AEU,
    0x278206ABU,0x23431B1CU,0x2E003DC5U,0x2AC12072U,
    0x128E9DCFU,0x164F8078U,0x1B0CA6A1U,0x1FCDBB16U,
    0x018AEB13U,0x054BF6A4U,0x0808D07DU,0x0CC9CDCAU,
    0x7897AB07U,0x7C56B6B0U,0x71159069U,0x75D48DDEU,
    0x6B93DDDBU,0x6F52C06CU,0x6211E6B5U,0x66D0FB02U,
    0x5E9F46BFU,0x5A5E5B08U,0x571D7DD1U,0x53DC6066U,
    0x4D9B3063U,0x495A2DD4U,0x44190B0DU,0x40D816BAU,
    0xACA5C697U,0xA864DB20U,0xA527FDF9U,0xA1E6E04EU,
    0xBFA1B04BU,0xBB60ADFCU,0xB6238B25U,0xB2E29692U,
    0x8AAD2B2FU,0x8E6C3698U,0x832F1041U,0x87EE0DF6U,
    0x99A95DF3U,0x9D684044U,0x902B669DU,0x94EA7B2AU,
    0xE0B41DE7U,0xE4750050U,0xE9362689U,0xEDF73B3EU,
    0xF3B06B3BU,0xF771768CU,0xFA325055U,0xFEF34DE2U,
    0xC6BCF05FU,0xC27DEDE8U,0xCF3ECB31U,0xCBFFD686U,
    0xD5B88683U,0xD1799B34U,0xDC3ABDEDU,0xD8FBA05AU,
    0x690CE0EEU,0x6DCDFD59U,0x608EDB80U,0x644FC637U,
    0x7A089632U,0x7EC98B85U,0x738AAD5CU,0x774BB0EBU,
    0x4F040D56U,0x4BC510E1U,0x46863638U,0x42472B8FU,
    0x5C007B8AU,0x58C1663DU,0x558240E4U,0x51435D53U,
    0x251D3B9EU,0x21DC2629U,0x2C9F00F0U,0x285E1D47U,
    0x36194D42U,0x32D850F5U,0x3F9B762CU,0x3B5A6B9BU,
    0x0315D626U,0x07D4CB91U,0x0A97ED48U,0x0E56F0FFU,
    0x1011A0FAU,0x14D0BD4DU,0x19939B94U,0x1D528623U,
    0xF12F560EU,0xF5EE4BB9U,0xF8AD6D60U,0xFC6C70D7U,
    0xE22B20D2U,0xE6EA3D65U,0xEBA91BBCU,0xEF68060BU,
    0xD727BBB6U,0xD3E6A601U,0xDEA580D8U,0xDA649D6FU,
    0xC423CD6AU,0xC0E2D0DDU,0xCDA1F604U,0xC960EBB3U,
    0xBD3E8D7EU,0xB9FF90C9U,0xB4BCB610U,0xB07DABA7U,
    0xAE3AFBA2U,0xAAFBE615U,0xA7B8C0CCU,0xA379DD7BU,
    0x9B3660C6U,0x9FF77D71U,0x92B45BA8U,0x9675461FU,
    0x8832161AU,0x8CF30BADU,0x81B02D74U,0x857130C3U,
    0x5D8A9099U,0x594B8D2EU,0x5408ABF7U,0x50C9B640U,
    0x4E8EE645U,0x4A4FFBF2U,0x470CDD2BU,0x43CDC09CU,
    0x7B827D21U,0x7F436096U,0x7200464FU,0x76C15BF8U,
    0x68860BFDU,0x6C47164AU,0x61043093U,0x65C52D24U,
    0x119B4BE9U,0x155A565EU,0x18197087U,0x1CD86D30U,
    0x029F3D35U,0x065E2082U,0x0B1D065BU,0x0FDC1BECU,
    0x3793A651U,0x3352BBE6U,0x3E119D3FU,0x3AD08088U,
    0x2497D08DU,0x2056CD3AU,0x2D15EBE3U,0x29D4F654U,
    0xC5A92679U,0xC1683BCEU,0xCC2B1D17U,0xC8EA00A0U,
    0xD6AD50A5U,0xD26C4D12U,0xDF2F6BCBU,0xDBEE767CU,
    0xE3A1CBC1U,0xE760D676U,0xEA23F0AFU,0xEEE2ED18U,
    0xF0A5BD1DU,0xF464A0AAU,0xF9278673U,0xFDE69BC4U,
    0x89B8FD09U,0x8D79E0BEU,0x803AC667U,0x84FBDBD0U,
    0x9ABC8BD5U,0x9E7D9662U,0x933EB0BBU,0x97FFAD0CU,
    0xAFB010B1U,0xAB710D06U,0xA6322BDFU,0xA2F33668U,
    0xBCB4666DU,0xB8757BDAU,0xB5365D03U,0xB1F740B4U,

};

/* ------------------------------------------
 * calc_crc32_d8
 *
 * Create 32b of CRC
 * ------------------------------------------ */

uint32_t calc_crc32_d8(uint32_t crc, uint8_t *data, int len)
{

	while (len > 0)
	{
		crc = crc_table[*data ^ ((crc >> 24) & 0xff)] ^ (crc << 8);
		data++;
		len--;
	}

	return crc;

}
