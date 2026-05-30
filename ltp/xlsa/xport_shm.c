/*
	xport_shm.c:	REFERENCE transport backend for the generic LTP
			link service template -- a shared-memory datagram
			ring.

	This is the recommended substrate for benchmarking ION's LTP
	engine on a single host with two ION instances.  Unlike UDP
	loopback it involves no kernel networking, no IP fragmentation,
	and no socket-buffer backpressure, so you measure ION's
	segmentation / reassembly / report-and-checkpoint machinery
	rather than the host network stack.

	It preserves message boundaries natively: each ring slot holds
	exactly one segment plus its length, so xport_recv() returns one
	segment per call -- satisfying the ltpHandleInboundSegment()
	contract without any framing.

	Link impairments (one-way delay, jitter, drop probability, and
	bandwidth) are modeled HERE, in the emulator.  Bandwidth is
	modeled the way a real flight link works: the emulated RADIO is
	the rate authority, the ring is the link service's finite RATE BUFFER,
	a full ring backpressures the LSO (which then stops dequeuing, so
	segments accumulate in the SDR).  The LSO is never paced directly
	-- there is no token bucket -- exactly as a correct flight link
	service would behave.

	ENDPOINT SPEC:	"shm:<name>"  e.g. "shm:ring_1to2".
			A single shared-memory segment ("/xlsa.<name>")
			carries one direction of traffic; the sender writes
			it and the receiver reads it.  Use one ring name
			per direction and have node A's LSO and node B's
			LSI reference the same name (and likewise for the
			reverse direction).  Either side may start first --
			the create-race is resolved internally.

	ENVIRONMENT KNOBS (all optional):
		XLSA_DELAY_US      one-way propagation delay per segment (usec)
		XLSA_JITTER_US     uniform +/- jitter added to delay (usec)
		XLSA_DROP_PPM      drop probability, parts-per-million
		XLSA_RATE_BPS      emulated radio rate, bytes/sec (0 = unlimited)
		XLSA_SLOTS         rate-buffer depth in segments (default 1024);
				   this is the link service's buffering capacity --
				   smaller => backpressure bites sooner

	IMPORTANT -- TWO RATES MUST AGREE:
	XLSA_RATE_BPS is the *actual* link rate enforced here.  ION's
	contact-plan localXmitRate (set via ionadmin) is a SEPARATE
	number: it gates the LSO dequeue loop (must be > 0 or nothing
	flows) and calibrates LTP's retransmission/expectation timers.
	In flight these match the planned radio rate, so set the contact
	rate to match XLSA_RATE_BPS for representative timer behavior.
	Deliberately mismatching them is a useful test: it emulates the
	radio degrading below the rate ION was told to expect.

	NOTE: This file is a clear, self-contained reference.  It uses a
	POSIX shared-memory segment and a mutex/condvar pair.  For a real
	flight link service you replace this entire file with one that drives your
	bus/serial/FPGA, keeping only the xport_* function signatures
	from xlsa.h.  A byte-stream backend MUST add framing -- see
	doc/DESIGN.md, "Message boundaries".
									*/

#include "xlsa.h"
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>

#define XLSA_SLOT_PAYLOAD	(XLSA_BUFSZ)

typedef struct
{
	int		length;			/*	Payload bytes.	*/
	unsigned long	releaseUsec;		/*	Earliest delivery.*/
	char		payload[XLSA_SLOT_PAYLOAD];
} XlsaSlot;

typedef struct
{
	pthread_mutex_t	mutex;
	pthread_cond_t	notEmpty;
	pthread_cond_t	notFull;
	unsigned int	head;			/*	Next to read.	*/
	unsigned int	tail;			/*	Next to write.	*/
	unsigned int	slots;			/*	Ring depth.	*/
	XlsaSlot	slot[];			/*	Flexible array.	*/
} XlsaRing;

