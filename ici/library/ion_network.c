/*
 * ion_network.c - Implementation of dual-stack network functions
 * Uses getaddrinfo() for automatic IPv4/IPv6 handling
 */

#include "ion_network.h"
#include "ion.h"

#include <pthread.h> /* addr_cache_mutex */

/* Address cache for frequent resolutions */
typedef struct
{
	char endpoint[MAX_FQDN_LEN + 16]; /* padding for port and alignment margin */
	IonNetworkAddress cached_addr;
	time_t            cache_time;
	int               is_valid;
	int               failed_count;
} NetworkAddressCache;

/*
 * The cache was a single unlocked entry.  Under ION_LWT -- RTEMS and the cFS
 * integration -- every daemon shares one address space, so that entry is
 * concurrently mutable state with no mutex; and a single entry thrashes
 * between peers whenever more than one outduct resolves.  A small table under
 * one mutex fixes both.  The mutex is held only while the table is read or
 * written, never across getaddrinfo().
 */
#define ION_ADDRESS_CACHE_SLOTS (8)

static NetworkAddressCache addr_cache[ION_ADDRESS_CACHE_SLOTS];
static pthread_mutex_t	   addr_cache_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * Returns the slot holding this endpoint, or NULL if absent. Caller holds
 * addr_cache_mutex.
 */
static NetworkAddressCache *findCacheEntry(const char *endpoint)
{
	int i;

	for (i = 0; i < ION_ADDRESS_CACHE_SLOTS; i++)
	{
		if (addr_cache[i].endpoint[0] != '\0' &&
				strcmp(addr_cache[i].endpoint, endpoint) == 0)
		{
			return &addr_cache[i];
		}
	}

	return NULL;
}

/*
 * Returns the slot to use for this endpoint: the one already holding it, else
 * an unused one, else the least recently resolved.  Caller holds
 * addr_cache_mutex.
 */
static NetworkAddressCache *claimCacheEntry(const char *endpoint)
{
	NetworkAddressCache *entry;
	NetworkAddressCache *oldest;
	int		     i;

	entry = findCacheEntry(endpoint);
	if (entry)
	{
		return entry;
	}

	oldest = &addr_cache[0];
	for (i = 0; i < ION_ADDRESS_CACHE_SLOTS; i++)
	{
		if (addr_cache[i].endpoint[0] == '\0')
		{
			return &addr_cache[i];
		}

		if (addr_cache[i].cache_time < oldest->cache_time)
		{
			oldest = &addr_cache[i];
		}
	}

	memset(oldest, 0, sizeof *oldest);
	return oldest;
}

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

	/*
	 * Wildcard recognition.  "" and "*" both mean "all interfaces,
	 * whichever address families this host has", and are canonicalized to
	 * "*" so that the resolver can tell them apart from an explicit "::".
	 * Each wildcard spelling is pinned to a family hint here, so that the
	 * address finally bound never depends on the order in which
	 * getaddrinfo() happens to return the passive wildcards -- an order
	 * POSIX does not specify.
	 */
	if (spec->hostname[0] == '\0' || strcmp(spec->hostname, "*") == 0)
	{
		istrcpy(spec->hostname, "*", sizeof(spec->hostname));
		spec->family_hint = AF_UNSPEC; /* Prefer IPv6. */
		spec->is_numeric_host = 0;
	}
	else if (strcmp(spec->hostname, "0.0.0.0") == 0)
	{
		/* Explicit IPv4-only wildcard. */
		spec->family_hint = AF_INET;
	}

	/*
	 * Handle special hostname "@".  Note that "@" is the local host name,
	 * not a wildcard: it must keep resolving to a specific address.
	 */
	if (strcmp(spec->hostname, "@") == 0)
	{
		getNameOfHost(spec->hostname, sizeof(spec->hostname));
		spec->is_numeric_host = 0;
	}

	return 0;
}

static int isWildcardHost(const char *hostname)
{
	return (strcmp(hostname, "*") == 0 || strcmp(hostname, "::") == 0 ||
			strcmp(hostname, "0.0.0.0") == 0);
}

int isDualStackWildcard(const char *endpoint)
{
	IonEndpointSpec spec;

	/*
	 * Only the family-agnostic wildcard -- an omitted host or "*" -- asks
	 * a listening socket to serve both address families.  An explicit
	 * "[::]" is the IPv6 wildcard and "0.0.0.0" the IPv4 wildcard; each
	 * carries its own family hint and stays single-family.
	 */
	if (!endpoint || parseNetworkEndpoint(endpoint, &spec) < 0)
	{
		return 0;
	}

	return (isWildcardHost(spec.hostname) && spec.family_hint == AF_UNSPEC);
}

/*
 * Resolves the endpoint within a single address family.  Returns 0 on success;
 * on failure returns -1 and reports the getaddrinfo() status in *gaiStatus (0
 * if getaddrinfo() itself succeeded but no returned address was usable).
 * Diagnostics are left to the caller, so that a failed first attempt in a
 * family fallback sequence doesn't post an error for a resolution that ends up
 * succeeding.
 */
