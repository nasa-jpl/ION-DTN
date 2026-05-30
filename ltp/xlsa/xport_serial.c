/*
	xport_serial.c:	STUB byte-stream transport backend.

	This is the "now adapt it to your hardware" example.  A serial
	line, UART, SpaceWire link, or FPGA FIFO delivers a BYTE STREAM,
	not datagrams: a single read() may return part of a segment, or
	parts of two segments.  LTP requires one whole segment per
	ltpHandleInboundSegment() call, so a byte-stream backend MUST:

	  1. FRAME on send  -- prepend a length header (and ideally a sync
	     marker + CRC) so the far end can find segment boundaries.
	  2. DEFRAME on recv -- buffer bytes until a full framed segment
	     is present, then hand exactly that segment up.

	This realism is the whole point of testing over a byte stream as
	well as the clean shared-memory ring: the framing/deframing cost
	and partial-read handling is exactly what your flight link service
	must get right.  Replace the read_bytes()/write_bytes() stubs with
	your device I/O and you have a working link service (LSO/LSI)
	skeleton.

	The framing here is deliberately minimal (4-byte big-endian
	length).  Add a sync word and CRC for any real link.
									*/

#include "xlsa.h"
#include <fcntl.h>
#include <poll.h>
#include <termios.h>

#define FRAME_HDR_LEN	4	/*	uint32 big-endian length.	*/

struct xport_link_st
{
	int	fd;		/*	Device descriptor.		*/
	int	wakePipe[2];	/*	[0] read end polled in xport_recv;
				 *	[1] write end posted in xport_wake.
				 *	Lets the daemon shut down cleanly
				 *	without writing a wake byte over
				 *	the wire.			*/
	int	isSender;
	/*	Deframer state (receiver side).				*/
	char	acc[XLSA_BUFSZ + FRAME_HDR_LEN];
	int	accLen;
};

/*	------ Replace these two with your device I/O. ------		*/

static int	write_bytes(int fd, const char *p, int n)
{
	int	total = 0;
	int	w;

	while (total < n)
	{
		w = write(fd, p + total, n - total);
		if (w < 0)
		{
			if (errno == EINTR)
			{
				continue;
			}

			return -1;
		}

		total += w;
	}

	return total;
}

static int	read_bytes(int fd, char *p, int n)
{
	int	r;

	do
	{
		r = read(fd, p, n);
	} while (r < 0 && errno == EINTR);

	return r;		/*	May be a PARTIAL read.		*/
}

/*	-----------------------------------------------------		*/

XportLink	*xport_open(const char *endpointSpec, int isSender,
			uvast remoteEngineId)
{
	XportLink	*link;
	const char	*dev;
	int		flags;

	(void) remoteEngineId;

	dev = endpointSpec;
	if (strncmp(dev, "serial:", 7) == 0)
	{
		dev += 7;
	}

	link = (XportLink *) MTAKE(sizeof(XportLink));
	if (link == NULL)
	{
		return NULL;
	}

	memset(link, 0, sizeof(XportLink));
	link->isSender = isSender;
	link->fd = -1;
	link->wakePipe[0] = -1;
	link->wakePipe[1] = -1;

	link->fd = open(dev, O_RDWR | O_NOCTTY);
	if (link->fd < 0)
	{
		putSysErrmsg("xport_serial: can't open device.", dev);
		MRELEASE(link);
		return NULL;
	}

	/*	A real backend configures termios (raw mode, baud rate),
	 *	or sets up the FPGA/DMA descriptors, here.		*/

	/*	Self-pipe wake.  xport_wake() writes one byte to the write
	 *	end; xport_recv() polls the read end alongside the device
	 *	fd and returns the shutdown sentinel when it fires.  This
	 *	keeps wake-up local and avoids injecting bytes on the wire.	*/

	if (pipe(link->wakePipe) < 0)
	{
		putSysErrmsg("xport_serial: can't create wake pipe.", NULL);
		close(link->fd);
		MRELEASE(link);
		return NULL;
	}

	flags = fcntl(link->wakePipe[0], F_GETFL, 0);
	if (flags >= 0)
	{
		fcntl(link->wakePipe[0], F_SETFL, flags | O_NONBLOCK);
	}

	flags = fcntl(link->wakePipe[1], F_GETFL, 0);
	if (flags >= 0)
	{
		fcntl(link->wakePipe[1], F_SETFL, flags | O_NONBLOCK);
	}

	return link;
}

