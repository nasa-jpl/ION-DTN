/*
 *
 * 	file2sdr.h:	definitions used for the file2sdr SDR
 * 			activity test system.
 *
 * 									*/

#include "platform.h"
#include "sdr.h"
#include "ion.h"

typedef struct
{
	int	cycleNbr;
	int	lineCount;	/*	Number of lines read from file.	*/
	Object	lines;		/*	Lines read, not yet written.	*/
} Cycle;

#define TEST_WM_SIZE	(1000000)
#define TEST_HEAP_WORDS	(1000000)
#define	TEST_SDR_NAME	"testsdr"
#define TEST_SEM_KEY	0x1100
#define TEST_PATH_NAME	"/usr/sdr"
#define CYCLE_LIST_NAME	"cycles"
#define EOF_LINE_TEXT	"*** End of the file ***"