static int resolveInFamily(const IonEndpointSpec *spec,
		IonNetworkAddress *result, int socket_type, int protocol,
		int family, int isWildcard, const char *service, int *gaiStatus)
{
	struct addrinfo hints, *res, *rp;
	const char     *node;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = family;
	hints.ai_socktype = socket_type;
	hints.ai_protocol = protocol;

	/*
	 * AI_ADDRCONFIG is deliberately not set: it would suppress the IPv6
	 * wildcard on a host that has no IPv6 address configured yet, which is
	 * exactly the address a passive socket needs to bind.
	 */
	hints.ai_flags = 0;

	if (isWildcard)
	{
		/*
		 * Passive resolution: the kernel supplies the wildcard address
		 * of the requested family.
		 */
		hints.ai_flags |= AI_PASSIVE;
		node = NULL;
	}
	else
	{
		if (spec->is_numeric_host)
		{
			hints.ai_flags |= AI_NUMERICHOST;
		}

		node = spec->hostname;
	}

	*gaiStatus = getaddrinfo(node, service, &hints, &res);
	if (*gaiStatus != 0)
	{
		return -1;
	}

	/*
	 * Take the first returned address whose family this host can actually
	 * create a socket for.
	 */
	for (rp = res; rp != NULL; rp = rp->ai_next)
	{
		int test_sock = socket(rp->ai_family, rp->ai_socktype,
				rp->ai_protocol);

		if (test_sock == -1)
		{
			continue; /* Family unsupported. */
		}

		closesocket(test_sock);
		memcpy(&result->addr, rp->ai_addr, rp->ai_addrlen);
		result->addr_len = rp->ai_addrlen;
		result->family = rp->ai_family;

		freeaddrinfo(res);
		return 0;
	}

	freeaddrinfo(res);
	return -1;
}

int resolveNetworkAddressEx(const IonEndpointSpec *spec,
		IonNetworkAddress *result, int socket_type, int protocol)
{
	const char *service;
	int	    isWildcard;
	int	    status = 0;

	if (!spec || !result)
	{
		return -1;
	}

	service = (spec->service[0]) ? spec->service : "4556";
	isWildcard = isWildcardHost(spec->hostname);

	if (isWildcard && spec->family_hint == AF_UNSPEC)
	{
		/*
		 * "*" (or an omitted host) means "all interfaces, whichever
		 * families this host has".  Prefer the IPv6 wildcard, so that
		 * one socket can serve both families, but fall back to the
		 * IPv4 wildcard on a host whose kernel has no IPv6 support at
		 * all. The preference is expressed as two explicit attempts
		 * rather than as a reliance on the order in which
		 * getaddrinfo() returns the two passive wildcards, which POSIX
		 * does not specify.
		 */
		if (resolveInFamily(spec, result, socket_type, protocol,
				    AF_INET6, 1, service, &status) == 0)
		{
			return 0;
		}

		if (resolveInFamily(spec, result, socket_type, protocol,
				    AF_INET, 1, service, &status) == 0)
		{
			writeMemo("[i] No IPv6 support on this host; wildcard \
address is bound as IPv4.");
			return 0;
		}
	}
	else
	{
		if (resolveInFamily(spec, result, socket_type, protocol,
				    spec->family_hint, isWildcard, service,
				    &status) == 0)
		{
			return 0;
		}
	}

	if (status != 0)
	{
		char error_msg[512];

		snprintf(error_msg, sizeof(error_msg),
				"getaddrinfo failed for %s:%s - %s",
				spec->hostname, service, gai_strerror(status));
		putErrmsg("Network address resolution failed", error_msg);
	}
	else
	{
		putErrmsg("No usable addresses found", spec->hostname);
	}

	return -1;
}

/* Backward compatibility - defaults to UDP */
int resolveNetworkAddress(const IonEndpointSpec *spec, IonNetworkAddress *result)
{
	return resolveNetworkAddressEx(spec, result, SOCK_DGRAM, IPPROTO_UDP);
}

/* TCP convenience function */
int resolveNetworkAddressTCP(const IonEndpointSpec *spec,
		IonNetworkAddress		   *result)
{
	return resolveNetworkAddressEx(spec, result, SOCK_STREAM, IPPROTO_TCP);
}

int resolveNetworkAddressPassive(const char *endpoint, unsigned short defaultPort,
		int socket_type, int protocol, IonNetworkAddress *result)
{
	IonEndpointSpec spec;

	if (!endpoint || !result)
	{
		return -1;
	}

	if (parseNetworkEndpoint(endpoint, &spec) < 0)
	{
		return -1;
	}

	/*
	 * A bind-side caller supplies its own default port, since the shared
	 * resolver's default is BP-specific and wrong for other protocols.
	 */
	if (spec.service[0] == '\0' && defaultPort != 0)
	{
		snprintf(spec.service, sizeof(spec.service), "%hu", defaultPort);
		spec.port = defaultPort;
	}

	/*
	 * No caching here.  A bind is resolved once at startup, so the cache
	 * would carry no benefit, and keeping passive resolutions out of it
	 * leaves the table for the outduct peers it exists to serve.
	 */

	return resolveNetworkAddressEx(&spec, result, socket_type, protocol);
}