int	xport_send(XportLink *link, char *segment, int length)
{
	char		hdr[FRAME_HDR_LEN];
	unsigned int	u = (unsigned int) length;

	/*	FRAME: big-endian length prefix, then payload.		*/

	hdr[0] = (char) ((u >> 24) & 0xff);
	hdr[1] = (char) ((u >> 16) & 0xff);
	hdr[2] = (char) ((u >> 8) & 0xff);
	hdr[3] = (char) (u & 0xff);

	if (write_bytes(link->fd, hdr, FRAME_HDR_LEN) < 0)
	{
		return -1;
	}

	if (write_bytes(link->fd, segment, length) < 0)
	{
		return -1;
	}

	return length;
}

int	xport_recv(XportLink *link, char *buf, int maxlen)
{
	char		chunk[1024];
	int		r;
	unsigned int	frameLen;
	struct pollfd	pfd[2];

	while (1)
	{
		/*	DEFRAME: do we already have a complete frame
		 *	buffered?					*/

		if (link->accLen >= FRAME_HDR_LEN)
		{
			frameLen = ((unsigned char) link->acc[0] << 24)
				| ((unsigned char) link->acc[1] << 16)
				| ((unsigned char) link->acc[2] << 8)
				| ((unsigned char) link->acc[3]);

			if ((int) frameLen > maxlen)
			{
				putErrmsg("xport_serial: framed segment too "
						"big.", itoa(frameLen));
				return -1;
			}

			if (link->accLen >= (int) (FRAME_HDR_LEN + frameLen))
			{
				memcpy(buf, link->acc + FRAME_HDR_LEN,
						frameLen);

				/*	Slide remaining bytes down.	*/

				link->accLen -= (FRAME_HDR_LEN + frameLen);
				memmove(link->acc,
					link->acc + FRAME_HDR_LEN + frameLen,
					link->accLen);
				return (int) frameLen;
			}
		}

		/*	Need more bytes.  Block until the device has data,
		 *	or a local wake arrives on the self-pipe.	*/

		pfd[0].fd = link->fd;
		pfd[0].events = POLLIN;
		pfd[0].revents = 0;
		pfd[1].fd = link->wakePipe[0];
		pfd[1].events = POLLIN;
		pfd[1].revents = 0;

		do
		{
			r = poll(pfd, 2, -1);
		} while (r < 0 && errno == EINTR);

		if (r < 0)
		{
			return -1;
		}

		if (pfd[1].revents & POLLIN)
		{
			char	junk[64];

			/*	Drain everything pending so a subsequent
			 *	wake (rare) isn't masked.		*/

			while (read(link->wakePipe[0], junk, sizeof junk) > 0)
			{
				/* drain */
			}

			return 1;		/*	Shutdown sentinel.	*/
		}

		if (!(pfd[0].revents & POLLIN))
		{
			continue;		/*	Spurious; retry.	*/
		}

		r = read_bytes(link->fd, chunk, sizeof chunk);
		if (r < 0)
		{
			return -1;
		}

		if (r == 0)			/*	EOF.		*/
		{
			return 0;		/*	Interrupted; retry.	*/
		}

		if (link->accLen + r > (int) sizeof link->acc)
		{
			putErrmsg("xport_serial: deframe buffer overflow.",
					NULL);
			return -1;
		}

		memcpy(link->acc + link->accLen, chunk, r);
		link->accLen += r;
	}
}

void	xport_wake(XportLink *link)
{
	char	b = 'W';

	/*	Local wake: post one byte to the self-pipe so a blocked
	 *	xport_recv() returns the shutdown sentinel.  We do NOT
	 *	write to the device fd -- that would inject bytes onto a
	 *	real wire, which a flight link service must never do.	*/

	if (link == NULL || link->wakePipe[1] < 0)
	{
		return;
	}

	(void) write(link->wakePipe[1], &b, 1);
}

void	xport_close(XportLink *link)
{
	if (link == NULL)
	{
		return;
	}

	if (link->wakePipe[0] >= 0)
	{
		close(link->wakePipe[0]);
	}

	if (link->wakePipe[1] >= 0)
	{
		close(link->wakePipe[1]);
	}

	if (link->fd >= 0)
	{
		close(link->fd);
	}

	MRELEASE(link);
}
