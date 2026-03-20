/*
	mqttcli.c:	BP MQTT-based convergence-layer input
			daemon.  Subscribes to an MQTT broker topic
			and receives bundles for delivery to the
			local node.

	Author: ION Development Team

	Copyright (c) 2026, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
								*/
#include "mqttcla.h"

typedef struct
{
	VInduct		*vduct;
	int		running;
	MQTTClient	client;
	char		topic[MQTT_MAX_TOPIC_LEN];
	MQTTClient_connectOptions *connOpts;
} ReceiverThreadParms;

static void	interruptThread(int signum)
{
	(void)signum;

	isignal(SIGTERM, interruptThread);
	ionKillMainThread("mqttcli");
}

static void	*receiveBundles(void *parm)
{
	ReceiverThreadParms	*rtp = (ReceiverThreadParms *) parm;
	char			*procName = "mqttcli";
	AcqWorkArea		*work;
	char			*topicName = NULL;
	int			topicLen;
	MQTTClient_message	*msg = NULL;
	int			rc;
	int			pause = 0;

	snooze(1);	/*	Let main thread become interruptible.	*/
	work = bpGetAcqArea(rtp->vduct);
	if (work == NULL)
	{
		putErrmsg("mqttcli can't get acquisition work area.", NULL);
		ionKillMainThread(procName);
		return NULL;
	}

	while (rtp->running)
	{
		/*	Check connection and reconnect if needed.	*/

		if (!MQTTClient_isConnected(rtp->client))
		{
			rc = MQTTClient_connect(rtp->client, rtp->connOpts);
			if (rc != MQTTCLIENT_SUCCESS)
			{
				if (pause == 0)
				{
					pause = 1;
				}
				else
				{
					pause <<= 1;
					if (pause > MQTT_MAX_RECONNECT_PAUSE)
					{
						pause =
						    MQTT_MAX_RECONNECT_PAUSE;
					}
				}

				snooze(pause);
				continue;
			}

			/*	Resubscribe after reconnect.		*/

			rc = MQTTClient_subscribe(rtp->client, rtp->topic,
					MQTT_QOS);
			if (rc != MQTTCLIENT_SUCCESS)
			{
				writeMemo("[?] mqttcli: resubscribe failed.");
				snooze(1);
				continue;
			}

			pause = 0;
		}

		/*	Wait for next message with 1-second timeout.	*/

		rc = MQTTClient_receive(rtp->client, &topicName, &topicLen,
				&msg, 1000);
		if (rc != MQTTCLIENT_SUCCESS)
		{
			if (rtp->running)
			{
				writeMemo("[?] mqttcli: receive error.");
			}

			continue;
		}

		if (msg == NULL)	/*	Timeout, no message.	*/
		{
			continue;
		}

		/*	Got a bundle — acquire it.			*/

		if (msg->payloadlen > 0)
		{
			if (bpBeginAcq(work, 0, NULL) < 0
			|| bpContinueAcq(work, (char *) msg->payload,
					msg->payloadlen, 0, 0) < 0
			|| bpEndAcq(work) < 0)
			{
				putErrmsg("Can't acquire bundle.", NULL);
				MQTTClient_freeMessage(&msg);
				MQTTClient_free(topicName);
				ionKillMainThread(procName);
				rtp->running = 0;
				continue;
			}
		}

		MQTTClient_freeMessage(&msg);
		MQTTClient_free(topicName);
		topicName = NULL;
		msg = NULL;

		sm_TaskYield();
	}

	writeErrmsgMemos();
	writeMemo("[i] mqttcli receiver thread has ended.");
	bpReleaseAcqArea(work);
	return NULL;
}

/*	*	*	Main thread functions	*	*	*	*/

