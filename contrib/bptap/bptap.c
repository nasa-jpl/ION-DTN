/*
Copyright (c) 2015, California Institute of Technology.
All rights reserved.  Based on Government Sponsored Research under contracts
NAS7-1407 and/or NAS7-03001.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
    1. Redistributions of source code must retain the above copyright notice,
       this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation
       and/or other materials provided with the distribution.
    3. Neither the name of the California Institute of Technology (Caltech),
       its operating division the Jet Propulsion Laboratory (JPL), the National
       Aeronautics and Space Administration (NASA), nor the names of its
       contributors may be used to endorse or promote products derived from
       this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE CALIFORNIA INSTITUTE OF TECHNOLOGY BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<pthread.h>
#include<bp.h>

#include<linux/if.h>
#include<linux/if_tun.h>

char **EidList;
int EidListLen = 0;
BpSAP sap;

int tap_fd;
int flags = IFF_TAP;
char *if_name;

/**************************************************************************
 * tun_alloc: allocates or reconnects to a tun/tap device. The caller     *
 *            must reserve enough space in *dev.                          *
 **************************************************************************/
int tun_alloc(char *dev, int flags) {

  struct ifreq ifr;
  int fd, err;
  char *clonedev = "/dev/net/tun";

  if( (fd = open(clonedev , O_RDWR)) < 0 ) {
    perror("open");
    return fd;
  }

  memset(&ifr, 0, sizeof ifr );

  ifr.ifr_flags = flags;

  istrcpy(ifr.ifr_name, dev, IFNAMSIZ);

  if( (err = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0 ) {
    perror("ioctl");
    close(fd);
    return err;
  }

  return fd;
}

void *sendthread( void *foo ) {
  unsigned char buffer[65536];
  Sdr sdr;
  Object extent, bundleZco, newBundle;
  sdr = bp_get_sdr( );

  for(;;) {
    int lineLength = read( tap_fd, buffer, sizeof buffer );
    if( lineLength < 1 ) {
      puts("read");
      exit(-1);
    }

    /* QoS
       Should DTNTAP only twiddle 802.1p PCP bits instead of IP DSCP bits?
       PCP only works on VLANs.
    */
    int classOfService = BP_BULK_PRIORITY;
    if ((buffer[12] == 0x8 && buffer[13] == 0 && (buffer[15] & 0xfc) == 0xb8)
    || (buffer[12] == 0x86 && buffer[13] == 0xdd && (buffer[14] & 0xf) == 0xb
    	&& (buffer[15] & 0xc0) == 0x80))
    {
      classOfService = BP_EXPEDITED_PRIORITY;
    }

    for( int i = 3; i < EidListLen; i++ ) {
      sdr_begin_xn(sdr);
      extent = sdr_malloc(sdr, lineLength/*sizeof buffer*/ );
      if (extent == 0) {
        sdr_cancel_xn(sdr);
        //puts( "No space for ZCO extent." );
        //exit(-1);
        continue;
      }

      sdr_write(sdr, extent, (char *)buffer, lineLength);
      bundleZco = zco_create(sdr, ZcoSdrSource, extent,
                             0, lineLength, ZcoOutbound, 0);

      if (sdr_end_xn(sdr) < 0 || bundleZco == 0) {
        puts("Can't create ZCO extent.");
        //exit(-1);
      } else {
        if (bp_send(sap, EidList[i]/*destEid*/, NULL, /*300*/10,
                    classOfService, NoCustodyRequested,
                    0, 0, NULL, bundleZco, &newBundle) < 1) {
          puts("bpsource can't send ADU.");
          exit(-1);
        }
      }
    }
  }
  pthread_exit( NULL );
}

void *recvthread( void *foo ) {
  BpDelivery dlv;
  Sdr sdr;
  unsigned char buffer[65536];
  ZcoReader reader;
  sdr = bp_get_sdr( );
  for(;;) {
    if (bp_receive(sap, &dlv, BP_BLOCKING) < 0) {
      puts("bpsink bundle reception failed.");
      exit(-1);
    }
    if (dlv.result == BpPayloadPresent) {
      int contentLength = zco_source_data_length(sdr, dlv.adu);
      if (contentLength <= sizeof buffer) {
        sdr_begin_xn(sdr);
        zco_start_receiving( dlv.adu, &reader);
        int len = zco_receive_source(sdr, &reader,
                                 contentLength, (char *)buffer);

        if (sdr_end_xn(sdr) < 0 || len < 0) {
          puts( "Can't handle delivery." );
          exit(-1);
        }

        int rc = write( tap_fd, buffer, contentLength );
        if( rc != contentLength ) {
          puts( "write" );
          exit(-1);
        }
      }
    } else {
      //puts("unexpected delivery result");
    }
    bp_release_delivery( &dlv, 1 );
  }
  pthread_exit( NULL );
}

int main( int argc, char **argv ) {
  puts( "Copyright (c) 2015, California Institute of Technology.\n"
        "All rights reserved.  Based on Government Sponsored Research under contracts\n"
        "NAS7-1407 and/or NAS7-03001." );
  if( argc < 4 ) {
    printf( "Usage: %s <device> <own eid> <dest eid1> [dest eid2] ...\n", *argv );
    exit(0);
  }

  if_name = argv[ 1 ];
  EidList = argv;
  EidListLen = argc;

  if (bp_attach() < 0) {
    putErrmsg("Can't attach to BP.", NULL);
    exit(0);
  }
  if (bp_open(EidList[ 2 ], &sap) < 0) {
    putErrmsg("Can't open own endpoint.", EidList[ 2 ]);
    exit(0);
  }

  /* initialize tun/tap interface */
  if ( (tap_fd = tun_alloc(if_name, flags | IFF_NO_PI)) < 0 ) {
    printf("Error connecting to tun/tap interface %s!\n", if_name);
    exit(1);
  }

  pthread_t threads[3];
  pthread_attr_t attr;
  int rc;
  long t = 0;
  pthread_attr_init( &attr );
  pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_JOINABLE );
  rc = pthread_create( &threads[0], NULL, recvthread, (void *)t );
  if( rc ) {
    printf( "pthread_create() returned %d\n", rc );
    exit(-1);
  }
  rc = pthread_create( &threads[1], NULL, sendthread, (void *)t );
  if( rc ) {
    printf( "pthread_create() returned %d\n", rc );
    exit(-1);
  }

  pthread_exit( NULL );
}