struct xport_link_st
{
	XlsaRing	*ring;
	int		fd;
	int		isSender;
	size_t		mapLen;
	char		shmName[128];	/*	"/xlsa.<name>" for unlink.*/
	/*	Impairment configuration (sender side).			*/
	unsigned long	delayUsec;	/*	One-way propagation.	*/
	unsigned long	jitterUsec;
	unsigned long	dropPpm;
	unsigned long	rateBps;	/*	Emulated radio rate.	*/
	unsigned long	channelFreeUsec;/*	Radio serialization clock:
					 *	time the link finishes
					 *	sending all bytes queued
					 *	so far.  NOT a sleep target;
					 *	rate is enforced by the
					 *	receiver-side release
					 *	schedule + bounded ring.	*/
};

static unsigned long	nowUsec(void)
{
	struct timeval	tv;

	getCurrentTime(&tv);
	return ((unsigned long) tv.tv_sec * 1000000UL)
			+ (unsigned long) tv.tv_usec;
}

static unsigned long	envULong(const char *name, unsigned long dflt)
{
	char	*s = getenv(name);

	if (s == NULL || *s == '\0')
	{
		return dflt;
	}

	return strtoul(s, NULL, 10);
}

/*	The two-step startup -- "create with O_EXCL, fall back to open" --
 *	races with the peer.  The loser must wait until the creator has
 *	(a) ftruncated the segment to the full mapLen and (b) initialized
 *	the in-ring mutex/condvars before touching them.  The 'slots' field
 *	in the ring is the publish-after-init sentinel: the creator writes
 *	it LAST, behind a full barrier; the opener polls it.		*/

#define XLSA_INIT_POLL_MAX	1000		/*	~1 second.	*/
#define XLSA_INIT_POLL_USEC	1000

