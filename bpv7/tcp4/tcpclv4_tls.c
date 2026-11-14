/*
	tcpclv4_tls.c:	TCPCLv4 TLS session implementation.

			Uses OpenSSL for TLS, handles contact header
			exchange, message framing over TLS/TCP.

			Per RFC 9174 §4.1, session establishment is:
			  1. TCP connection
			  2. Contact header exchange (ALWAYS plaintext)
			  3. TLS negotiation, only if both sides set
			     CAN_TLS in their contact headers
			  4. SESS_INIT exchange (caller's job, over
			     the now-secured channel)

	Author: iondev33

	Copyright (c) 2026, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
								*/

#include "tcpclv4_tls.h"
#include "tcpclv4_msg.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509v3.h>
#include <openssl/objects.h>

/*	Ceiling on a single message accepted from the peer before
 *	SESS_INIT negotiation gives the caller a chance to impose
 *	its own segment MRU.  Guards recv-buffer growth against a
 *	hostile length field.						*/

#define TCPCLV4_DEFAULT_MAX_MSG	(64ULL * 1024 * 1024)

/*	Session structure							*/

struct tcpclv4_tls_session_s
{
	int		fd;			/* Socket file descriptor */
	SSL		*ssl;			/* OpenSSL handle (NULL if no TLS) */
	SSL_CTX		*ssl_ctx;		/* SSL context (NULL if no TLS) */

	char		*peer_nodeid;		/* Peer node ID from SESS_INIT */
	char		*local_nodeid;		/* Our node ID */

	char		error_msg[512];		/* Last error message */
	int		tls_enabled;		/* 1 if TLS, 0 if plain TCP */
	uint8_t		peer_contact_flags;	/* Peer's contact header flags */
	uint64_t	max_msg_len;		/* Framing sanity limit */

	/* Receive buffer for message framing */
	uint8_t		*recv_buf;
	size_t		recv_buf_cap;
	size_t		recv_buf_len;		/* Bytes currently in buffer */
};

/*	Global initialization flag					*/

static int g_openssl_initialized = 0;

/*	==============================================================
 *	INTERNAL HELPERS
 *	==============================================================
 */

static void set_error(tcpclv4_tls_session_t *sess, const char *msg)
{
	if (sess)
	{
		snprintf(sess->error_msg, sizeof(sess->error_msg), "%s", msg);
	}
}

static void set_error_ssl(tcpclv4_tls_session_t *sess, const char *prefix)
{
	unsigned long	err;
	char		buf[256];

	err = ERR_get_error();
	if (err != 0)
	{
		ERR_error_string_n(err, buf, sizeof(buf));
		snprintf(sess->error_msg, sizeof(sess->error_msg),
			 "%s: %s", prefix, buf);
	}
	else
	{
		snprintf(sess->error_msg, sizeof(sess->error_msg),
			 "%s", prefix);
	}
}

/*	Create TCP socket and connect to host:port			*/

static int tcp_connect(const char *host, int port, char *error_buf,
		       size_t error_buf_len)
{
	struct addrinfo	hints;
	struct addrinfo	*result;
	struct addrinfo	*rp;
	int		sfd;
	int		ret;
	char		port_str[16];

	snprintf(port_str, sizeof(port_str), "%d", port);

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;		/* IPv4 or IPv6 */
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	ret = getaddrinfo(host, port_str, &hints, &result);
	if (ret != 0)
	{
		snprintf(error_buf, error_buf_len, "getaddrinfo: %s",
			 gai_strerror(ret));
		return -1;
	}

	/* Try each address until we successfully connect */
	for (rp = result; rp != NULL; rp = rp->ai_next)
	{
		sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (sfd == -1)
		{
			continue;
		}

		if (connect(sfd, rp->ai_addr, rp->ai_addrlen) == 0)
		{
			break;	/* Success */
		}

		close(sfd);
	}

	freeaddrinfo(result);

	if (rp == NULL)
	{
		snprintf(error_buf, error_buf_len,
			 "Could not connect to %s:%d", host, port);
		return -1;
	}

	/* Disable Nagle's algorithm for low latency */
	int flag = 1;
	setsockopt(sfd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));

	return sfd;
}

