/*
 * Minimal stub for netinet/udp.h - RTEMS compatibility
 *
 * This is a minimal stub header for RTEMS BSPs that don't include
 * full BSD networking stack. It provides just enough definitions
 * for ION to compile when using non-network transports (e.g., POSIX
 * message queues).
 *
 * For actual UDP networking, use rtems-libbsd.
 */

#ifndef _NETINET_UDP_H_
#define _NETINET_UDP_H_

#include <sys/types.h>

/*
 * Minimal UDP header structure for compatibility.
 * Based on RFC 768 User Datagram Protocol specification.
 */
struct udphdr {
    uint16_t source;    /* Source port */
    uint16_t dest;      /* Destination port */
    uint16_t len;       /* UDP length */
    uint16_t check;     /* UDP checksum */
};

/* Alternative structure name used on some systems */
#define uh_sport source
#define uh_dport dest
#define uh_ulen  len
#define uh_sum   check

#endif /* _NETINET_UDP_H_ */
