/*
 * ion_network.c - Implementation of dual-stack network functions
 * Uses getaddrinfo() for automatic IPv4/IPv6 handling
 */

#include "ion_network.h"
#include "ion.h"

/* Address cache for frequent resolutions */
typedef struct
{
	char endpoint[MAX_FQDN_LEN + 16]; /* padding for port and alignment margin */
	IonNetworkAddress cached_addr;
	time_t            cache_time;
	int               is_valid;
	int               failed_count;
} NetworkAddressCache;

static NetworkAddressCache addr_cache = { 0 };

int parseNetworkEndpoint(const char *endpoint, IonEndpointSpec *spec)
{
	const char *bracket_start, *bracket_end, *colon_pos;

	if (!endpoint || !spec)
	{
		return -1;
	}

	memset(spec, 0, sizeof(IonEndpointSpec));
	spec->family_hint = AF_UNSPEC; /* Let getaddrinfo choose */

	/* Handle IPv6 bracket notation: [::1]:4556 */
	bracket_start = strchr(endpoint, '[');
	bracket_end = strchr(endpoint, ']');

	if (bracket_start && bracket_end && (bracket_start < bracket_end))
	{
		/* IPv6 bracket notation - existing logic unchanged */
		size_t host_len = bracket_end - bracket_start - 1;
		if (host_len >= sizeof(spec->hostname))
		{
			return -1;
		}

		strncpy(spec->hostname, bracket_start + 1, host_len);
		spec->hostname[host_len] = '\0';
		spec->family_hint = AF_INET6; /* User explicitly wants IPv6 */
		spec->is_numeric_host = isIPv6Address(spec->hostname);

		/* Look for port after ] */
		colon_pos = strchr(bracket_end, ':');
		if (colon_pos)
		{
			strncpy(spec->service, colon_pos + 1,
			                sizeof(spec->service) - 1);
			spec->service[sizeof(spec->service) - 1] = '\0';
			spec->port = (unsigned short) strtoul(colon_pos + 1,
			                NULL, 10);
		}
	}
	else
	{
		/* No brackets - could be IPv4:port, IPv6, or hostname:port */

		/* NFirst check if the entire string is a valid IPv6 address */
		if (isIPv6Address(endpoint))
		{
			/* It's an IPv6 address without brackets and without port */
			strncpy(spec->hostname, endpoint,
			                sizeof(spec->hostname) - 1);
			spec->hostname[sizeof(spec->hostname) - 1] = '\0';
			spec->family_hint = AF_INET6;
			spec->is_numeric_host = 1;
			/* No port specified, service remains empty - will default to 4556 */
		}
		else
		{
			/* Not a pure IPv6 address, use existing logic for IPv4/hostname */
			colon_pos = strrchr(endpoint, ':'); /* Last colon for IPv4 */

			if (colon_pos)
			{
				/* Check if this might be IPv6 with port in non-standard format */
				/* Count colons to distinguish IPv6:port from IPv4:port */
				int         colon_count = 0;
				const char *p;
				for (p = endpoint; p < colon_pos; p++)
				{
					if (*p == ':')
						colon_count++;
				}

				/* If multiple colons before the last one, likely IPv6 with port */
				if (colon_count > 0)
				{
					/* Could be IPv6:port format like 2001:db8::1:4556 */
					/* This is ambiguous, so we'll be conservative and parse as
           * hostname:port */
					/* User should use brackets for clarity: [2001:db8::1]:4556 */
				}

				/* Parse as hostname:port */
				size_t host_len = colon_pos - endpoint;
				if (host_len >= sizeof(spec->hostname))
				{
					return -1;
				}

				strncpy(spec->hostname, endpoint, host_len);
				spec->hostname[host_len] = '\0';
				strncpy(spec->service, colon_pos + 1,
				                sizeof(spec->service) - 1);
				spec->service[sizeof(spec->service) - 1] = '\0';
				spec->port = (unsigned short) strtoul(colon_pos
				                                + 1,
				                NULL, 10);
			}
			else
			{
				/* No colon found - entire string is hostname */
				strncpy(spec->hostname, endpoint,
				                sizeof(spec->hostname) - 1);
				spec->hostname[sizeof(spec->hostname) - 1] = '\0';
			}

			/* Determine if hostname is numeric after parsing */
			spec->is_numeric_host = isIPv4Address(spec->hostname)
			                || isIPv6Address(spec->hostname);
		}
	}

	/* Handle special hostname "@" */
	if (strcmp(spec->hostname, "@") == 0)
	{
		getNameOfHost(spec->hostname, sizeof(spec->hostname));
		spec->is_numeric_host = 0;
	}

	return 0;
}