/*	Plaintext exact-length read/write on the raw socket, used
 *	only for the contact header exchange (which precedes any
 *	TLS negotiation per RFC 9174 §4.1).				*/

static int read_exact(int fd, uint8_t *buf, size_t len)
{
	size_t	got = 0;
	ssize_t	n;

	while (got < len)
	{
		n = read(fd, buf + got, len - got);
		if (n <= 0)
		{
			if (n < 0 && errno == EINTR)
			{
				continue;
			}

			return -1;
		}

		got += n;
	}

	return 0;
}

static int write_exact(int fd, const uint8_t *buf, size_t len)
{
	size_t	sent = 0;
	ssize_t	n;

	while (sent < len)
	{
		n = write(fd, buf + sent, len - sent);
		if (n <= 0)
		{
			if (n < 0 && errno == EINTR)
			{
				continue;
			}

			return -1;
		}

		sent += n;
	}

	return 0;
}

/*	Exchange contact headers in plaintext (RFC 9174 §4.2).
 *
 *	offer_tls:	1 if we set CAN_TLS in our header.
 *
 *	On success, sess->peer_contact_flags holds the peer's flags.	*/

static int exchange_contact_headers(tcpclv4_tls_session_t *sess,
		int offer_tls)
{
	uint8_t				buf[TCPCLV4_CONTACT_HEADER_LEN];
	size_t				len;
	tcpclv4_contact_header_t	hdr;

	hdr.version = TCPCLV4_PROTOCOL_VERSION;
	hdr.flags = offer_tls ? TCPCLV4_CONTACT_FLAG_CAN_TLS : 0;

	if (tcpclv4_encode_contact_header(&hdr, buf, sizeof(buf), &len) < 0)
	{
		set_error(sess, "Failed to encode contact header");
		return -1;
	}

	if (write_exact(sess->fd, buf, len) < 0)
	{
		set_error(sess, "Failed to send contact header");
		return -1;
	}

	if (read_exact(sess->fd, buf, TCPCLV4_CONTACT_HEADER_LEN) < 0)
	{
		set_error(sess, "Failed to receive contact header");
		return -1;
	}

	if (tcpclv4_decode_contact_header(buf, TCPCLV4_CONTACT_HEADER_LEN,
			&hdr) < 0)
	{
		set_error(sess, "Invalid contact header from peer");
		return -1;
	}

	sess->peer_contact_flags = hdr.flags;

	/* Note: peer_nodeid comes from SESS_INIT, not contact header */

	return 0;
}

/*	Create the SSL context and object and run the handshake.
 *	Called only after contact headers agreed on TLS.		*/

