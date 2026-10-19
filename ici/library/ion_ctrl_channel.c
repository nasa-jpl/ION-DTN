/*
	ion_ctrl_channel.c:	Unix domain socket control channel
				implementation with TLV framing in
				network byte order.

	Author: iondev33

	Copyright (c) 2026, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
								*/

#include "platform.h"
#include "ion_ctrl_channel.h"

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/uio.h>
#include <poll.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <arpa/inet.h>

#ifdef __linux__
#include <endian.h>
#else
/*	Portable htobe64/be64toh for non-Linux systems.			*/
#include <arpa/inet.h>
static uint64_t	htobe64_portable(uint64_t val)
{
	uint32_t	hi = htonl((uint32_t) (val >> 32));
	uint32_t	lo = htonl((uint32_t) (val & 0xFFFFFFFF));

	return ((uint64_t) hi) | (((uint64_t) lo) << 32);
}
#define htobe64(x)	htobe64_portable(x)
#define be64toh(x)	htobe64_portable(x)
#endif

/*	Internal: read exactly 'len' bytes from fd.
 *	Returns 0 on success, -1 on error or short read.		*/

static int	readFully(int fd, char *buf, size_t len)
{
	size_t	total = 0;

	while (total < len)
	{
		ssize_t	n = read(fd, buf + total, len - total);

		if (n <= 0)
		{
			return -1;	/*	Disconnect or error.	*/
		}

		total += n;
	}

	return 0;
}

/*	Internal: ensure the parent directory of a socket path
 *	exists.  Creates it with mode 0750 if needed.			*/

static void	ensureParentDir(const char *path)
{
	char	dir[256];
	char	*slash;

	snprintf(dir, sizeof(dir), "%s", path);
	slash = strrchr(dir, '/');
	if (slash != NULL && slash != dir)
	{
		*slash = '\0';
		mkdir(dir, 0750);	/*	Ignore error if exists.	*/
	}
}

int	ctrl_server_create(CtrlChannel *ch, const char *path,
		int keepalive_interval, int keepalive_timeout)
{
	struct sockaddr_un	addr;
	int			fd;

	if (ch == NULL || path == NULL)
	{
		return -1;
	}

	memset(ch, 0, sizeof(CtrlChannel));
	ch->server_fd = -1;
	ch->conn_fd = -1;
	ch->keepalive_interval = keepalive_interval;
	ch->keepalive_timeout = keepalive_timeout;
	snprintf(ch->path, sizeof(ch->path), "%s", path);

	ensureParentDir(path);

	/*	Remove any stale socket file.				*/

	unlink(path);

	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd < 0)
	{
		perror("ctrl_server_create: socket");
		return -1;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", path);

	if (bind(fd, (struct sockaddr *) &addr, sizeof(addr)) < 0)
	{
		perror("ctrl_server_create: bind");
		close(fd);
		return -1;
	}

	if (listen(fd, 1) < 0)
	{
		perror("ctrl_server_create: listen");
		close(fd);
		unlink(path);
		return -1;
	}

	ch->server_fd = fd;
	return 0;
}

int	ctrl_server_accept(CtrlChannel *ch)
{
	int	fd;

	if (ch == NULL || ch->server_fd < 0)
	{
		return -1;
	}

	fd = accept(ch->server_fd, NULL, NULL);
	if (fd < 0)
	{
		perror("ctrl_server_accept: accept");
		return -1;
	}

	ch->conn_fd = fd;
	ch->last_recv_time = time(NULL);
	ch->last_send_time = time(NULL);
	return 0;
}

int	ctrl_client_connect(CtrlChannel *ch, const char *path,
		int keepalive_interval, int keepalive_timeout)
{
	struct sockaddr_un	addr;
	int			fd;

	if (ch == NULL || path == NULL)
	{
		return -1;
	}

	memset(ch, 0, sizeof(CtrlChannel));
	ch->server_fd = -1;
	ch->conn_fd = -1;
	ch->keepalive_interval = keepalive_interval;
	ch->keepalive_timeout = keepalive_timeout;
	snprintf(ch->path, sizeof(ch->path), "%s", path);

	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd < 0)
	{
		perror("ctrl_client_connect: socket");
		return -1;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", path);

	if (connect(fd, (struct sockaddr *) &addr, sizeof(addr)) < 0)
	{
		perror("ctrl_client_connect: connect");
		close(fd);
		return -1;
	}

	ch->conn_fd = fd;
	ch->last_recv_time = time(NULL);
	ch->last_send_time = time(NULL);
	return 0;
}