int resolveNetworkAddressCached(const char *endpoint, IonNetworkAddress *result)
{
	IonEndpointSpec	     spec;
	NetworkAddressCache *entry;
	time_t		     current_time;
	int		     failures;

	if (!endpoint || !result)
	{
		return -1;
	}

	current_time = time(NULL);

	/* Check cache */
	pthread_mutex_lock(&addr_cache_mutex);
	entry = findCacheEntry(endpoint);
	if (entry && entry->is_valid &&
			(current_time - entry->cache_time) <
					ION_ADDRESS_CACHE_INTERVAL_SEC)
	{
		*result = entry->cached_addr;
		pthread_mutex_unlock(&addr_cache_mutex);
		return 0;
	}

	pthread_mutex_unlock(&addr_cache_mutex);

	/* Parse and resolve */
	if (parseNetworkEndpoint(endpoint, &spec) < 0)
	{
		return -1;
	}

	if (resolveNetworkAddress(&spec, result) < 0)
	{
		/*
		 * Failures are counted per endpoint.  A peer whose name will
		 * not resolve must not push an unrelated peer over the retry
		 * limit, nor evict that peer's valid entry -- the limit stops
		 * the daemon, so charging it to the wrong endpoint is fatal to
		 * a duct that was resolving perfectly well.
		 */
		pthread_mutex_lock(&addr_cache_mutex);
		entry = claimCacheEntry(endpoint);
		istrcpy(entry->endpoint, endpoint, sizeof entry->endpoint);
		entry->cache_time = current_time;
		entry->is_valid = 0;
		entry->failed_count++;
		failures = entry->failed_count;
		pthread_mutex_unlock(&addr_cache_mutex);

		if (failures >= ION_MAX_FAILED_LOOKUPS)
		{
			putErrmsg("Maximum DNS failures reached", endpoint);
			return -2; /* Signal to stop trying */
		}

		return -1;
	}

	/* Update cache */
	pthread_mutex_lock(&addr_cache_mutex);
	entry = claimCacheEntry(endpoint);
	istrcpy(entry->endpoint, endpoint, sizeof entry->endpoint);
	entry->cached_addr = *result;
	entry->cache_time = current_time;
	entry->is_valid = 1;
	entry->failed_count = 0;
	pthread_mutex_unlock(&addr_cache_mutex);

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
	const void          *addr_ptr;
	unsigned short port;
	char           addr_str[INET6_ADDRSTRLEN];

	if (!addr || !buffer)
	{
		return "invalid";
	}

	if (addr->family == AF_INET)
	{
		const struct sockaddr_in *sin = (const struct sockaddr_in *) &addr->addr;
		addr_ptr = &sin->sin_addr;
		port = ntohs(sin->sin_port);

		inet_ntop(AF_INET, addr_ptr, addr_str, sizeof(addr_str));
		snprintf(buffer, buflen, "%s:%u", addr_str, port);
	}
	else if (addr->family == AF_INET6)
	{
		const struct sockaddr_in6 *sin6 = (const struct sockaddr_in6 *) &addr->addr;
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

int createNetworkSocketEx(int socket_type, const IonNetworkAddress *local_addr,
		int flags, int *socket_fd)
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

	if (local_addr->family == AF_INET6)
	{
		const struct sockaddr_in6 *sin6 =
				(const struct sockaddr_in6 *) &local_addr->addr;
		int v6only = 1;

		/*
		 * Dual-stack is requested by the caller, and only for the
		 * family-agnostic wildcard -- an omitted host or "*" -- which
		 * the resolver binds as the IPv6 wildcard so that one socket
		 * can serve both families.  An explicit "[::]" resolves to the
		 * same unspecified address but is an IPv6 wildcard by spelling,
		 * so its caller asks for IPv6 only; the address alone cannot
		 * tell the two apart, so the distinction rides in on the flags.
		 * The IN6_IS_ADDR_UNSPECIFIED guard keeps an IPv6 literal
		 * single-family even if a caller passes the flag by mistake.
		 * Deriving this from the flags and the resolved address, rather
		 * than from a new structure member, keeps both public
		 * structures byte-identical.
		 */
		if ((flags & ION_SOCK_DUALSTACK) && !(flags & ION_SOCK_V6ONLY) &&
				IN6_IS_ADDR_UNSPECIFIED(&sin6->sin6_addr))
		{
			v6only = 0;
		}

		/* Must precede bind(). */

		if (setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, &v6only,
				    sizeof(v6only)) < 0 &&
				v6only == 0)
		{
			/*
			 * Platform refuses dual-stack sockets, as OpenBSD does
			 * and as some hardened sysctl settings do.  Degrade to
			 * IPv6-only rather than failing to come up at all.
			 */
			writeMemoNote("[i] Dual-stack unavailable; socket \
will accept IPv6 only",
					"IPV6_V6ONLY");
		}
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

int createNetworkSocket(int socket_type, const IonNetworkAddress *local_addr,
		int *socket_fd)
{
	return createNetworkSocketEx(socket_type, local_addr,
			ION_SOCK_DUALSTACK, socket_fd);
}
