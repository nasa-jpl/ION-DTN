/*
	mqttcla.h:	common definitions for MQTT convergence layer
			adapter modules.

	Author: ION Development Team

	Copyright (c) 2026, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
								*/

#ifndef MQTTCLA_H
#define MQTTCLA_H

#include "bpP.h"
#include <pthread.h>
#include <MQTTClient.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MQTTCLA_BUFSZ		(256 * 1024)
#define MQTT_TOPIC_PREFIX	"ion/bundles/"
#define MQTT_DEFAULT_PORT	1883
#define MQTT_DEFAULT_TLS_PORT	8883
#define MQTT_KEEPALIVE_SEC	60
#define MQTT_QOS		1
#define MQTT_MAX_RECONNECT_PAUSE	30

#define MQTT_MAX_HOST_LEN	256
#define MQTT_MAX_TOPIC_LEN	512
#define MQTT_MAX_PATH_LEN	1024
#define MQTT_MAX_CRED_LEN	256

/*
 * Configuration parsed from command-line arguments.
 */
typedef struct
{
	char	username[MQTT_MAX_CRED_LEN];
	char	password[MQTT_MAX_CRED_LEN];
	int	useTls;
	char	caFile[MQTT_MAX_PATH_LEN];
	char	caPath[MQTT_MAX_PATH_LEN];
	char	keyStore[MQTT_MAX_PATH_LEN];
	char	keyPassword[MQTT_MAX_CRED_LEN];
} MqttClaConfig;

/*
 * Parse a duct name of the form "host:port/suffix" into components.
 * Returns 0 on success, -1 on error.
 */
static int	parseMqttDuctName(const char *ductName, char *host,
			int *port, char *topicSuffix)
{
	const char	*colon;
	const char	*slash;
	int		hostLen;

	if (ductName == NULL || host == NULL || port == NULL
			|| topicSuffix == NULL)
	{
		return -1;
	}

	colon = strchr(ductName, ':');
	if (colon == NULL)
	{
		return -1;
	}

	hostLen = colon - ductName;
	if (hostLen <= 0 || hostLen >= MQTT_MAX_HOST_LEN)
	{
		return -1;
	}

	memcpy(host, ductName, hostLen);
	host[hostLen] = '\0';

	slash = strchr(colon + 1, '/');
	if (slash == NULL)
	{
		return -1;
	}

	if (sscanf(colon + 1, "%d", port) != 1 || *port <= 0
			|| *port > 65535)
	{
		return -1;
	}

	if (strlen(slash + 1) == 0
			|| strlen(slash + 1) >= MQTT_MAX_TOPIC_LEN)
	{
		return -1;
	}

	istrcpy(topicSuffix, slash + 1, MQTT_MAX_TOPIC_LEN);
	return 0;
}

/*
 * Build an MQTT topic string from a suffix.
 * Result is "ion/bundles/<suffix>".
 * Returns 0 on success, -1 on error.
 */
static int	buildMqttTopic(const char *suffix, char *buf, int bufLen)
{
	int	needed;

	if (suffix == NULL || buf == NULL)
	{
		return -1;
	}

	needed = strlen(MQTT_TOPIC_PREFIX) + strlen(suffix) + 1;
	if (needed > bufLen)
	{
		return -1;
	}

	isprintf(buf, bufLen, "%s%s", MQTT_TOPIC_PREFIX, suffix);
	return 0;
}

/*
 * Parse optional command-line arguments for authentication and TLS.
 *
 * argv[0] is the program name, argv[1] is the duct name (already
 * consumed by the caller).  This function scans argv[2..argc-1]
 * for the following flags:
 *
 *   -u <username>       MQTT username
 *   -p <password>       MQTT password
 *   -t                  Enable TLS (use ssl:// URI scheme)
 *   -c <cafile>         CA certificate file (PEM)
 *   -C <capath>         CA certificate directory
 *   -k <keystore>       Client certificate + key file (PEM)
 *   -K <keypassword>    Private key password
 *
 * Returns 0 on success, -1 on error.
 */
static int	parseMqttArgs(int argc, char *argv[], MqttClaConfig *cfg)
{
	int	i;

	memset(cfg, 0, sizeof(MqttClaConfig));

	for (i = 2; i < argc; i++)
	{
		if (strcmp(argv[i], "-u") == 0 && i + 1 < argc)
		{
			istrcpy(cfg->username, argv[++i],
					MQTT_MAX_CRED_LEN);
		}
		else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc)
		{
			istrcpy(cfg->password, argv[++i],
					MQTT_MAX_CRED_LEN);
		}
		else if (strcmp(argv[i], "-t") == 0)
		{
			cfg->useTls = 1;
		}
		else if (strcmp(argv[i], "-c") == 0 && i + 1 < argc)
		{
			istrcpy(cfg->caFile, argv[++i],
					MQTT_MAX_PATH_LEN);
			cfg->useTls = 1;
		}
		else if (strcmp(argv[i], "-C") == 0 && i + 1 < argc)
		{
			istrcpy(cfg->caPath, argv[++i],
					MQTT_MAX_PATH_LEN);
			cfg->useTls = 1;
		}
		else if (strcmp(argv[i], "-k") == 0 && i + 1 < argc)
		{
			istrcpy(cfg->keyStore, argv[++i],
					MQTT_MAX_PATH_LEN);
			cfg->useTls = 1;
		}
		else if (strcmp(argv[i], "-K") == 0 && i + 1 < argc)
		{
			istrcpy(cfg->keyPassword, argv[++i],
					MQTT_MAX_CRED_LEN);
		}
		else
		{
			putErrmsg("mqttcla: unknown argument.", argv[i]);
			return -1;
		}
	}

	return 0;
}

/*
 * Build the server URI string.
 * Uses "ssl://" when TLS is enabled, "tcp://" otherwise.
 */
static void	buildMqttServerURI(const char *host, int port,
			int useTls, char *buf, int bufLen)
{
	isprintf(buf, bufLen, "%s://%s:%d",
			useTls ? "ssl" : "tcp", host, port);
}

/*
 * Apply authentication and TLS configuration to connection options.
 * The sslOpts structure must remain valid for the lifetime of
 * connOpts (i.e., both must be on the caller's stack or heap).
 */
static void	applyMqttConfig(const MqttClaConfig *cfg,
			MQTTClient_connectOptions *connOpts,
			MQTTClient_SSLOptions *sslOpts)
{
	if (cfg->username[0] != '\0')
	{
		connOpts->username = cfg->username;
	}

	if (cfg->password[0] != '\0')
	{
		connOpts->password = cfg->password;
	}

	if (cfg->useTls)
	{
		MQTTClient_SSLOptions initSsl =
				MQTTClient_SSLOptions_initializer;

		*sslOpts = initSsl;

		if (cfg->caFile[0] != '\0')
		{
			sslOpts->trustStore = cfg->caFile;
		}

		if (cfg->caPath[0] != '\0')
		{
			sslOpts->CApath = cfg->caPath;
		}

		if (cfg->keyStore[0] != '\0')
		{
			sslOpts->keyStore = cfg->keyStore;
		}

		if (cfg->keyPassword[0] != '\0')
		{
			sslOpts->privateKeyPassword = cfg->keyPassword;
		}

		sslOpts->enableServerCertAuth = 1;
		connOpts->ssl = sslOpts;
	}
}

#ifdef __cplusplus
}
#endif

#endif /* MQTTCLA_H */