int	ctrl_send(CtrlChannel *ch, const CtrlMsg *msg)
{
	char		hdr[CTRL_HDR_SIZE];
	char		pbuf[CTRL_MAX_PAYLOAD];
	uint16_t	slot_net;
	uint64_t	xfer_net;
	uint16_t	plen_net;
	struct iovec	iov[2];
	int		iovcnt;
	ssize_t		n;

	if (ch == NULL || ch->conn_fd < 0 || msg == NULL)
	{
		return -1;
	}

	/*	Serialize header in network byte order.			*/

	hdr[0] = (char) msg->type;
	hdr[1] = 0;	/*	Reserved.				*/

	slot_net = htons(msg->slot_index);
	memcpy(hdr + 2, &slot_net, 2);

	xfer_net = htobe64(msg->xfer_id);
	memcpy(hdr + 4, &xfer_net, 8);

	plen_net = htons(msg->payload_len);
	memcpy(hdr + 12, &plen_net, 2);

	iov[0].iov_base = hdr;
	iov[0].iov_len = CTRL_HDR_SIZE;
	iovcnt = 1;

	if (msg->payload_len > 0)
	{
		memcpy(pbuf, msg->payload, msg->payload_len);
		iov[1].iov_base = pbuf;
		iov[1].iov_len = msg->payload_len;
		iovcnt = 2;
	}

	/*	writev is atomic for small messages on Unix domain
	 *	sockets.  Blocks if the socket buffer is full
	 *	(intentional backpressure).				*/

	n = writev(ch->conn_fd, iov, iovcnt);
	if (n < 0)
	{
		return -1;
	}

	ch->last_send_time = time(NULL);
	return 0;
}

int	ctrl_recv(CtrlChannel *ch, CtrlMsg *msg, int timeout_ms)
{
	struct pollfd	pfd;
	int		ret;
	char		hdr[CTRL_HDR_SIZE];
	uint16_t	slot_net;
	uint64_t	xfer_net;
	uint16_t	plen_net;

	if (ch == NULL || ch->conn_fd < 0 || msg == NULL)
	{
		return -1;
	}

	pfd.fd = ch->conn_fd;
	pfd.events = POLLIN;

	ret = poll(&pfd, 1, timeout_ms);
	if (ret < 0)
	{
		if (errno == EINTR)
		{
			return 0;	/*	Treat as timeout.	*/
		}

		return -1;
	}

	if (ret == 0)
	{
		return 0;		/*	Timeout.		*/
	}

	if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL))
	{
		return -1;		/*	Disconnect.		*/
	}

	/*	Read the 14-byte fixed header.				*/

	if (readFully(ch->conn_fd, hdr, CTRL_HDR_SIZE) < 0)
	{
		return -1;
	}

	/*	Deserialize from network byte order.			*/

	memset(msg, 0, sizeof(CtrlMsg));
	msg->type = (uint8_t) hdr[0];

	memcpy(&slot_net, hdr + 2, 2);
	msg->slot_index = ntohs(slot_net);

	memcpy(&xfer_net, hdr + 4, 8);
	msg->xfer_id = be64toh(xfer_net);

	memcpy(&plen_net, hdr + 12, 2);
	msg->payload_len = ntohs(plen_net);

	/*	Read payload if present.				*/

	if (msg->payload_len > 0)
	{
		if (msg->payload_len > CTRL_MAX_PAYLOAD)
		{
			return -1;	/*	Protocol error.		*/
		}

		if (readFully(ch->conn_fd, msg->payload,
				msg->payload_len) < 0)
		{
			return -1;
		}
	}

	ch->last_recv_time = time(NULL);
	return 1;
}

int	ctrl_send_keepalive(CtrlChannel *ch)
{
	CtrlMsg	msg;

	ctrl_msg_init(&msg, CTRL_KEEPALIVE, 0, 0, NULL, 0);
	return ctrl_send(ch, &msg);
}

int	ctrl_check_timeout(CtrlChannel *ch)
{
	if (ch == NULL || ch->keepalive_timeout <= 0)
	{
		return 0;
	}

	return (time(NULL) - ch->last_recv_time) > ch->keepalive_timeout;
}

int	ctrl_keepalive_due(CtrlChannel *ch)
{
	if (ch == NULL || ch->keepalive_interval <= 0)
	{
		return 0;
	}

	return (time(NULL) - ch->last_send_time) >= ch->keepalive_interval;
}

void	ctrl_disconnect(CtrlChannel *ch)
{
	if (ch == NULL)
	{
		return;
	}

	if (ch->conn_fd >= 0)
	{
		close(ch->conn_fd);
		ch->conn_fd = -1;
	}
}

void	ctrl_destroy(CtrlChannel *ch)
{
	if (ch == NULL)
	{
		return;
	}

	ctrl_disconnect(ch);

	if (ch->server_fd >= 0)
	{
		close(ch->server_fd);
		ch->server_fd = -1;
	}

	if (ch->path[0] != '\0')
	{
		unlink(ch->path);
		ch->path[0] = '\0';
	}
}

void	ctrl_msg_init(CtrlMsg *msg, uint8_t type,
		uint16_t slot_index, uint64_t xfer_id,
		const char *payload, uint16_t payload_len)
{
	if (msg == NULL)
	{
		return;
	}

	memset(msg, 0, sizeof(CtrlMsg));
	msg->type = type;
	msg->slot_index = slot_index;
	msg->xfer_id = xfer_id;

	if (payload != NULL && payload_len > 0)
	{
		if (payload_len > CTRL_MAX_PAYLOAD)
		{
			payload_len = CTRL_MAX_PAYLOAD;
		}

		memcpy(msg->payload, payload, payload_len);
		msg->payload_len = payload_len;
	}
}