#if defined (ION_LWT)
int	mqttcli(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char	*ductName = (char *) a1;
	int	largc = 2;
	char	*largv[2];

	largv[0] = "mqttcli";
	largv[1] = ductName;
#else
int	main(int argc, char *argv[])
{
	char	*ductName = (argc > 1 ? argv[1] : NULL);
	int	largc = argc;
	char	**largv = argv;
#endif
	VInduct			*vduct;
	PsmAddress		vductElt;
	Sdr			sdr;
	Induct			duct;
	ClProtocol		protocol;
	ReceiverThreadParms	rtp;
	pthread_t		receiverThread;
	char			hostName[MQTT_MAX_HOST_LEN];
	int			portNbr;
	char			topicSuffix[MQTT_MAX_TOPIC_LEN];
	char			serverURI[MQTT_MAX_HOST_LEN + 32];
	char			clientId[256];
	MQTTClient		client = NULL;
	MQTTClient_connectOptions connOpts =
					MQTTClient_connectOptions_initializer;
	MQTTClient_SSLOptions	sslOpts;
	MqttClaConfig		mqttCfg;
	int			rc;

	if (ductName == NULL)
	{
		PUTS("Usage: mqttcli <host:port/suffix> "
				"[-u user] [-p pass] [-t] "
				"[-c cafile] [-C capath] "
				"[-k keystore] [-K keypass]");
		return 0;
	}

	if (parseMqttArgs(largc, largv, &mqttCfg) < 0)
	{
		putErrmsg("mqttcli: invalid arguments.", NULL);
		return -1;
	}

	if (parseMqttDuctName(ductName, hostName, &portNbr,
			topicSuffix) < 0)
	{
		putErrmsg("mqttcli: invalid duct name.", ductName);
		return -1;
	}

	if (bpAttach() < 0)
	{
		putErrmsg("mqttcli can't attach to BP.", NULL);
		return -1;
	}

	findInduct("mqtt", ductName, &vduct, &vductElt);
	if (vductElt == 0)
	{
		putErrmsg("No such mqtt duct.", ductName);
		return -1;
	}

	if (vduct->cliPid != ERROR && vduct->cliPid != sm_TaskIdSelf())
	{
		putErrmsg("CLI task is already started for this duct.",
				itoa(vduct->cliPid));
		return -1;
	}

	/*	All command-line arguments are now validated.		*/

	sdr = getIonsdr();
	CHKZERO(sdr_begin_xn(sdr));
	sdr_read(sdr, (char *) &duct, sdr_list_data(sdr, vduct->inductElt),
			sizeof(Induct));
	sdr_read(sdr, (char *) &protocol, duct.protocol,
			sizeof(ClProtocol));
	sdr_exit_xn(sdr);

	/*	Connect to MQTT broker.					*/

	buildMqttServerURI(hostName, portNbr, mqttCfg.useTls,
			serverURI, sizeof(serverURI));

	/*	Use a deterministic client ID for persistent session
	 *	recovery: include the topic suffix so the broker
	 *	can match the session even across restarts.		*/

	isprintf(clientId, sizeof(clientId), "ion-cli-%s",
			topicSuffix);

	rc = MQTTClient_create(&client, serverURI, clientId,
			MQTTCLIENT_PERSISTENCE_NONE, NULL);
	if (rc != MQTTCLIENT_SUCCESS)
	{
		putErrmsg("mqttcli: MQTTClient_create failed.", serverURI);
		return -1;
	}

	connOpts.keepAliveInterval = MQTT_KEEPALIVE_SEC;
	connOpts.cleansession = 0;
	connOpts.connectTimeout = 10;
	applyMqttConfig(&mqttCfg, &connOpts, &sslOpts);

	rc = MQTTClient_connect(client, &connOpts);
	if (rc != MQTTCLIENT_SUCCESS)
	{
		putErrmsg("mqttcli: can't connect to MQTT broker.", serverURI);
		writeMemo("[?] mqttcli will attempt reconnect in receiver "
				"thread.");
	}
	else
	{
		/*	Subscribe to our topic.				*/

		char	topicBuf[MQTT_MAX_TOPIC_LEN];

		if (buildMqttTopic(topicSuffix, topicBuf,
				sizeof(topicBuf)) < 0)
		{
			putErrmsg("mqttcli: can't build topic.",
					topicSuffix);
			MQTTClient_destroy(&client);
			return -1;
		}

		rc = MQTTClient_subscribe(client, topicBuf, MQTT_QOS);
		if (rc != MQTTCLIENT_SUCCESS)
		{
			putErrmsg("mqttcli: subscribe failed.", topicBuf);
			MQTTClient_disconnect(client, 5000);
			MQTTClient_destroy(&client);
			return -1;
		}
	}

	/*	Set up receiver thread parameters.			*/

	memset(&rtp, 0, sizeof(rtp));
	rtp.vduct = vduct;
	rtp.client = client;
	rtp.connOpts = &connOpts;

	if (buildMqttTopic(topicSuffix, rtp.topic,
			sizeof(rtp.topic)) < 0)
	{
		putErrmsg("mqttcli: can't build topic.", topicSuffix);
		MQTTClient_destroy(&client);
		return -1;
	}

	/*	Set up signal handling; SIGTERM is shutdown signal.	*/

	ionNoteMainThread("mqttcli");
	isignal(SIGTERM, interruptThread);

	/*	Start the receiver thread.				*/

	rtp.running = 1;
	if (pthread_begin(&receiverThread, NULL, receiveBundles, &rtp))
	{
		putSysErrmsg("mqttcli can't create receiver thread", NULL);
		MQTTClient_disconnect(client, 5000);
		MQTTClient_destroy(&client);
		return -1;
	}

	{
		char	txt[1024];

		isprintf(txt, sizeof(txt),
				"[i] mqttcli is running, duct '%s', "
				"broker '%s', topic '%s'.",
				ductName, serverURI, rtp.topic);
		writeMemo(txt);
	}

	/*	Wait for shutdown.					*/

	ionPauseMainThread(-1);

	/*	Time to shut down.					*/

	rtp.running = 0;
	pthread_join(receiverThread, NULL);

	if (client != NULL)
	{
		if (MQTTClient_isConnected(client))
		{
			MQTTClient_disconnect(client, 5000);
		}

		MQTTClient_destroy(&client);
	}

	writeErrmsgMemos();
	writeMemo("[i] mqttcli duct has ended.");
	ionDetach();
	return 0;
}