XportLink	*xport_open(const char *endpointSpec, int isSender,
			uvast remoteEngineId)
{
	XportLink	*link;
	const char	*name;
	char		shmName[128];
	unsigned int	slots;
	size_t		mapLen;
	int		created = 0;
	int		attempt;
	struct stat	st;

	(void) remoteEngineId;

	/*	Accept "shm:<name>" or a bare "<name>".			*/

	name = endpointSpec;
	if (strncmp(name, "shm:", 4) == 0)
	{
		name += 4;
	}

	slots = (unsigned int) envULong("XLSA_SLOTS", 1024);
	mapLen = sizeof(XlsaRing) + ((size_t) slots * sizeof(XlsaSlot));

	link = (XportLink *) MTAKE(sizeof(XportLink));
	if (link == NULL)
	{
		putErrmsg("xport_shm: no memory for link.", NULL);
		return NULL;
	}

	memset(link, 0, sizeof(XportLink));
	link->isSender = isSender;
	link->mapLen = mapLen;
	link->delayUsec = envULong("XLSA_DELAY_US", 0);
	link->jitterUsec = envULong("XLSA_JITTER_US", 0);
	link->dropPpm = envULong("XLSA_DROP_PPM", 0);
	link->rateBps = envULong("XLSA_RATE_BPS", 0);
	link->channelFreeUsec = nowUsec();

	/*	One shm segment per ring.  Both the sender (xlso) and the
	 *	receiver (xlsi) on the opposite node reference the same
	 *	endpoint name and therefore the same shm object.	*/

	isprintf(shmName, sizeof shmName, "/xlsa.%s", name);
	istrcpy(link->shmName, shmName, sizeof link->shmName);

	/*	Resolve the create/open race.  Either side may be first.
	 *	If we get EEXIST, the peer beat us to creation; if our
	 *	follow-on plain open hits ENOENT we lost in a window the
	 *	peer unlinked through, so retry the whole sequence.	*/

	for (attempt = 0; attempt < XLSA_INIT_POLL_MAX; attempt++)
	{
		link->fd = shm_open(shmName, O_RDWR | O_CREAT | O_EXCL, 0660);
		if (link->fd >= 0)
		{
			created = 1;
			break;
		}

		if (errno != EEXIST)
		{
			putSysErrmsg("xport_shm: shm_open(CREAT) failed.",
					shmName);
			MRELEASE(link);
			return NULL;
		}

		link->fd = shm_open(shmName, O_RDWR, 0660);
		if (link->fd >= 0)
		{
			break;
		}

		if (errno != ENOENT)
		{
			putSysErrmsg("xport_shm: shm_open(open) failed.",
					shmName);
			MRELEASE(link);
			return NULL;
		}

		microsnooze(XLSA_INIT_POLL_USEC);
	}

	if (link->fd < 0)
	{
		putErrmsg("xport_shm: create/open race did not settle.",
				shmName);
		MRELEASE(link);
		return NULL;
	}

	if (created)
	{
		pthread_mutexattr_t	ma;
		pthread_condattr_t	ca;

		if (ftruncate(link->fd, mapLen) < 0)
		{
			putSysErrmsg("xport_shm: ftruncate failed.", shmName);
			close(link->fd);
			shm_unlink(shmName);
			MRELEASE(link);
			return NULL;
		}

		link->ring = (XlsaRing *) mmap(NULL, mapLen,
				PROT_READ | PROT_WRITE, MAP_SHARED, link->fd, 0);
		if (link->ring == MAP_FAILED)
		{
			putSysErrmsg("xport_shm: mmap failed.", shmName);
			close(link->fd);
			shm_unlink(shmName);
			MRELEASE(link);
			return NULL;
		}

		memset(link->ring, 0, mapLen);

		/*	Initialize process-shared sync primitives FIRST so a
		 *	peer that races in and sees a published slots != 0
		 *	can safely lock the mutex.			*/

		pthread_mutexattr_init(&ma);
		pthread_mutexattr_setpshared(&ma, PTHREAD_PROCESS_SHARED);
		pthread_mutex_init(&link->ring->mutex, &ma);
		pthread_mutexattr_destroy(&ma);

		pthread_condattr_init(&ca);
		pthread_condattr_setpshared(&ca, PTHREAD_PROCESS_SHARED);
		pthread_cond_init(&link->ring->notEmpty, &ca);
		pthread_cond_init(&link->ring->notFull, &ca);
		pthread_condattr_destroy(&ca);

		/*	Publish 'slots' LAST behind a full barrier; it is the
		 *	init-complete sentinel the opener polls.	*/

		__sync_synchronize();
		link->ring->slots = slots;
	}
	else
	{
		/*	Opener: wait for the creator's ftruncate to land.	*/

		for (attempt = 0; attempt < XLSA_INIT_POLL_MAX; attempt++)
		{
			if (fstat(link->fd, &st) < 0)
			{
				putSysErrmsg("xport_shm: fstat failed.",
						shmName);
				close(link->fd);
				MRELEASE(link);
				return NULL;
			}

			if ((size_t) st.st_size >= mapLen)
			{
				break;
			}

			microsnooze(XLSA_INIT_POLL_USEC);
		}

		if ((size_t) st.st_size < mapLen)
		{
			putErrmsg("xport_shm: timed out waiting for ring "
					"truncation; check XLSA_SLOTS matches "
					"across processes.", shmName);
			close(link->fd);
			MRELEASE(link);
			return NULL;
		}

		link->ring = (XlsaRing *) mmap(NULL, mapLen,
				PROT_READ | PROT_WRITE, MAP_SHARED, link->fd, 0);
		if (link->ring == MAP_FAILED)
		{
			putSysErrmsg("xport_shm: mmap failed.", shmName);
			close(link->fd);
			MRELEASE(link);
			return NULL;
		}

		/*	Wait for the creator's publish of 'slots'.	*/

		for (attempt = 0; attempt < XLSA_INIT_POLL_MAX; attempt++)
		{
			__sync_synchronize();
			if (link->ring->slots != 0)
			{
				break;
			}

			microsnooze(XLSA_INIT_POLL_USEC);
		}

		if (link->ring->slots == 0)
		{
			putErrmsg("xport_shm: timed out waiting for ring init.",
					shmName);
			munmap(link->ring, mapLen);
			close(link->fd);
			MRELEASE(link);
			return NULL;
		}

		/*	Both sides must agree on the slot count -- otherwise
		 *	one mmap is smaller than the other and producer/
		 *	consumer step out of bounds.			*/

		if (link->ring->slots != slots)
		{
			putErrmsg("xport_shm: XLSA_SLOTS disagrees with peer.",
					itoa((int) link->ring->slots));
			munmap(link->ring, mapLen);
			close(link->fd);
			MRELEASE(link);
			return NULL;
		}
	}

	return link;
}

