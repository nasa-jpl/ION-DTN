#include "time.h"

#if (TIME_ANALYSIS_ENABLED)
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>

#include "../cgr/cgr_phases.h"

typedef struct
{
	int beginOk;
	struct timespec beginTime;
	int endOk;
	struct timespec endTime;
} TimeRecorded;

typedef struct
{
	int configured;
	int fd;
} TimeFile;

static TimeFile * get_time_file()
{
	static TimeFile timeFile;

	return &timeFile;
}

#if (COMPUTE_PHASES_TIME)

/******************** PHASES TIME SECTION ********************/

typedef struct
{
	TimeRecorded phase_time;
	unsigned int call_counter;
	unsigned long long timer;
} PhaseTimeLogger;

typedef struct
{
	PhaseTimeLogger phase_time_logger[3];
} PhasesTime;

static PhasesTime *get_phases_time()
{
	static PhasesTime phasesTime;

	return &phasesTime;
}

void record_phases_start_time(UniboCgrPhase phase)
{
	PhasesTime *phasesTime = get_phases_time();
	int found = 0;
	int phaseSelected = 0;

	if(phase == phaseOne)
	{
		phaseSelected = 0; // phase one
		found = 1;
	}
	else if(phase == phaseTwo)
	{
		phaseSelected = 1; // phase two
		found = 1;
	}
	else if(phase == phaseThree)
	{
		phaseSelected = 2; // phase three
		found = 1;
	}
	else
	{
		// nothing to do
		found = 0;
	}

	if (found)
	{
		phasesTime->phase_time_logger[phaseSelected].phase_time.beginOk =
				clock_gettime(CLOCK_REALTIME, &(phasesTime->phase_time_logger[phaseSelected].phase_time.beginTime));
	}

	return;
}

void record_phases_stop_time(UniboCgrPhase phase)
{
	PhasesTime *phasesTime;
	int endOk;
	int found;
	UniboCgrPhase phaseSelected;
	struct timespec endTime;

	endOk = clock_gettime(CLOCK_REALTIME, &endTime);

	if(endOk != 0)
	{
		// some error
		return;
	}

	found = 0;
	phaseSelected = 0;

	phasesTime = get_phases_time();

	if(phase == phaseOne)
	{
		phaseSelected = 0; // phase one
		found = 1;
	}
	else if(phase == phaseTwo)
	{
		phaseSelected = 1; // phase two
		found = 1;
	}
	else if(phase == phaseThree)
	{
		phaseSelected = 2; // phase three
		found = 1;
	}
	else
	{
		// nothing to do
		found = 0;
	}

	if(found)
	{
		if(phasesTime->phase_time_logger[phaseSelected].phase_time.beginOk == 0)
		{
			phasesTime->phase_time_logger[phaseSelected].timer +=
					(endTime.tv_sec
							- phasesTime->phase_time_logger[phaseSelected].phase_time.beginTime.tv_sec)
							* 1000000000 + endTime.tv_nsec
							- phasesTime->phase_time_logger[phaseSelected].phase_time.beginTime.tv_nsec;

			phasesTime->phase_time_logger[phaseSelected].call_counter += 1;
		}

	}

	return;

}

/*************************************************************/

#endif

#if (COMPUTE_TOTAL_CORE_TIME || COMPUTE_TOTAL_INTERFACE_TIME)

typedef struct
{
	TimeRecorded total_time;
	unsigned long long timer;
} TotalTime;

#if (COMPUTE_TOTAL_CORE_TIME)

/******************** TOTAL CORE TIME SECTION ********************/

static TotalTime *get_total_core_time()
{
	static TotalTime totalTime;

	return &totalTime;
}

void record_total_core_start_time()
{
	TotalTime *totalTime = get_total_core_time();

	totalTime->total_time.beginOk = clock_gettime(CLOCK_REALTIME, &(totalTime->total_time.beginTime));

	return;
}

void record_total_core_stop_time()
{
	TotalTime *totalTime;
	int endOk;
	struct timespec endTime;

	endOk = clock_gettime(CLOCK_REALTIME, &endTime);

	if(endOk != 0)
	{
		// some error
		return;
	}

	totalTime = get_total_core_time();

	totalTime->timer = (endTime.tv_sec - totalTime->total_time.beginTime.tv_sec) * 1000000000 + endTime.tv_nsec - totalTime->total_time.beginTime.tv_nsec;

	return;

}

/************************************************************/

#endif

#if (COMPUTE_TOTAL_INTERFACE_TIME)

/******************** TOTAL INTERFACE TIME SECTION ********************/

