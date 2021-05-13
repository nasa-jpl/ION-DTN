/** \file log.c
 * 
 *  \brief  This file provides the implementation of a library
 *          to print various log files. Only for Unix-like systems.
 *
 ** \copyright Copyright (c) 2020, Alma Mater Studiorum, University of Bologna, All rights reserved.
 **
 ** \par License
 **
 **    This file is part of Unibo-CGR.                                            <br>
 **                                                                               <br>
 **    Unibo-CGR is free software: you can redistribute it and/or modify
 **    it under the terms of the GNU General Public License as published by
 **    the Free Software Foundation, either version 3 of the License, or
 **    (at your option) any later version.                                        <br>
 **    Unibo-CGR is distributed in the hope that it will be useful,
 **    but WITHOUT ANY WARRANTY; without even the implied warranty of
 **    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **    GNU General Public License for more details.                               <br>
 **                                                                               <br>
 **    You should have received a copy of the GNU General Public License
 **    along with Unibo-CGR.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  \author Lorenzo Persampieri, lorenzo.persampieri@studio.unibo.it
 *
 *  \par Supervisor
 *       Carlo Caini, carlo.caini@unibo.it
 */

#include "log.h"

#include <dirent.h>
#include "../../contact_plan/contacts/contacts.h"
#include "../../contact_plan/nodes/nodes.h"
#include "../../contact_plan/ranges/ranges.h"
#include "../../library/list/list.h"

#if (LOG == 1)

typedef struct {
	/**
	 * \brief The main log file.
	 */
	FILE *file_log;
	/**
	 * \brief Path of the logs directory.
	 */
	char log_dir[256];
	/**
	 * \brief Boolean: '1' if we already checked the existence of the
	 * log directory, '0' otherwise.
	 */
	char log_dir_exists;
	/**
	 * \brief Length of the log_dir string. (strlen)
	 */
	int len_log_dir;
	/**
	 * \brief The time used by the log files.
	 */
	time_t currentTime;
	/**
	 * \brief The last time when the logs have been printed.
	 */
	time_t lastFlushTime;
	/**
	 * \brief The buffer used to print the logs in the main log file.
	 */
	char buffer[256]; //don't touch the size of the buffer
} LogSAP;

