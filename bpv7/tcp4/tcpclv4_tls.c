/*
	tcpclv4_tls.c:	TCPCLv4 TLS session implementation.

			Uses OpenSSL for TLS, handles contact header
			exchange, message framing over TLS/TCP.

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

/*	Session structure							*/

struct tcpclv4_tls_session_s
{
	int		fd;			/* Socket file descriptor */
	SSL		*ssl;			/* OpenSSL handle (NULL if no TLS) */
	SSL_CTX		*ssl_ctx;		/* SSL context (NULL if no TLS) */

	char		*peer_nodeid;		/* Peer node ID from contact header */
	char		*local_nodeid;		/* Our node ID */

	char		error_msg[512];		/* Last error message */
	int		tls_enabled;		/* 1 if TLS, 0 if plain TCP */

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

/*	Perform TLS handshake						*/

static int do_tls_handshake(tcpclv4_tls_session_t *sess)
{
	int	ret;

	ret = SSL_connect(sess->ssl);
	if (ret != 1)
	{
		set_error_ssl(sess, "TLS handshake failed");
		return -1;
	}

	return 0;
}

/*	Exchange contact headers (after TLS handshake or on plain TCP)	*/

static int exchange_contact_headers(tcpclv4_tls_session_t *sess)
{
	uint8_t				buf[256];
	size_t				len;
	tcpclv4_contact_header_t	hdr;
	int				ret;
	ssize_t				n;

	/* Send our contact header */
	hdr.magic = TCPCLV4_CONTACT_MAGIC;
	hdr.flags = sess->tls_enabled ? TCPCLV4_CONTACT_FLAG_CAN_TLS : 0;

	ret = tcpclv4_encode_contact_header(&hdr, buf, sizeof(buf), &len);
	if (ret < 0)
	{
		set_error(sess, "Failed to encode contact header");
		return -1;
	}

	if (sess->tls_enabled)
	{
		ret = SSL_write(sess->ssl, buf, len);
	}
	else
	{
		ret = write(sess->fd, buf, len);
	}

	if (ret != (int)len)
	{
		set_error(sess, "Failed to send contact header");
		return -1;
	}

	/* Receive peer's contact header */
	if (sess->tls_enabled)
	{
		n = SSL_read(sess->ssl, buf, 4);
	}
	else
	{
		n = read(sess->fd, buf, 4);
	}

	if (n != 4)
	{
		set_error(sess, "Failed to receive contact header");
		return -1;
	}

	ret = tcpclv4_decode_contact_header(buf, n, &hdr);
	if (ret < 0)
	{
		set_error(sess, "Invalid contact header from peer");
		return -1;
	}

	/* Note: peer_nodeid comes from SESS_INIT, not contact header */

	return 0;
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

	if (!g_openssl_initialized)
	{
		tcpclv4_tls_init();
	}

	/* Allocate session */
	sess = calloc(1, sizeof(*sess));
	if (sess == NULL)
	{
		return NULL;
	}

	sess->fd = -1;
	sess->local_nodeid = strdup(local_node_id);
	sess->tls_enabled = (tls_config != NULL);

	/* Allocate receive buffer */
	sess->recv_buf_cap = 65536;
	sess->recv_buf = malloc(sess->recv_buf_cap);
	if (sess->recv_buf == NULL)
	{
		free(sess->local_nodeid);
		free(sess);
		return NULL;
	}

	/* Connect TCP socket */
	fd = tcp_connect(host, port, sess->error_msg,
			 sizeof(sess->error_msg));
	if (fd < 0)
	{
		free(sess->recv_buf);
		free(sess->local_nodeid);
		free(sess);
		return NULL;
	}

	sess->fd = fd;

	/* TLS setup */
	if (sess->tls_enabled)
	{
		sess->ssl_ctx = SSL_CTX_new(TLS_client_method());
		if (sess->ssl_ctx == NULL)
		{
			set_error_ssl(sess, "SSL_CTX_new failed");
			tcpclv4_tls_close(sess);
			return NULL;
		}

		/* Load certificates */
		if (tls_config->tls_cert_file)
		{
			if (SSL_CTX_use_certificate_file(sess->ssl_ctx,
					tls_config->tls_cert_file,
					SSL_FILETYPE_PEM) <= 0)
			{
				set_error_ssl(sess, "Failed to load cert");
				tcpclv4_tls_close(sess);
				return NULL;
			}
		}

		if (tls_config->tls_key_file)
		{
			if (SSL_CTX_use_PrivateKey_file(sess->ssl_ctx,
					tls_config->tls_key_file,
					SSL_FILETYPE_PEM) <= 0)
			{
				set_error_ssl(sess, "Failed to load key");
				tcpclv4_tls_close(sess);
				return NULL;
			}
		}

		if (tls_config->tls_ca_file)
		{
			if (SSL_CTX_load_verify_locations(sess->ssl_ctx,
					tls_config->tls_ca_file, NULL) <= 0)
			{
				set_error_ssl(sess, "Failed to load CA");
				tcpclv4_tls_close(sess);
				return NULL;
			}
		}

		if (tls_config->tls_verify_peer)
		{
			SSL_CTX_set_verify(sess->ssl_ctx,
					   SSL_VERIFY_PEER, NULL);
		}

		/* Create SSL object */
		sess->ssl = SSL_new(sess->ssl_ctx);
		if (sess->ssl == NULL)
		{
			set_error_ssl(sess, "SSL_new failed");
			tcpclv4_tls_close(sess);
			return NULL;
		}

		SSL_set_fd(sess->ssl, sess->fd);

		/* TLS handshake */
		if (do_tls_handshake(sess) < 0)
		{
			tcpclv4_tls_close(sess);
			return NULL;
		}
	}

	/* Exchange contact headers */
	if (exchange_contact_headers(sess) < 0)
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

	if (!g_openssl_initialized)
	{
		tcpclv4_tls_init();
	}

	/* Allocate session */
	sess = calloc(1, sizeof(*sess));
	if (sess == NULL)
	{
		return NULL;
	}

	sess->fd = client_fd;
	sess->local_nodeid = strdup(local_node_id);
	sess->tls_enabled = (tls_config != NULL);

	/* Allocate receive buffer */
	sess->recv_buf_cap = 65536;
	sess->recv_buf = malloc(sess->recv_buf_cap);
	if (sess->recv_buf == NULL)
	{
		free(sess->local_nodeid);
		free(sess);
		return NULL;
	}

	/* Disable Nagle's algorithm */
	int flag = 1;
	setsockopt(sess->fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));

