/*
 * platform_smP.h: private definitions for platform shared memory API.
 */

#ifndef PLATFORM_SMP_H
#define PLATFORM_SMP_H

#include "platform_sm.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Private functions for internal ION use only. */

extern void sm_TaskKillWait(int taskId, char *text, char *note);

#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_SMP_H */