int	xport_send(XportLink *link, char *segment, int length)
{
	XlsaRing	*ring = link->ring;
	XlsaSlot	*slot;
	unsigned int	next;
	unsigned long	now;
	unsigned long	delay;

	if (length > XLSA_SLOT_PAYLOAD)
	{
		putErrmsg("xport_shm: segment exceeds slot payload.",
				itoa(length));
		return -1;
	}

	/*	Drop model: discard before enqueue, report as accepted
	 *	(link loss, not an error) so LTP's own retransmission
	 *	machinery is what recovers it -- exactly what we want to
	 *	exercise in a benchmark.				*/

	if (link->dropPpm > 0
	&& ((unsigned long) (rand() % 1000000) < link->dropPpm))
	{
		return length;
	}

	/*	Bandwidth model -- the emulated RADIO, not the LSO, is the
	 *	rate authority.  We do NOT sleep here.  Instead we advance a
	 *	serialization clock (channelFreeUsec) by the time the radio
	 *	needs to clock these bytes out, and stamp the segment's
	 *	release time accordingly.  The bounded ring is the link
	 *	service's rate buffer: xport_send blocks below only when that
	 *	buffer is full, which backpressures the LSO (it stops calling
	 *	ltpDequeueOutboundSegment, so segments pile up in the SDR).
	 *	This reproduces the flight flow-control path exactly.	*/

	now = nowUsec();
	if (link->rateBps > 0)
	{
		unsigned long	serUsec;

		serUsec = ((unsigned long) length * 1000000UL) / link->rateBps;

		/*	If the link has been idle, it is free now; otherwise
		 *	this segment queues behind the ones still clocking
		 *	out.  Either way, no sleeping -- the spacing of
		 *	release times is what enforces the rate, and a full
		 *	ring is what enforces backpressure.		*/

		if (link->channelFreeUsec < now)
		{
			link->channelFreeUsec = now;
		}

		link->channelFreeUsec += serUsec;	/*	Tx complete.	*/
	}
	else	/*	Unlimited rate: link is always free now.		*/
	{
		link->channelFreeUsec = now;
	}

	/*	Propagation delay + jitter is added AFTER serialization:
	 *	release = (time fully clocked out) + one-way light time.
	 *	The receiver honors this stamped release time.		*/

	delay = link->delayUsec;
	if (link->jitterUsec > 0)
	{
		long	j;

		j = (long) (rand() % (2 * link->jitterUsec + 1))
				- (long) link->jitterUsec;
		if ((long) delay + j < 0)
		{
			delay = 0;
		}
		else
		{
			delay = (unsigned long) ((long) delay + j);
		}
	}

	pthread_mutex_lock(&ring->mutex);
	next = (ring->tail + 1) % ring->slots;
	while (next == ring->head)	/*	Rate buffer full -- block. */
	{
		/*	This block IS the backpressure to the LSO.  The
		 *	receiver drains at the link rate (via release times),
		 *	so the LSO is released one segment at a time at that
		 *	rate once the buffer is saturated.		*/

		pthread_cond_wait(&ring->notFull, &ring->mutex);
		next = (ring->tail + 1) % ring->slots;
	}

	slot = &ring->slot[ring->tail];
	slot->length = length;
	slot->releaseUsec = link->channelFreeUsec + delay;
	memcpy(slot->payload, segment, length);
	ring->tail = next;
	pthread_cond_signal(&ring->notEmpty);
	pthread_mutex_unlock(&ring->mutex);

	return length;
}

