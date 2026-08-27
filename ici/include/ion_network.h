/*
 * ion_network.h - Enhanced network address resolution for ION
 * Provides dual-stack IPv4/IPv6 support while preserving legacy API
 */

#ifndef ION_NETWORK_H
#define ION_NETWORK_H

/* POSIX feature test macros - portable across all Unix systems, no GNU */
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200112L  /* POSIX.1-2001: includes getaddrinfo() */
#endif

#include "platform.h"
#include <netdb.h>
#include <sys/socket.h>

#define MAX_FQDN_LEN 255 /* Maximum length of a fully qualified domain name */
#define INET6_ADDR_WITH_PORT_STRLEN (INET6_ADDRSTRLEN + 10) /* [IPv6]:port\0 */

#ifdef __cplusplus
extern "C" {
#endif

/* Enhanced address structure supporting both IPv4 and IPv6 */
typedef struct
{
	struct sockaddr_storage addr;     /* Stores IPv4 or IPv6 address */
	socklen_t               addr_len; /* Length of the address structure */
	int family; /* AF_INET or AF_INET6 for convenience */
} IonNetworkAddress;

/* Endpoint specification parsing result */
typedef struct
{
	char           hostname[MAX_FQDN_LEN + 1]; /* Extracted hostname */
	char           service[16];                /* Port as string */
	unsigned short port;            /* Port as number (convenience) */
	int            family_hint;     /* AF_UNSPEC, AF_INET, or AF_INET6 */
	int            is_numeric_host; /* 1 if hostname is numeric IP */
} IonEndpointSpec;

/* Cache configuration */
#define ION_ADDRESS_CACHE_INTERVAL_SEC 60
#define ION_MAX_FAILED_LOOKUPS         100

/* New dual-stack functions */
/* Core function with full control */
extern int resolveNetworkAddressEx(const IonEndpointSpec *spec,
                IonNetworkAddress *result, int socket_type, int protocol);

/* Backward compatibility function - defaults to UDP */
extern int resolveNetworkAddress(const IonEndpointSpec *spec,
                IonNetworkAddress                      *result);

/* TCP convenience function - only other protocol we need */
extern int resolveNetworkAddressTCP(const IonEndpointSpec *spec,
                IonNetworkAddress                         *result);

/*
 * Passive (bind-side) resolution.  The shared resolver defaults to BP's port
 * when the endpoint names none, which is wrong for every other protocol; a
 * bind-side caller states its own default here.
 */
extern int resolveNetworkAddressPassive(const char *endpoint,
		unsigned short defaultPort, int socket_type, int protocol,
		IonNetworkAddress *result);

/* Cached versions */
extern int resolveNetworkAddressCached(const char *endpoint,
                IonNetworkAddress                 *result);
extern int resolveNetworkAddressCachedTCP(const char *endpoint,
                IonNetworkAddress                    *result);

/* Endpoint parsing */
extern int parseNetworkEndpoint(const char *endpoint, IonEndpointSpec *spec);

/*
 * Reports whether a passive endpoint is the family-agnostic wildcard (an
 * omitted host or "*"), the only spelling for which a listening socket should
 * request dual-stack.  An explicit "[::]" or "0.0.0.0" stays single-family.
 */
extern int isDualStackWildcard(const char *endpoint);
extern const char *formatNetworkAddress(const IonNetworkAddress *addr,
                char *buffer, size_t buflen);

/* Socket creation helpers */

/* Flags for createNetworkSocketEx() */
#define ION_SOCK_DUALSTACK (1) /* Accept IPv4 on an IPv6 socket */
#define ION_SOCK_V6ONLY	   (2) /* Force IPV6_V6ONLY = 1 */

extern int createNetworkSocketEx(int	 socket_type,
		const IonNetworkAddress *local_addr, int flags, int *socket_fd);

/* Backward compatibility wrapper - dual-stack on the wildcard address */
extern int createNetworkSocket(int       socket_type,
                const IonNetworkAddress *local_addr, int *socket_fd);

/* Utility functions */
extern int isIPv6Address(const char *addr_str);
extern int isIPv4Address(const char *addr_str);

/* Dual-stack TCP connection function */
extern int itcp_connect_dualstack(char *socketSpec, unsigned short defaultPort,
                int *sock, IonNetworkAddress *remoteAddr);

#ifdef __cplusplus
}
#endif

#endif /* ION_NETWORK_H */