int resolveNetworkAddressEx(const IonEndpointSpec *spec,
                IonNetworkAddress *result, int socket_type, int protocol)
{
	struct addrinfo hints, *res, *rp;
	int             status;

	if (!spec || !result)
	{
		return -1;
	}

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = spec->family_hint;
	hints.ai_socktype = socket_type;
	hints.ai_protocol = protocol;
	hints.ai_flags = AI_ADDRCONFIG;

	if (spec->is_numeric_host)
	{
		hints.ai_flags |= AI_NUMERICHOST;
	}

	const char *service = (spec->service[0]) ? spec->service : "4556";

	status = getaddrinfo(spec->hostname, service, &hints, &res);
	if (status != 0)
	{
		char error_msg[512];
		snprintf(error_msg, sizeof(error_msg),
		                "getaddrinfo failed for %s:%s - %s",
		                spec->hostname, service, gai_strerror(status));
		putErrmsg("Network address resolution failed", error_msg);
		return -1;
	}

	/* Try each address - test socket()
     For now, this is used for only UDP client for remote address
     so we don't check for bind() but in the future we might want to */
	for (rp = res; rp != NULL; rp = rp->ai_next)
	{
		int test_sock = socket(rp->ai_family, rp->ai_socktype,
		                rp->ai_protocol);
		if (test_sock == -1)
		{
			continue; /* Can't even create socket for this family */
		}
		/* close test socket */
		closesocket(test_sock);
		memcpy(&result->addr, rp->ai_addr, rp->ai_addrlen);
		result->addr_len = rp->ai_addrlen;
		result->family = rp->ai_family;

		freeaddrinfo(res);
		return 0;
	}

	freeaddrinfo(res);
	putErrmsg("No usable addresses found", spec->hostname);
	return -1;
}

/* Backward compatibility - defaults to UDP */
int resolveNetworkAddress(const IonEndpointSpec *spec, IonNetworkAddress *result)
{
	return resolveNetworkAddressEx(spec, result, SOCK_DGRAM, IPPROTO_UDP);
}

/* TCP convenience function */
int resolveNetworkAddressTCP(const IonEndpointSpec *spec,
                IonNetworkAddress                  *result)
{
	return resolveNetworkAddressEx(spec, result, SOCK_STREAM, IPPROTO_TCP);
}

int resolveNetworkAddressCached(const char *endpoint, IonNetworkAddress *result)
{
	IonEndpointSpec spec;
	time_t          current_time;

	if (!endpoint || !result)
	{
		return -1;
	}

	current_time = time(NULL);

	/* Check cache */
	if (addr_cache.is_valid && strcmp(addr_cache.endpoint, endpoint) == 0
	                && (current_time - addr_cache.cache_time)
	                                < ION_ADDRESS_CACHE_INTERVAL_SEC)
	{

		*result = addr_cache.cached_addr;
		return 0;
	}

	/* Parse and resolve */
	if (parseNetworkEndpoint(endpoint, &spec) < 0)
	{
		return -1;
	}

	if (resolveNetworkAddress(&spec, result) < 0)
	{
		addr_cache.failed_count++;
		if (addr_cache.failed_count >= ION_MAX_FAILED_LOOKUPS)
		{
			putErrmsg("Maximum DNS failures reached", endpoint);
			return -2; /* Signal to stop trying */
		}
		addr_cache.is_valid = 0;
		return -1;
	}

	/* Update cache */
	strncpy(addr_cache.endpoint, endpoint, sizeof(addr_cache.endpoint) - 1);
	addr_cache.endpoint[sizeof(addr_cache.endpoint) - 1] = '\0';
	addr_cache.cached_addr = *result;
	addr_cache.cache_time = current_time;
	addr_cache.is_valid = 1;
	addr_cache.failed_count = 0;

	return 0;
}