static int setup_tls(tcpclv4_tls_session_t *sess,
		const tcpclv4_tls_config_t *tls_config, int is_server,
		const char *expected_host)
{
	int	ret;

	sess->ssl_ctx = SSL_CTX_new(is_server ? TLS_server_method()
			: TLS_client_method());
	if (sess->ssl_ctx == NULL)
	{
		set_error_ssl(sess, "SSL_CTX_new failed");
		return -1;
	}

	if (SSL_CTX_set_min_proto_version(sess->ssl_ctx, TLS1_2_VERSION) != 1)
	{
		set_error_ssl(sess, "Failed to set minimum TLS version");
		return -1;
	}

	if (tls_config->tls_cert_file)
	{
		if (SSL_CTX_use_certificate_file(sess->ssl_ctx,
				tls_config->tls_cert_file,
				SSL_FILETYPE_PEM) <= 0)
		{
			set_error_ssl(sess, "Failed to load cert");
			return -1;
		}
	}

	if (tls_config->tls_key_file)
	{
		if (SSL_CTX_use_PrivateKey_file(sess->ssl_ctx,
				tls_config->tls_key_file,
				SSL_FILETYPE_PEM) <= 0)
		{
			set_error_ssl(sess, "Failed to load key");
			return -1;
		}
	}

	if (tls_config->tls_ca_file)
	{
		if (SSL_CTX_load_verify_locations(sess->ssl_ctx,
				tls_config->tls_ca_file, NULL) <= 0)
		{
			set_error_ssl(sess, "Failed to load CA");
			return -1;
		}
	}

	if (tls_config->tls_verify_peer)
	{
		SSL_CTX_set_verify(sess->ssl_ctx, is_server
				? (SSL_VERIFY_PEER
					| SSL_VERIFY_FAIL_IF_NO_PEER_CERT)
				: SSL_VERIFY_PEER, NULL);
	}

	/*	Partial-write modes let SSL_write consume what fits
	 *	on a non-blocking socket and be resumed with a moved
	 *	buffer (the session layer's output queue compacts).	*/

	SSL_CTX_set_mode(sess->ssl_ctx, SSL_MODE_ENABLE_PARTIAL_WRITE
			| SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);

	sess->ssl = SSL_new(sess->ssl_ctx);
	if (sess->ssl == NULL)
	{
		set_error_ssl(sess, "SSL_new failed");
		return -1;
	}

	SSL_set_fd(sess->ssl, sess->fd);

	/*	Active side: bind the handshake to the host we dialled so
	 *	the passive peer's certificate is checked against it (RFC
	 *	9174 DNS-ID / Network-ID), not merely chained to a trusted
	 *	CA.  An IP literal is matched as an iPAddress SAN; anything
	 *	else as a DNS name.  Only meaningful when verifying.	*/

	if (!is_server && tls_config->tls_verify_peer && expected_host != NULL)
	{
		X509_VERIFY_PARAM	*vp = SSL_get0_param(sess->ssl);
		struct in6_addr		scratch;

		if (inet_pton(AF_INET, expected_host, &scratch) == 1
		|| inet_pton(AF_INET6, expected_host, &scratch) == 1)
		{
			if (X509_VERIFY_PARAM_set1_ip_asc(vp, expected_host)
					!= 1)
			{
				set_error(sess, "Failed to set expected peer "
						"IP for certificate check");
				return -1;
			}
		}
		else
		{
			X509_VERIFY_PARAM_set_hostflags(vp,
				X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS);
			if (X509_VERIFY_PARAM_set1_host(vp, expected_host, 0)
					!= 1)
			{
				set_error(sess, "Failed to set expected peer "
						"host for certificate check");
				return -1;
			}
		}
	}

	ret = is_server ? SSL_accept(sess->ssl) : SSL_connect(sess->ssl);
	if (ret != 1)
	{
		set_error_ssl(sess, is_server ? "TLS accept failed"
				: "TLS handshake failed");
		return -1;
	}

	sess->tls_enabled = 1;
	return 0;
}

/*	Allocate and initialize a bare session object.			*/

static tcpclv4_tls_session_t *session_alloc(const char *local_node_id)
{
	tcpclv4_tls_session_t	*sess;

	if (!g_openssl_initialized)
	{
		tcpclv4_tls_init();
	}

	sess = calloc(1, sizeof(*sess));
	if (sess == NULL)
	{
		return NULL;
	}

	sess->fd = -1;
	sess->max_msg_len = TCPCLV4_DEFAULT_MAX_MSG;
	sess->local_nodeid = strdup(local_node_id);
	if (sess->local_nodeid == NULL)
	{
		free(sess);
		return NULL;
	}

	sess->recv_buf_cap = 65536;
	sess->recv_buf = malloc(sess->recv_buf_cap);
	if (sess->recv_buf == NULL)
	{
		free(sess->local_nodeid);
		free(sess);
		return NULL;
	}

	return sess;
}

/*	Common post-connect establishment: contact headers, then
 *	TLS if negotiated.  RFC 9174 §4.3: TLS is used only when
 *	BOTH contact headers set CAN_TLS.  If we were given a TLS
 *	config but the peer cannot do TLS, the session is refused
 *	(policy: configured TLS is required TLS).			*/

static int establish(tcpclv4_tls_session_t *sess,
		const tcpclv4_tls_config_t *tls_config, int is_server,
		const char *expected_host)
{
	int	offer_tls = (tls_config != NULL);

	if (exchange_contact_headers(sess, offer_tls) < 0)
	{
		return -1;
	}

	if (!offer_tls)
	{
		return 0;	/* Plain TCP session */
	}

	if ((sess->peer_contact_flags & TCPCLV4_CONTACT_FLAG_CAN_TLS) == 0)
	{
		set_error(sess, "TLS configured but peer did not offer "
				"CAN_TLS");
		return -1;
	}

	return setup_tls(sess, tls_config, is_server, expected_host);
}