int	xport_recv(XportLink *link, char *buf, int maxlen)
{
	XlsaRing	*ring = link->ring;
	XlsaSlot	*slot;
	int		length;
	unsigned long	now;

	pthread_mutex_lock(&ring->mutex);
	while (ring->head == ring->tail)	/*	Empty -- block.	*/
	{
		pthread_cond_wait(&ring->notEmpty, &ring->mutex);
	}

	slot = &ring->slot[ring->head];

	/*	Honor the delivery time stamped by the sender.		*/

	now = nowUsec();
	if (slot->releaseUsec > now)
	{
		struct timespec	ts;
		unsigned long	wakeUsec = slot->releaseUsec;

		ts.tv_sec = (time_t) (wakeUsec / 1000000UL);
		ts.tv_nsec = (long) ((wakeUsec % 1000000UL) * 1000UL);
		pthread_cond_timedwait(&ring->notEmpty, &ring->mutex, &ts);

		/*	Re-check: a shutdown sentinel may have arrived,
		 *	or time has now passed.				*/

		now = nowUsec();
		if (ring->head == ring->tail)
		{
			pthread_mutex_unlock(&ring->mutex);
			return 0;	/*	Interrupted; retry.	*/
		}

		slot = &ring->slot[ring->head];
		if (slot->releaseUsec > now)
		{
			pthread_mutex_unlock(&ring->mutex);
			return 0;	/*	Not ready yet; retry.	*/
		}
	}

	length = slot->length;
	if (length > maxlen)
	{
		length = maxlen;
	}

	memcpy(buf, slot->payload, length);
	ring->head = (ring->head + 1) % ring->slots;
	pthread_cond_signal(&ring->notFull);
	pthread_mutex_unlock(&ring->mutex);

	return length;
}

void	xport_wake(XportLink *link)
{
	XlsaRing	*ring = link->ring;
	XlsaSlot	*slot;
	unsigned int	next;

	/*	Enqueue the one-byte shutdown sentinel, mirroring the
	 *	UDP driver's self-addressed 1-byte datagram trick.	*/

	pthread_mutex_lock(&ring->mutex);
	next = (ring->tail + 1) % ring->slots;
	if (next != ring->head)
	{
		slot = &ring->slot[ring->tail];
		slot->length = 1;
		slot->releaseUsec = 0;
		slot->payload[0] = '\0';
		ring->tail = next;
	}

	pthread_cond_broadcast(&ring->notEmpty);
	pthread_mutex_unlock(&ring->mutex);
}

void	xport_close(XportLink *link)
{
	if (link == NULL)
	{
		return;
	}

	if (link->ring != NULL && link->ring != MAP_FAILED)
	{
		munmap(link->ring, link->mapLen);
	}

	if (link->fd >= 0)
	{
		close(link->fd);
	}

	/*	Unlink the shm name so the next run starts clean.  Either
	 *	side may shm_unlink; the second call gets ENOENT (harmless).
	 *	Without this a stale ring -- with mutex/condvars from a
	 *	previously-killed process and slots holding garbage
	 *	releaseUsec values -- would persist in the POSIX namespace
	 *	and the next xport_open would attach to it, causing the
	 *	receiver thread to spin (xport_recv returns 0 on every iter
	 *	because the stale slot's releaseUsec is in the far future).	*/

	if (link->shmName[0] != '\0')
	{
		(void) shm_unlink(link->shmName);
	}

	MRELEASE(link);
}
