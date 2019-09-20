/*
 eclsaBlacklist.h

 Author: Nicola Alessi (nicola.alessi@studio.unibo.it)
 Project Supervisor: Carlo Caini (carlo.caini@unibo.it)

 Copyright (c) 2016, Alma Mater Studiorum, University of Bologna
 All rights reserved.

todo

 * */

#ifndef _ECLSABLACKLIST_H_
#define _ECLSABLACKLIST_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif


void blacklistInit();
void blacklistDestroy();
void addToBlacklist( unsigned short engineID, unsigned short matrixID);
bool isBlacklisted( unsigned short engineID,unsigned short matrixID);


#ifdef __cplusplus
}
#endif

#endif