/*	==============================================================
 *	PUBLIC API: INITIALIZATION
 *	==============================================================
 */

void tcpclv4_tls_init(void)
{
	if (!g_openssl_initialized)
	{
		SSL_load_error_strings();
		SSL_library_init();
		OpenSSL_add_all_algorithms();
		g_openssl_initialized = 1;
	}
}

void tcpclv4_tls_cleanup(void)
{
	if (g_openssl_initialized)
	{
		EVP_cleanup();
		ERR_free_strings();
		g_openssl_initialized = 0;
	}
}

/*	==============================================================
 *	PUBLIC API: SESSION ESTABLISHMENT
 *	==============================================================
 */

tcpclv4_tls_session_t* tcpclv4_tls_connect(
	const char *host, int port,
	const char *local_node_id,
	const tcpclv4_tls_config_t *tls_config)
{
	tcpclv4_tls_session_t	*sess;
	int			fd;

	sess = session_alloc(local_node_id);
	if (sess == NULL)
	{
		return NULL;
	}

	fd = tcp_connect(host, port, sess->error_msg,
			 sizeof(sess->error_msg));
	if (fd < 0)
	{
		tcpclv4_tls_close(sess);
		return NULL;
	}

	sess->fd = fd;

	if (establish(sess, tls_config, 0, host) < 0)
	{
		tcpclv4_tls_close(sess);
		return NULL;
	}

	return sess;
}

tcpclv4_tls_session_t* tcpclv4_tls_accept(
	int client_fd,
	const char *local_node_id,
	const tcpclv4_tls_config_t *tls_config)
{
	tcpclv4_tls_session_t	*sess;
	int			flag = 1;

	sess = session_alloc(local_node_id);
	if (sess == NULL)
	{
		close(client_fd);	/* Accept always owns the fd. */
		return NULL;
	}

	sess->fd = client_fd;

	/* Disable Nagle's algorithm */
	setsockopt(sess->fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));

	if (establish(sess, tls_config, 1, NULL) < 0)
	{
		tcpclv4_tls_close(sess);
		return NULL;
	}

	return sess;
}

/*	==============================================================
 *	PUBLIC API: MESSAGE I/O
 *	==============================================================
 */

int tcpclv4_tls_send_msg(tcpclv4_tls_session_t *sess,
			  const uint8_t *msg_buf, size_t msg_len)
{
	int		ret;
	size_t		total_sent;
	const uint8_t	*p;

	if (sess == NULL || sess->fd < 0)
	{
		return -1;
	}

	total_sent = 0;
	p = msg_buf;

	while (total_sent < msg_len)
	{
		if (sess->tls_enabled)
		{
			ret = SSL_write(sess->ssl, p, msg_len - total_sent);
			if (ret <= 0)
			{
				int ssl_err = SSL_get_error(sess->ssl, ret);

				if (ssl_err == SSL_ERROR_WANT_WRITE)
				{
					continue;	/* Blocking socket:
							 * retry the write. */
				}

				set_error_ssl(sess, "SSL_write failed");
				return -1;
			}
		}
		else
		{
			ret = write(sess->fd, p, msg_len - total_sent);
			if (ret < 0)
			{
				if (errno == EINTR)
				{
					continue;
				}

				snprintf(sess->error_msg,
					 sizeof(sess->error_msg),
					 "write failed: %s", strerror(errno));
				return -1;
			}
		}

		total_sent += ret;
		p += ret;
	}

	return total_sent;
}