	/* TLS setup (server mode) */
	if (sess->tls_enabled)
	{
		sess->ssl_ctx = SSL_CTX_new(TLS_server_method());
		if (sess->ssl_ctx == NULL)
		{
			set_error_ssl(sess, "SSL_CTX_new failed");
			tcpclv4_tls_close(sess);
			return NULL;
		}

		/* Load server certificates */
		if (tls_config->tls_cert_file)
		{
			if (SSL_CTX_use_certificate_file(sess->ssl_ctx,
					tls_config->tls_cert_file,
					SSL_FILETYPE_PEM) <= 0)
			{
				set_error_ssl(sess, "Failed to load cert");
				tcpclv4_tls_close(sess);
				return NULL;
			}
		}

		if (tls_config->tls_key_file)
		{
			if (SSL_CTX_use_PrivateKey_file(sess->ssl_ctx,
					tls_config->tls_key_file,
					SSL_FILETYPE_PEM) <= 0)
			{
				set_error_ssl(sess, "Failed to load key");
				tcpclv4_tls_close(sess);
				return NULL;
			}
		}

		if (tls_config->tls_ca_file)
		{
			if (SSL_CTX_load_verify_locations(sess->ssl_ctx,
					tls_config->tls_ca_file, NULL) <= 0)
			{
				set_error_ssl(sess, "Failed to load CA");
				tcpclv4_tls_close(sess);
				return NULL;
			}
		}

		if (tls_config->tls_verify_peer)
		{
			SSL_CTX_set_verify(sess->ssl_ctx,
					   SSL_VERIFY_PEER |
					   SSL_VERIFY_FAIL_IF_NO_PEER_CERT,
					   NULL);
		}

		/* Create SSL object */
		sess->ssl = SSL_new(sess->ssl_ctx);
		if (sess->ssl == NULL)
		{
			set_error_ssl(sess, "SSL_new failed");
			tcpclv4_tls_close(sess);
			return NULL;
		}

		SSL_set_fd(sess->ssl, sess->fd);

		/* TLS handshake (server side) */
		if (SSL_accept(sess->ssl) != 1)
		{
			set_error_ssl(sess, "TLS accept failed");
			tcpclv4_tls_close(sess);
			return NULL;
		}
	}

	/* Exchange contact headers */
	if (exchange_contact_headers(sess) < 0)
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
					return 0;	/* Would block */
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
				if (errno == EAGAIN || errno == EWOULDBLOCK)
				{
					return 0;	/* Would block */
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

int tcpclv4_tls_recv_msg(tcpclv4_tls_session_t *sess,
			  uint8_t *buf, size_t buf_cap, size_t *out_len)
{
	ssize_t	n;

	if (sess == NULL || sess->fd < 0)
	{
		return -1;
	}

	/* Try to read data into receive buffer */
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

	/* Simple framing: check if we have at least the message type byte */
	if (sess->recv_buf_len < 1)
	{
		return 0;	/* Need more data */
	}

	/* For simplicity, we extract one complete message.
	 * Real implementation would parse message length from headers. */
	if (sess->recv_buf_len > buf_cap)
	{
		set_error(sess, "Message too large for buffer");
		return -1;
	}

	/* Copy message to output buffer */
	memcpy(buf, sess->recv_buf, sess->recv_buf_len);
	*out_len = sess->recv_buf_len;

	/* Clear receive buffer */
	sess->recv_buf_len = 0;

	return 1;	/* Success */
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