static TotalTime *get_total_interface_time()
{
	static TotalTime totalTime;

	return &totalTime;
}

void record_total_interface_start_time()
{
	TotalTime *totalTime = get_total_interface_time();

	totalTime->total_time.beginOk = clock_gettime(CLOCK_REALTIME, &(totalTime->total_time.beginTime));

	return;
}

void record_total_interface_stop_time()
{
	TotalTime *totalTime;
	int endOk;
	struct timespec endTime;

	endOk = clock_gettime(CLOCK_REALTIME, &endTime);

	if(endOk != 0)
	{
		// some error
		return;
	}

	totalTime = get_total_interface_time();

	totalTime->timer = (endTime.tv_sec - totalTime->total_time.beginTime.tv_sec) * 1000000000 + endTime.tv_nsec - totalTime->total_time.beginTime.tv_nsec;

	return;

}

/************************************************************/

#endif

#endif

void print_time_results(time_t currentTime, unsigned int callNumber, CgrBundleID *id)
{
	TimeFile *timeFile = get_time_file();
#if (COMPUTE_PHASES_TIME)
	PhasesTime *phasesTime = get_phases_time();
	int i;
#endif
#if (COMPUTE_TOTAL_CORE_TIME)
	TotalTime *totalCoreTime = get_total_core_time();
#endif
#if (COMPUTE_TOTAL_INTERFACE_TIME)
	TotalTime *totalInterfaceTime = get_total_interface_time();
#endif
	char row[256];
	char phasesString[220];
	unsigned long long totalCore = 0;
	unsigned long long totalInterface = 0;
	int written;
	CgrBundleID tempId;

	if(id == NULL)
	{
		memset(&tempId, 0, sizeof(CgrBundleID));
		id = &tempId;
	}

#if (COMPUTE_TOTAL_CORE_TIME)
	totalCore = totalCoreTime->timer;
	totalCoreTime->timer = 0;
#endif
#if (COMPUTE_TOTAL_INTERFACE_TIME)
	totalInterface = totalInterfaceTime->timer;
	totalInterfaceTime->timer = 0;
#endif

#if (COMPUTE_PHASES_TIME)
	sprintf(phasesString, "%llu,%u,%llu,%u,%llu,%u",
			phasesTime->phase_time_logger[0].timer,
			phasesTime->phase_time_logger[0].call_counter,
			phasesTime->phase_time_logger[1].timer,
			phasesTime->phase_time_logger[1].call_counter,
			phasesTime->phase_time_logger[2].timer,
			phasesTime->phase_time_logger[2].call_counter);

	for(i = 0; i < 3; i++)
	{
		phasesTime->phase_time_logger[i].timer = 0;
		phasesTime->phase_time_logger[i].call_counter = 0;
	}
#else
	strcpy(phasesString, "0,0,0,0,0,0");
#endif

	written = sprintf(row, "%llu,%ld,%u,%llu,%u,%u,%u,%u,%llu,%llu,%s\n", get_local_node(), (long int) currentTime, callNumber,
			id->source_node, id->creation_timestamp, id->sequence_number, id->fragment_length,
			id->fragment_offset, totalInterface, totalCore, phasesString);

	if(written > 0 && timeFile->configured && timeFile->fd >= 0)
	{
		if (write(timeFile->fd, row, written * sizeof(char)) < 0)
		{
			verbose_debug_printf("Error during value reporting\n");
		}
	}
}

void initialize_time_analysis()
{
	TimeFile *timeFile = get_time_file();
	unsigned long long node_eid = get_local_node();
	char file_name[50];
	char *header = "local_node,current_time,call_num,src,ts,sqn_num,fragL,fragO,total_interface,total_core,ph_1_time,ph_1_calls,ph_2_time,ph_2_calls,ph_3_time,ph_3_calls\n";

	if (timeFile->configured == 0 || timeFile->fd < 0)
	{
		sprintf(file_name, "total_%llu.csv", node_eid);
		timeFile->fd = open(file_name, O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);

		if (timeFile->fd < 0)
		{
			debug_printf("Error during phase time file creation");
		}
		else if (write(timeFile->fd, header, strlen(header) * sizeof(char)) < 0)
		{
			debug_printf("Error during phase header writing\n");
			close(timeFile->fd);
		}
		else
		{
			timeFile->configured = 1;
		}
	}
}

void destroy_time_analysis()
{
	TimeFile *timeFile = get_time_file();

	if(timeFile->configured && timeFile->fd >= 0)
	{
		close(timeFile->fd);
	}
}

#endif