/******************************************************************************
 *
 * \par Function Name:
 * 		get_log_sap
 *
 * \brief Get a reference to LogSAP where are stored all data used to print some logs.
 *
 *
 * \par Date Written:
 * 	    02/07/20
 *
 * \return LogSAP*
 *
 * \retval  LogSAP*  The struct with log informations
 *
 * \param[in]   *newSap      If you just want a reference to the SAP set NULL here;
 *                           otherwise set a new LogSAP (the previous one will be overwritten).
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |  DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  02/07/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
static LogSAP * get_log_sap(LogSAP *newSap) {
	static LogSAP sap;

	if(newSap != NULL) {
		sap = *newSap;
	}

	return &sap;
}

/******************************************************************************
 *
 * \par Function Name:
 *      writeLog
 *
 * \brief Write a log line on the main log file
 *
 *
 * \par Date Written:
 *      24/01/20
 *
 * \return void
 *
 * \param[in]  *format   Use like a printf function
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  24/01/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
void writeLog(const char *format, ...)
{
	va_list args;
	LogSAP *sap = get_log_sap(NULL);

	if (sap->file_log != NULL)
	{
		va_start(args, format);

		fprintf(sap->file_log, "%s", sap->buffer); //[            time]:
		vfprintf(sap->file_log, format, args);
		fputc('\n', sap->file_log);

		debug_fflush(stdout);

		va_end(args);
	}
	return;
}

/******************************************************************************
 *
 * \par Function Name:
 *      writeLogFlush
 *
 * \brief Write a log line on the main log file and flush the output stream
 *
 *
 * \par Date Written:
 *      03/04/20
 *
 * \return void
 *
 * \param[in]  *format   Use like a printf function
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  03/04/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
void writeLogFlush(const char *format, ...)
{
	va_list args;
	LogSAP *sap = get_log_sap(NULL);

	if (sap->file_log != NULL)
	{
		va_start(args, format);

		fprintf(sap->file_log, "%s", sap->buffer); //[            time]:
		vfprintf(sap->file_log, format, args);
		fputc('\n', sap->file_log);
		fflush(sap->file_log);

		va_end(args);
	}
	return;
}


/******************************************************************************
 *
 * \par Function Name:
 *      log_fflush
 *
 * \brief Flush the stream
 *
 *
 * \par Date Written:
 *      11/04/20
 *
 * \return void
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  11/04/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
void log_fflush()
{
	LogSAP *sap = get_log_sap(NULL);
	if(sap->file_log != NULL)
	{
		fflush(sap->file_log);
		sap->lastFlushTime = sap->currentTime;
	}
}

/******************************************************************************
 *
 * \par Function Name:
 *      setLogTime
 *
 * \brief  Set the time that will be printed in the next log lines.
 *
 *
 * \par Date Written:
 *      24/01/20
 *
 * \return void
 *
 * \param[in]  time   	The time that we want to print in the next log lines
 *
 * \par Notes:
 *            1.   Set the first 19 characters of the buffer
 *                 used to print the log (translated: "[    ...xxxxx...]: "
 *                 where ...xxxxx... is the time)
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  24/01/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
void setLogTime(time_t time)
{
	LogSAP *sap = get_log_sap(NULL);
	if (time != sap->currentTime && time >= 0)
	{
		sap->currentTime = time;
		sprintf(sap->buffer, "[%15ld]: ", (long int) sap->currentTime);
		//set the first 19 characters of the buffer
		if(sap->currentTime - sap->lastFlushTime > 5) // After 5 seconds.
		{
			log_fflush();
		}
	}
}

/******************************************************************************
 *
 * \par Function Name:
 *      print_string
 *
 * \brief  Print a string to the indicated file
 *
 *
 * \par Date Written:
 *      24/01/20
 *
 * \return int
 *
 * \retval   0   Success case
 * \retval  -1   Arguments error
 * \retval  -2   Write error
 *
 * \param[in]  file       The file where we want to print the string
 * \param[in]  *toPrint   The string that we want to print
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  24/01/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
int print_string(FILE *file, char *toPrint)
{
	int result = -1;
	if (file != NULL && toPrint != NULL)
	{
		result = fprintf(file, "%s", toPrint);

		result = (result >= 0) ? 0 : -2;

		debug_fflush(stdout);
	}

	return result;
}

/******************************************************************************
 *
 * \par Function Name:
 *      createLogDir
 *
 * \brief  Create the main directory where the log files will be putted in
 *
 *
 * \par Date Written:
 *      24/01/20
 *
 * \return int
 *
 * \retval   0   We already called this function and the directory exists
 * \retval   1   Directory created correctly
 * \retval  -1   Error case: the directory cannot be created
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  24/01/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
int createLogDir()
{
	/*
	 * Currently the directory is created under the current directory
	 * with the name: cgr_log
	 *
	 * Relative path: ./cgr_log
	 */
	LogSAP *sap = get_log_sap(NULL);

	int result = 0;
//long unsigned int len;
//char *homedir;
	if (sap->log_dir_exists != '1')
	{
		/*
		 homedir = getenv("HOME");
		 if (homedir != NULL)
		 {
		 strcpy(log_dir, homedir);
		 len = strlen(log_dir);
		 if (len == 0 || (len > 0 && log_dir[len - 1] != '/'))
		 {
		 log_dir[len] = '/';
		 log_dir[len + 1] = '\0';
		 }
		 strcat(log_dir, "cgr_log/");
		 }
		 */
		strcpy(sap->log_dir, "./cgr_log/");
		if (mkdir(sap->log_dir, 0777) != 0 && errno != EEXIST)
		{
			perror("Error CGR log dir cannot be created");
			result = -1;
			sap->len_log_dir = 0;
		}
		else
		{
			result = 1;
			sap->log_dir_exists = '1';
			sap->len_log_dir = strlen("./cgr_log/");
		}
	}

	return result;
}

