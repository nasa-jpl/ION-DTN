/*
 * Minimal stub for netinet/ip.h - RTEMS compatibility
 *
 * This is a minimal stub header for RTEMS BSPs that don't include
 * full BSD networking stack. It provides just enough definitions
 * for ION to compile when using non-network transports (e.g., POSIX
 * message queues).
 *
 * For actual UDP/IP networking, use rtems-libbsd.
 */

#ifndef _NETINET_IP_H_
#define _NETINET_IP_H_

#include <sys/types.h>
#include <netinet/in.h>

/*
 * Minimal IP header structure for compatibility.
 * Based on RFC 791 Internet Protocol specification.
 */
struct iphdr {
#if __BYTE_ORDER == __LITTLE_ENDIAN
    unsigned int ihl:4;
    unsigned int version:4;
#elif __BYTE_ORDER == __BIG_ENDIAN
    unsigned int version:4;
    unsigned int ihl:4;
#else
# error "Please fix <bits/endian.h>"
#endif
    uint8_t tos;
    uint16_t tot_len;
    uint16_t id;
    uint16_t frag_off;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t check;
    uint32_t saddr;
    uint32_t daddr;
    /*The options start here. */
};

/* Protocol field values */
#define IPPROTO_IP      0   /* Dummy protocol for TCP */
#define IPPROTO_ICMP    1   /* Internet Control Message Protocol */
#define IPPROTO_TCP     6   /* Transmission Control Protocol */
#define IPPROTO_UDP     17  /* User Datagram Protocol */

#endif /* _NETINET_IP_H_ */