int tcpclv4_tls_write(tcpclv4_tls_session_t *sess,
		const uint8_t *buf, size_t len)
{
	int	n;

	if (sess == NULL || sess->fd < 0 || buf == NULL)
	{
		return -1;
	}

	if (len == 0)
	{
		return 0;
	}

	if (sess->tls_enabled)
	{
		n = SSL_write(sess->ssl, buf, len);
		if (n <= 0)
		{
			int	ssl_err = SSL_get_error(sess->ssl, n);

			if (ssl_err == SSL_ERROR_WANT_WRITE
			|| ssl_err == SSL_ERROR_WANT_READ)
			{
				return 0;	/* Would block */
			}

			set_error_ssl(sess, "SSL_write failed");
			return -1;
		}

		return n;
	}

	n = write(sess->fd, buf, len);
	if (n < 0)
	{
		if (errno == EAGAIN || errno == EWOULDBLOCK
		|| errno == EINTR)
		{
			return 0;	/* Would block */
		}

		snprintf(sess->error_msg, sizeof(sess->error_msg),
			 "write failed: %s", strerror(errno));
		return -1;
	}

	return n;
}

/*	Ensure the receive buffer can hold at least `needed` bytes.	*/

static int recv_buf_reserve(tcpclv4_tls_session_t *sess, size_t needed)
{
	if (needed <= sess->recv_buf_cap)
	{
		return 0;
	}

	/*	The receive buffer is preallocated to max_msg_len (see
	 *	tcpclv4_tls_set_max_msg_len), and frames larger than that
	 *	are refused during framing, so the data path never needs
	 *	to grow it.  A shortfall here is a bound/framing error,
	 *	not an allocation trigger.				*/

	set_error(sess, "Receive buffer request exceeds maximum "
			"message length");
	return -1;
}

/*	Extract the first complete message from the receive buffer,
 *	if one is present.
 *
 *	Returns 1 and copies the message out if complete,
 *	0 if more stream bytes are needed (never touches the socket),
 *	-1 on malformed stream or undersized caller buffer.		*/

static int extract_message(tcpclv4_tls_session_t *sess,
		uint8_t *buf, size_t buf_cap, size_t *out_len)
{
	size_t	frame;
	int	ret;

	if (sess->recv_buf_len == 0)
	{
		return 0;
	}

	ret = tcpclv4_msg_frame_len(sess->recv_buf, sess->recv_buf_len,
			sess->max_msg_len, &frame);
	if (ret < 0)
	{
		set_error(sess, "Malformed or oversized message from peer");
		return -1;
	}

	if (ret == 0)
	{
		/*	Incomplete.  If the total frame length is known,
		 *	pre-grow the buffer so subsequent reads fit.	*/

		if (frame > 0 && recv_buf_reserve(sess, frame) < 0)
		{
			return -1;
		}

		return 0;
	}

	if (frame > buf_cap)
	{
		set_error(sess, "Message too large for caller buffer");
		return -1;
	}

	memcpy(buf, sess->recv_buf, frame);
	*out_len = frame;

	/*	Retain any bytes of the next message.			*/

	sess->recv_buf_len -= frame;
	if (sess->recv_buf_len > 0)
	{
		memmove(sess->recv_buf, sess->recv_buf + frame,
				sess->recv_buf_len);
	}

	return 1;
}

int tcpclv4_tls_recv_msg(tcpclv4_tls_session_t *sess,
			  uint8_t *buf, size_t buf_cap, size_t *out_len)
{
	ssize_t	n;
	int	ret;

	if (sess == NULL || sess->fd < 0 || buf == NULL || out_len == NULL)
	{
		return -1;
	}

	/*	A complete message may already be buffered from a
	 *	previous read; callers should invoke this function
	 *	repeatedly until it returns 0.				*/

	ret = extract_message(sess, buf, buf_cap, out_len);
	if (ret != 0)
	{
		return ret;
	}

	/*	Need more stream bytes: read once from the socket.	*/

	if (sess->recv_buf_len == sess->recv_buf_cap)
	{
		set_error(sess, "Message exceeds receive buffer");
		return -1;
	}

	if (sess->tls_enabled)
	{
		n = SSL_read(sess->ssl,
			     sess->recv_buf + sess->recv_buf_len,
			     sess->recv_buf_cap - sess->recv_buf_len);
		if (n <= 0)
		{
			int ssl_err = SSL_get_error(sess->ssl, n);

			if (ssl_err == SSL_ERROR_WANT_READ)
			{
				return 0;	/* Would block */
			}

			if (ssl_err == SSL_ERROR_ZERO_RETURN)
			{
				set_error(sess, "Connection closed by peer");
				return -1;
			}

			set_error_ssl(sess, "SSL_read failed");
			return -1;
		}
	}
	else
	{
		n = read(sess->fd,
			 sess->recv_buf + sess->recv_buf_len,
			 sess->recv_buf_cap - sess->recv_buf_len);
		if (n < 0)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
			{
				return 0;	/* Would block */
			}

			snprintf(sess->error_msg, sizeof(sess->error_msg),
				 "read failed: %s", strerror(errno));
			return -1;
		}

		if (n == 0)
		{
			set_error(sess, "Connection closed by peer");
			return -1;
		}
	}

	sess->recv_buf_len += n;

	return extract_message(sess, buf, buf_cap, out_len);
}