/******************************************************************************
 *
 * \par Function Name:
 *      openBundleFile
 *
 * \brief  Open the file where the cgr will print the characteristics of the
 *         bundle and all the decisions taken for the forwarding of that bundle
 *
 *
 * \par Date Written:
 *      24/01/20
 *
 * \return FILE*
 * 
 * \retval  FILE*   The file opened
 * \retval  NULL    Error case
 *
 * \param[in]  num  The * in the file's name (see note 2.)
 *
 *       \par Notes:
 *                    1. The file will be opened in write only mode.
 *                    2. Currently the name of the file follows the pattern: call_#*
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  24/01/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
FILE* openBundleFile(unsigned int num)
{
	FILE *file = NULL;
	LogSAP *sap = get_log_sap(NULL);

	if (sap->len_log_dir > 0)
	{
		sprintf(sap->log_dir + sap->len_log_dir, "call_#%u", num);
		file = fopen(sap->log_dir, "w");

		sap->log_dir[sap->len_log_dir] = '\0';
	}

	return file;
}

/******************************************************************************
 *
 * \par Function Name:
 *      closeBundleFile
 *
 * \brief  Close the "call file"
 *
 *
 * \par Date Written:
 *      24/01/20
 *
 * \return null
 *
 * \param[in,out]  **file_call  The FILE to close, at the end file_call will points to NULL
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  24/01/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
void closeBundleFile(FILE **file_call)
{
	if (file_call != NULL && *file_call != NULL)
	{
		fflush(*file_call);
		fclose(*file_call);
		*file_call = NULL;
	}
}

/******************************************************************************
 *
 * \par Function Name:
 *      cleanLogDir
 *
 * \brief Clean the previously create directory where the log will be printed.
 *
 *
 * \par Date Written:
 *      24/01/20
 *
 * \return int
 *
 * \retval   1   Success case
 * \retval   0   If we never called 'createLogDir' or the directory
 *               wasn't created due to an error.
 * \retval  -1   The directory cannot be opened
 *
 * \par Notes:
 * 			1. The only file that will not be deleted is "log.txt"
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  24/01/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
int cleanLogDir()
{
	int result = 0;
	DIR *dir;
	struct dirent *file;
	LogSAP *sap = get_log_sap(NULL);

	if (sap->log_dir_exists == '1')
	{
		result = 1;

		sap->len_log_dir = strlen(sap->log_dir);
		if ((dir = opendir(sap->log_dir)) != NULL)
		{
			while ((file = readdir(dir)) != NULL)
			{
				if ((strcmp(file->d_name, ".") != 0) && (strcmp(file->d_name, "..") != 0)
						&& (strcmp(file->d_name, "log.txt") != 0) && file->d_type == DT_REG)
				{
					strcpy(sap->log_dir + sap->len_log_dir, file->d_name);
					remove(sap->log_dir);
					sap->log_dir[sap->len_log_dir] = '\0';
				}
			}

			closedir(dir);
		}
		else
		{
			result = -1;
		}
	}

	return result;
}

/******************************************************************************
 *
 * \par Function Name:
 *      openLogFile
 *
 * \brief Open the main log file
 *
 *
 * \par Date Written:
 *      24/01/20
 *
 * \return int
 * 
 * \retval   1   Success case, main log file opened.
 * \retval   0   If we never called 'createLogDir' or the directory
 *               wasn't created due to an error or the main log file already exists
 * \retval  -1   The file cannot be opened for some reason
 *
 * \par Notes:
 * 			1. The file will be created in write only mode.
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  24/01/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
int openLogFile()
{
	int result = 0;
	long unsigned int len;
	LogSAP *sap = get_log_sap(NULL);

	if (sap->file_log != NULL)
	{
		result = 1;
	}
	else if (sap->log_dir_exists == '1')
	{
		sap->currentTime = -1;
		len = strlen(sap->log_dir);
		strcat(sap->log_dir, "log.txt");

		sap->file_log = fopen(sap->log_dir, "w");

		if (sap->file_log == NULL)
		{
			perror("Error file ./cgr_log/log.txt cannot be opened");
			result = -1;
		}
		else
		{
			result = 1;
		}

		sap->log_dir[len] = '\0';
	}

	return result;
}

/******************************************************************************
 *
 * \par Function Name:
 *      closeLogFile
 *
 * \brief  Close the main log file
 *
 *
 * \par Date Written:
 *      24/01/20
 *
 * \return  void
 *
 * \par Notes:
 * 			1. fd_log will be setted to -1.
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  24/01/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
void closeLogFile()
{
	LogSAP *sap = get_log_sap(NULL);
	if (sap->file_log != NULL)
	{
		fflush(sap->file_log);
		fclose(sap->file_log);
		sap->file_log = NULL;
	}
}

/******************************************************************************
 *
 * \par Function Name:
 *      printCurrentState
 *
 * \brief  Print all the informations to keep trace of the current state of the graphs
 *
 *
 * \par Date Written:
 *      30/01/20
 *
 * \return void
 *
 * \par Notes:
 * 			1.	The contacts graph is printed in append mode in the file "contacts.txt"
 * 			2.	The ranges graph is printed in append mode in the file "ranges.txt"
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  30/01/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
void printCurrentState()
{
	long unsigned int len = 0;
	FILE * file_contacts, *file_ranges;
	LogSAP *sap = get_log_sap(NULL);
	if (sap->log_dir_exists == '1')
	{
		len = strlen(sap->log_dir);
		sap->log_dir[len] = '\0';
		strcat(sap->log_dir, "contacts.txt");
		if ((file_contacts = fopen(sap->log_dir, "a")) == NULL)
		{
			perror("Error contacts graph's file cannot be opened");
			return;
		}
		sap->log_dir[len] = '\0';
		strcat(sap->log_dir, "ranges.txt");
		if ((file_ranges = fopen(sap->log_dir, "a")) == NULL)
		{
			perror("Error contacts graph's file cannot be opened");
			fclose(file_contacts);
			return;
		}
		sap->log_dir[len] = '\0';

		printContactsGraph(file_contacts, sap->currentTime);
		printRangesGraph(file_ranges, sap->currentTime);

		fflush(file_contacts);
		fclose(file_contacts);
		fflush(file_ranges);
		fclose(file_ranges);
		file_contacts = NULL;
		file_ranges = NULL;
	}
}

/******************************************************************************
 *
 * \par Function Name:
 *      print_ull_list
 *
 * \brief  Print a list of unsigned long long element
 *
 *
 * \par Date Written:
 *      29/03/20
 *
 * \return int
 *
 * \retval   0   Success case
 * \retval  -1   Arguments error
 * \retval  -2   Write error
 *
 * \param[in]  file        The file where we want to print the list
 * \param[in]  list        The list to print
 * \param[in]  *brief      A description printed before the list
 * \param[in]  *separator  The separator used between the elements
 *
 * \warning brief must be a well-formed string
 * \warning separator must be a well-formed string
 * \warning All the elements of the list (ListElt->data) have to be != NULL
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  29/03/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
int print_ull_list(FILE *file, List list, char *brief, char *separator)
{
	int len, result = -1, temp;
	ListElt *elt;
	if (file != NULL && list != NULL && brief != NULL && separator != NULL)
	{
		result = 0;
		len = fprintf(file, "%s", brief);
		if (len < 0)
		{
			result = -2;
		}
		for (elt = list->first; elt != NULL && result == 0; elt = elt->next)
		{
			temp = fprintf(file, "%llu%s", *((unsigned long long*) elt->data),
					(elt == list->last) ? "" : separator);
			if (temp >= 0)
			{
				len += temp;
			}
			else
			{
				result = -2;
			}
			if (len > 85)
			{
				temp = fputc('\n', file);

				if (temp < 0)
				{
					result = -2;
				}

				len = 0;
			}
		}

		temp = fputc('\n', file);
		if (temp < 0)
		{
			result = -2;
		}

	}

	return result;
}

#endif