/* Cached TCP : Placeholder for TCPCL */
int resolveNetworkAddressCachedTCP(const char *endpoint, IonNetworkAddress *result)
{
	IonEndpointSpec spec;

	if (!endpoint || !result)
	{
		return -1;
	}

	/* For TCP, we might want separate caching or just resolve fresh each time */
	/* TCP connections are less frequent than UDP packets */

	if (parseNetworkEndpoint(endpoint, &spec) < 0)
	{
		return -1;
	}

	return resolveNetworkAddressTCP(&spec, result);
}

const char *formatNetworkAddress(const IonNetworkAddress *addr, char *buffer,
                size_t buflen)
{
	void          *addr_ptr;
	unsigned short port;
	char           addr_str[INET6_ADDRSTRLEN];

	if (!addr || !buffer)
	{
		return "invalid";
	}

	if (addr->family == AF_INET)
	{
		struct sockaddr_in *sin = (const struct sockaddr_in *) &addr->addr;
		addr_ptr = &sin->sin_addr;
		port = ntohs(sin->sin_port);

		inet_ntop(AF_INET, addr_ptr, addr_str, sizeof(addr_str));
		snprintf(buffer, buflen, "%s:%u", addr_str, port);
	}
	else if (addr->family == AF_INET6)
	{
		struct sockaddr_in6 *sin6 = (const struct sockaddr_in6 *) &addr->addr;
		addr_ptr = &sin6->sin6_addr;
		port = ntohs(sin6->sin6_port);

		inet_ntop(AF_INET6, addr_ptr, addr_str, sizeof(addr_str));
		snprintf(buffer, buflen, "[%s]:%u", addr_str, port);
	}
	else
	{
		snprintf(buffer, buflen, "unknown_family_%d", addr->family);
	}

	return buffer;
}

int isIPv4Address(const char *addr_str)
{
	struct sockaddr_in sa;
	return inet_pton(AF_INET, addr_str, &(sa.sin_addr)) != 0;
}

int isIPv6Address(const char *addr_str)
{
	struct sockaddr_in6 sa;
	return inet_pton(AF_INET6, addr_str, &(sa.sin6_addr)) != 0;
}

int createNetworkSocket(int socket_type, const IonNetworkAddress *local_addr,
                int *socket_fd)
{
	int sock;

	if (!local_addr || !socket_fd)
	{
		return -1;
	}

	/* Create socket - family automatically correct from getaddrinfo() */
	sock = socket(local_addr->family, socket_type,
	                (socket_type == SOCK_DGRAM) ? IPPROTO_UDP : IPPROTO_TCP);
	if (sock < 0)
	{
		putSysErrmsg("Can't create network socket", NULL);
		return -1;
	}

	/* Set socket options */
	if (reUseAddress(sock) < 0)
	{
		closesocket(sock);
		return -1;
	}

	/* For IPv6, set IPv6-only to avoid conflicts */
	if (local_addr->family == AF_INET6)
	{
		int ipv6only = 1;
		setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, &ipv6only,
		                sizeof(ipv6only));
	}

	/* Bind to local address - structure automatically correct from getaddrinfo()
   */
	if (bind(sock, (const struct sockaddr *) &local_addr->addr, local_addr->addr_len)
	                < 0)
	{
		closesocket(sock);
		putSysErrmsg("Can't bind network socket", NULL);
		return -1;
	}

	*socket_fd = sock;
	return 0;
}
