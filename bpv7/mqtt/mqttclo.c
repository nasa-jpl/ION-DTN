/*
	mqttclo.c:	BP MQTT-based convergence-layer output
			daemon.  Publishes bundles to an MQTT broker
			for delivery to remote nodes.

	Author: ION Development Team

	Copyright (c) 2026, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
								*/
#include "mqttcla.h"

static sm_SemId		mqttcloSemaphore(sm_SemId *semid)
{
	static sm_SemId	semaphore = -1;

	if (semid)
	{
		semaphore = *semid;
	}

	return semaphore;
}

static void	shutDownClo(int signum)
{
	(void)signum;

	sm_SemEnd(mqttcloSemaphore(NULL));
}

/*	*	*	Main thread functions	*	*	*	*/

#if defined (ION_LWT)
int	mqttclo(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char	*ductName = (char *) a1;
	int	largc = 2;
	char	*largv[2];

	largv[0] = "mqttclo";
	largv[1] = ductName;
#else
int	main(int argc, char *argv[])
{
	char	*ductName = (argc > 1 ? argv[1] : NULL);
	int	largc = argc;
	char	**largv = argv;
#endif
	unsigned char		*buffer;
	VOutduct		*vduct;
	PsmAddress		vductElt;
	Sdr			sdr;
	Outduct			outduct;
	Object			planDuctList;
	Object			planObj = 0;
	BpPlan			plan;
	Object			bundleZco;
	BpAncillaryData		ancillaryData;
	unsigned int		bundleLength;
	int			bytesSent;
	char			hostName[MQTT_MAX_HOST_LEN];
	int			portNbr;
	char			topicSuffix[MQTT_MAX_TOPIC_LEN];
	char			topic[MQTT_MAX_TOPIC_LEN];
	char			serverURI[MQTT_MAX_HOST_LEN + 32];
	char			clientId[256];
	MQTTClient		client = NULL;
	MQTTClient_connectOptions connOpts =
					MQTTClient_connectOptions_initializer;
	MQTTClient_SSLOptions	sslOpts;
	MQTTClient_message	pubmsg = MQTTClient_message_initializer;
	MQTTClient_deliveryToken token;
	MqttClaConfig		mqttCfg;
	int			rc;
	int			pause = 0;
	ZcoReader		reader;
	int			bytesToSend;

	if (ductName == NULL)
	{
		PUTS("Usage: mqttclo <host:port/suffix> "
				"[-u user] [-p pass] [-t] "
				"[-c cafile] [-C capath] "
				"[-k keystore] [-K keypass]");
		return 0;
	}

	if (parseMqttArgs(largc, largv, &mqttCfg) < 0)
	{
		putErrmsg("mqttclo: invalid arguments.", NULL);
		return -1;
	}

	if (parseMqttDuctName(ductName, hostName, &portNbr,
			topicSuffix) < 0)
	{
		putErrmsg("mqttclo: invalid duct name.", ductName);
		return -1;
	}

	if (buildMqttTopic(topicSuffix, topic, sizeof(topic)) < 0)
	{
		putErrmsg("mqttclo: can't build topic.", topicSuffix);
		return -1;
	}

	if (bpAttach() < 0)
	{
		putErrmsg("mqttclo can't attach to BP.", NULL);
		return -1;
	}

	buffer = MTAKE(MQTTCLA_BUFSZ);
	if (buffer == NULL)
	{
		putErrmsg("No memory for MQTT buffer in mqttclo.", NULL);
		return -1;
	}

	findOutduct("mqtt", ductName, &vduct, &vductElt);
	if (vductElt == 0)
	{
		putErrmsg("No such mqtt duct.", ductName);
		MRELEASE(buffer);
		return -1;
	}

	if (vduct->cloPid != ERROR && vduct->cloPid != sm_TaskIdSelf())
	{
		putErrmsg("CLO task is already started for this duct.",
				itoa(vduct->cloPid));
		MRELEASE(buffer);
		return -1;
	}

	/*	All command-line arguments are now validated.		*/

	sdr = getIonsdr();
	CHKZERO(sdr_begin_xn(sdr));
	sdr_read(sdr, (char *) &outduct, sdr_list_data(sdr,
			vduct->outductElt), sizeof(Outduct));
	if (outduct.planDuctListElt)
	{
		planDuctList = sdr_list_list(sdr, outduct.planDuctListElt);
		planObj = sdr_list_user_data(sdr, planDuctList);
		if (planObj)
		{
			sdr_read(sdr, (char *) &plan, planObj,
					sizeof(BpPlan));
		}
	}

	sdr_exit_xn(sdr);

	/*	Connect to MQTT broker.					*/

	buildMqttServerURI(hostName, portNbr, mqttCfg.useTls,
			serverURI, sizeof(serverURI));
	isprintf(clientId, sizeof(clientId), "ion-clo-%d-%s",
			(int) sm_TaskIdSelf(), topicSuffix);

	rc = MQTTClient_create(&client, serverURI, clientId,
			MQTTCLIENT_PERSISTENCE_NONE, NULL);
	if (rc != MQTTCLIENT_SUCCESS)
	{
		putErrmsg("mqttclo: MQTTClient_create failed.", serverURI);
		MRELEASE(buffer);
		return -1;
	}

	connOpts.keepAliveInterval = MQTT_KEEPALIVE_SEC;
	connOpts.cleansession = 0;
	connOpts.connectTimeout = 10;
	applyMqttConfig(&mqttCfg, &connOpts, &sslOpts);

	rc = MQTTClient_connect(client, &connOpts);
	if (rc != MQTTCLIENT_SUCCESS)
	{
		putErrmsg("mqttclo: can't connect to MQTT broker.", serverURI);
		writeMemo("[?] mqttclo will attempt reconnect in main loop.");
	}

	/*	Set up signal handling.  SIGTERM is shutdown signal.	*/

	oK(mqttcloSemaphore(&(vduct->semaphore)));
	isignal(SIGTERM, shutDownClo);

	/*	Can now begin transmitting to MQTT broker.		*/

	{
		char	memoBuf[1024];

		isprintf(memoBuf, sizeof(memoBuf),
				"[i] mqttclo is running, duct '%s', "
				"broker '%s', topic '%s'.",
				ductName, serverURI, topic);
		writeMemo(memoBuf);
	}

	while (!(sm_SemEnded(vduct->semaphore)))
	{
		if (bpDequeue(vduct, &bundleZco, &ancillaryData, -1) < 0)
		{
			putErrmsg("Can't dequeue bundle.", NULL);
			break;
		}

		if (bundleZco == 0)		/*	Outduct closed.	*/
		{
			writeMemo("[i] mqttclo outduct closed.");
			sm_SemEnd(mqttcloSemaphore(NULL));
			continue;
		}

		if (bundleZco == 1)	/*	Got a corrupt bundle.	*/
		{
			continue;
		}

		CHKZERO(sdr_begin_xn(sdr));
		bundleLength = zco_length(sdr, bundleZco);
		sdr_exit_xn(sdr);

		if (bundleLength > MQTTCLA_BUFSZ)
		{
			putErrmsg("Bundle too big for MQTT CLA buffer.",
					itoa(bundleLength));
			if (bpHandleXmitFailure(bundleZco) < 0)
			{
				putErrmsg("Can't handle xmit failure.", NULL);
				break;
			}

			continue;
		}

		/*	Extract bundle bytes from ZCO.			*/

		zco_start_transmitting(bundleZco, &reader);
		zco_track_file_offset(&reader);
		CHKZERO(sdr_begin_xn(sdr));
		bytesToSend = zco_transmit(sdr, &reader, MQTTCLA_BUFSZ,
				(char *) buffer);
		if (sdr_end_xn(sdr) < 0 || bytesToSend < 0)
		{
			putErrmsg("Can't issue from ZCO.", NULL);
			break;
		}

		/*	Publish bundle to MQTT broker.			*/

		if (!MQTTClient_isConnected(client))
		{
			rc = MQTTClient_connect(client, &connOpts);
			if (rc != MQTTCLIENT_SUCCESS)
			{
				writeMemo("[?] mqttclo: reconnect failed.");
				if (bpHandleXmitFailure(bundleZco) < 0)
				{
					putErrmsg("Can't handle xmit failure.",
							NULL);
					break;
				}

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

			pause = 0;
		}

		pubmsg.payload = buffer;
		pubmsg.payloadlen = bytesToSend;
		pubmsg.qos = MQTT_QOS;
		pubmsg.retained = 0;

		rc = MQTTClient_publishMessage(client, topic, &pubmsg,
				&token);
		if (rc != MQTTCLIENT_SUCCESS)
		{
			writeMemo("[?] mqttclo: publish failed.");
			bytesSent = -1;
		}
		else
		{
			rc = MQTTClient_waitForCompletion(client, token,
					10000);
			if (rc != MQTTCLIENT_SUCCESS)
			{
				writeMemo("[?] mqttclo: publish delivery "
						"timed out.");
				bytesSent = -1;
			}
			else
			{
				bytesSent = bytesToSend;
			}
		}

		if (bytesSent < 0)
		{
			if (bpHandleXmitFailure(bundleZco) < 0)
			{
				putErrmsg("Can't handle xmit failure.", NULL);
				break;
			}

			if (pause == 0)
			{
				pause = 1;
			}
			else
			{
				pause <<= 1;
				if (pause > MQTT_MAX_RECONNECT_PAUSE)
				{
					pause = MQTT_MAX_RECONNECT_PAUSE;
				}
			}

			snooze(pause);
		}
		else
		{
			if (bpHandleXmitSuccess(bundleZco) < 0)
			{
				putErrmsg("Can't handle xmit success.", NULL);
				break;
			}

			pause = 0;
			sm_TaskYield();
		}
	}

	if (client != NULL)
	{
		if (MQTTClient_isConnected(client))
		{
			MQTTClient_disconnect(client, 5000);
		}

		MQTTClient_destroy(&client);
	}

	writeErrmsgMemos();
	writeMemo("[i] mqttclo duct has ended.");
	MRELEASE(buffer);
	ionDetach();
	return 0;
}