/*	==============================================================
 *	PUBLIC API: SESSION CONTROL
 *	==============================================================
 */

int tcpclv4_tls_get_fd(tcpclv4_tls_session_t *sess)
{
	if (sess == NULL)
	{
		return -1;
	}

	return sess->fd;
}

int tcpclv4_tls_pending(tcpclv4_tls_session_t *sess)
{
	size_t	frame;

	if (sess == NULL)
	{
		return 0;
	}

	if (sess->recv_buf_len > 0
	&& tcpclv4_msg_frame_len(sess->recv_buf, sess->recv_buf_len,
			sess->max_msg_len, &frame) == 1)
	{
		return 1;
	}

	if (sess->tls_enabled && sess->ssl != NULL
	&& SSL_pending(sess->ssl) > 0)
	{
		return 1;
	}

	return 0;
}

int tcpclv4_tls_set_max_msg_len(tcpclv4_tls_session_t *sess,
		uint64_t max_msg_len)
{
	if (sess == NULL)
	{
		return -1;
	}

	sess->max_msg_len = max_msg_len;

	/*	Preallocate the receive buffer to the largest legal
	 *	message.  This is a one-time, per-session allocation at
	 *	establishment; thereafter the data path never grows it
	 *	(a frame exceeding max_msg_len is refused during
	 *	framing).						*/

	if (sess->recv_buf_cap < max_msg_len)
	{
		uint8_t	*buf = realloc(sess->recv_buf, max_msg_len);

		if (buf == NULL)
		{
			set_error(sess, "Receive buffer allocation failed");
			return -1;
		}

		sess->recv_buf = buf;
		sess->recv_buf_cap = max_msg_len;
	}

	return 0;
}

int tcpclv4_tls_set_nonblocking(tcpclv4_tls_session_t *sess, int nonblock)
{
	int	flags;

	if (sess == NULL || sess->fd < 0)
	{
		return -1;
	}

	flags = fcntl(sess->fd, F_GETFL, 0);
	if (flags < 0)
	{
		snprintf(sess->error_msg, sizeof(sess->error_msg),
			 "fcntl F_GETFL failed: %s", strerror(errno));
		return -1;
	}

	if (nonblock)
	{
		flags |= O_NONBLOCK;
	}
	else
	{
		flags &= ~O_NONBLOCK;
	}

	if (fcntl(sess->fd, F_SETFL, flags) < 0)
	{
		snprintf(sess->error_msg, sizeof(sess->error_msg),
			 "fcntl F_SETFL failed: %s", strerror(errno));
		return -1;
	}

	return 0;
}

const char* tcpclv4_tls_get_peer_nodeid(tcpclv4_tls_session_t *sess)
{
	if (sess == NULL)
	{
		return NULL;
	}

	return sess->peer_nodeid;
}

/*	Object identifier of the id-on-bundleEID otherName (RFC 9174
 *	§4.4.3), which carries a Node ID in a certificate's SANs.	*/

#define ID_ON_BUNDLE_EID_OID	"1.3.6.1.5.5.7.8.11"

/*	Return 1 if cert carries an id-on-bundleEID otherName SAN whose
 *	value equals node_id, else 0.					*/

