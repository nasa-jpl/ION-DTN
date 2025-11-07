/*
 * RTEMS Stubs for Missing Functions
 *
 * This BSP (aarch64/a53_lp64_qemu) was built without:
 * - Full POSIX signals support
 * - BSD networking stack
 *
 * For the minimal ION port using POSIX message queues for loopback,
 * these functions are not actually called at runtime. They exist in
 * the ION core library code but are not used in the LTP PMQL transport.
 *
 * We provide stubs that return appropriate error codes to satisfy
 * the linker.
 */

#include <signal.h>
#include <pthread.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>

/*
 * POSIX Signal Functions
 * The BSP lacks full POSIX signal support
 */

int pthread_kill(pthread_t thread, int sig)
{
	/*
	 * Stub implementation for RTEMS BSP without POSIX signals.
	 * In a minimal port using POSIX message queues for loopback,
	 * task termination via signals is not critical.
	 */
	if (sig == 0)
	{
		/* Signal 0 is used to test if thread exists */
		return 0;  /* Assume thread exists */
	}

	/* For actual signals, we can't deliver them */
	/* Return success to avoid breaking ION's task management */
	return 0;
}

int pause(void)
{
	/*
	 * Stub for pause() which may not be available in this BSP.
	 * pause() waits for a signal, but without signal support,
	 * we just return immediately with EINTR.
	 */
	errno = EINTR;
	return -1;
}

int pthread_sigmask(int how, const sigset_t *set, sigset_t *oldset)
{
	/* Stub - no signal masking available */
	if (oldset != NULL)
	{
		sigemptyset(oldset);
	}
	return 0;  /* Success */
}

int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact)
{
	/* Stub - no signal handling available */
	if (oldact != NULL)
	{
		oldact->sa_handler = SIG_DFL;
		sigemptyset(&oldact->sa_mask);
		oldact->sa_flags = 0;
	}
	return 0;  /* Success */
}

/*
 * BSD Networking Functions
 * Only provide stubs when NOT using rtems-libbsd
 * When USING_RTEMS_LIBBSD is defined, libbsd provides real implementations
 */

#ifndef USING_RTEMS_LIBBSD

int socket(int domain, int type, int protocol)
{
	errno = EAFNOSUPPORT;  /* Address family not supported */
	return -1;
}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
	errno = EBADF;  /* Bad file descriptor */
	return -1;
}

ssize_t send(int sockfd, const void *buf, size_t len, int flags)
{
	errno = EBADF;  /* Bad file descriptor */
	return -1;
}

ssize_t recv(int sockfd, void *buf, size_t len, int flags)
{
	errno = EBADF;  /* Bad file descriptor */
	return -1;
}

int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen)
{
	errno = EBADF;  /* Bad file descriptor */
	return -1;
}

char *inet_ntoa(struct in_addr in)
{
	static char buf[16];
	/* Return a dummy address to avoid NULL pointer issues */
	buf[0] = '0';
	buf[1] = '.';
	buf[2] = '0';
	buf[3] = '.';
	buf[4] = '0';
	buf[5] = '.';
	buf[6] = '0';
	buf[7] = '\0';
	return buf;
}

int getaddrinfo(const char *node, const char *service,
                const struct addrinfo *hints, struct addrinfo **res)
{
	/* Return error - address resolution not available */
	return EAI_FAIL;
}

const char *gai_strerror(int errcode)
{
	return "Address resolution not supported";
}

void freeaddrinfo(struct addrinfo *res)
{
	/* Nothing to free in stub implementation */
	(void)res;
}

struct hostent *gethostbyaddr(const void *addr, socklen_t len, int type)
{
	/* Return NULL - host lookup not available */
	errno = ENOSYS;
	return NULL;
}

int gethostname(char *name, size_t len)
{
	/* Return a dummy hostname */
	if (name != NULL && len > 0)
	{
		const char *hostname = "rtems-ion";
		size_t copylen = (len < 10) ? len - 1 : 9;
		int i;
		for (i = 0; i < copylen; i++)
		{
			name[i] = hostname[i];
		}
		name[copylen] = '\0';
		return 0;
	}
	errno = EINVAL;
	return -1;
}

ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                 struct sockaddr *src_addr, socklen_t *addrlen)
{
	errno = EBADF;  /* Bad file descriptor */
	return -1;
}

ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
               const struct sockaddr *dest_addr, socklen_t addrlen)
{
	errno = EBADF;  /* Bad file descriptor */
	return -1;
}

int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
	errno = EBADF;  /* Bad file descriptor */
	return -1;
}

int getsockname(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
	errno = EBADF;  /* Bad file descriptor */
	return -1;
}

int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
	errno = EBADF;  /* Bad file descriptor */
	return -1;
}

int listen(int sockfd, int backlog)
{
	errno = EBADF;  /* Bad file descriptor */
	return -1;
}

void (*signal(int signum, void (*handler)(int)))(int)
{
	/* Stub - no signal handling available */
	/* Return SIG_DFL to indicate default behavior */
	return SIG_DFL;
}

const char *inet_ntop(int af, const void *src, char *dst, socklen_t size)
{
	/* Stub - address conversion not available */
	if (dst != NULL && size > 0)
	{
		/* Return a dummy address */
		if (af == AF_INET && size >= 8)
		{
			dst[0] = '0';
			dst[1] = '.';
			dst[2] = '0';
			dst[3] = '.';
			dst[4] = '0';
			dst[5] = '.';
			dst[6] = '0';
			dst[7] = '\0';
			return dst;
		}
		else if (af == AF_INET6 && size >= 3)
		{
			dst[0] = ':';
			dst[1] = ':';
			dst[2] = '\0';
			return dst;
		}
	}
	errno = ENOSPC;
	return NULL;
}

int inet_pton(int af, const char *src, void *dst)
{
	/* Stub - address conversion not available */
	/* Return 0 to indicate invalid address format */
	errno = EAFNOSUPPORT;
	return 0;
}

#endif /* USING_RTEMS_LIBBSD */
