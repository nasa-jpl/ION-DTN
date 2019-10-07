/********************************************************
    Authors:
    Lorenzo Mustich, lorenzo.mustich@studio.unibo.it
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
 * utility.h
 */
#include <types.h>
#include <pthread.h>

#ifndef DTNfog_SRC_UTILITY_H_
#define DTNfog_SRC_UTILITY_H_

void close_socket(int sd, int side);
void set_is_running_to_false(pthread_mutex_t mutex, int * is_running);
void signal_to_main(pthread_mutex_t mutex, pthread_cond_t cond);
int numberOfChar(char toFind, char * address);
void criticalStopExecution();
#endif /* DTNfog_SRC_UTILITY_H_ */