static int cert_has_bundle_eid(X509 *cert, const char *node_id)
{
	GENERAL_NAMES	*sans;
	ASN1_OBJECT	*eid_oid;
	size_t		node_id_len;
	int		i;
	int		found = 0;

	sans = X509_get_ext_d2i(cert, NID_subject_alt_name, NULL, NULL);
	if (sans == NULL)
	{
		return 0;	/* No SANs at all. */
	}

	eid_oid = OBJ_txt2obj(ID_ON_BUNDLE_EID_OID, 1);
	node_id_len = strlen(node_id);

	for (i = 0; i < sk_GENERAL_NAME_num(sans) && !found; i++)
	{
		GENERAL_NAME	*gn = sk_GENERAL_NAME_value(sans, i);
		ASN1_TYPE	*val;
		ASN1_STRING	*str;

		if (gn->type != GEN_OTHERNAME
		|| OBJ_cmp(gn->d.otherName->type_id, eid_oid) != 0)
		{
			continue;
		}

		/*	RFC 9174 encodes the EID as an IA5String.	*/

		val = gn->d.otherName->value;
		if (val == NULL || val->type != V_ASN1_IA5STRING)
		{
			continue;
		}

		str = val->value.ia5string;
		if ((size_t) ASN1_STRING_length(str) == node_id_len
		&& memcmp(ASN1_STRING_get0_data(str), node_id,
				node_id_len) == 0)
		{
			found = 1;
		}
	}

	ASN1_OBJECT_free(eid_oid);
	GENERAL_NAMES_free(sans);
	return found;
}

int tcpclv4_tls_authenticate_nodeid(tcpclv4_tls_session_t *sess,
		const char *node_id, int required)
{
	X509	*cert;
	int	matched;

	if (sess == NULL || node_id == NULL)
	{
		return required ? -1 : 0;
	}

	if (!sess->tls_enabled)
	{
		return 0;	/* Plain TCP: no certificate to check. */
	}

#if OPENSSL_VERSION_NUMBER >= 0x30000000L
	cert = SSL_get1_peer_certificate(sess->ssl);
#else
	cert = SSL_get_peer_certificate(sess->ssl);
#endif
	if (cert == NULL)
	{
		if (required)
		{
			set_error(sess, "Peer presented no certificate for "
					"node ID authentication");
			return -1;
		}

		return 0;
	}

	matched = cert_has_bundle_eid(cert, node_id);
	X509_free(cert);

	if (matched)
	{
		return 0;
	}

	if (required)
	{
		snprintf(sess->error_msg, sizeof(sess->error_msg),
			"Peer node ID %.128s is not authenticated by its "
			"certificate", node_id);
		return -1;
	}

	return 0;	/* Verification not required: allow. */
}

int tcpclv4_tls_is_secure(tcpclv4_tls_session_t *sess)
{
	if (sess == NULL)
	{
		return 0;
	}

	return sess->tls_enabled;
}

/*	==============================================================
 *	PUBLIC API: SESSION TEARDOWN
 *	==============================================================
 */

void tcpclv4_tls_close(tcpclv4_tls_session_t *sess)
{
	if (sess == NULL)
	{
		return;
	}

	/* Shutdown TLS */
	if (sess->ssl != NULL)
	{
		SSL_shutdown(sess->ssl);
		SSL_free(sess->ssl);
		sess->ssl = NULL;
	}

	if (sess->ssl_ctx != NULL)
	{
		SSL_CTX_free(sess->ssl_ctx);
		sess->ssl_ctx = NULL;
	}

	/* Close socket */
	if (sess->fd >= 0)
	{
		close(sess->fd);
		sess->fd = -1;
	}

	/* Free memory */
	if (sess->peer_nodeid != NULL)
	{
		free(sess->peer_nodeid);
	}

	if (sess->local_nodeid != NULL)
	{
		free(sess->local_nodeid);
	}

	if (sess->recv_buf != NULL)
	{
		free(sess->recv_buf);
	}

	free(sess);
}

/*	==============================================================
 *	PUBLIC API: ERROR HANDLING
 *	==============================================================
 */

const char* tcpclv4_tls_get_error(tcpclv4_tls_session_t *sess)
{
	if (sess == NULL)
	{
		return NULL;
	}

	return sess->error_msg[0] ? sess->error_msg : NULL;
}

void tcpclv4_tls_clear_error(tcpclv4_tls_session_t *sess)
{
	if (sess != NULL)
	{
		sess->error_msg[0] = '\0';
	}
}
