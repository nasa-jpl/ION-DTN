/*
	libstcpcla.c:	common functions for BP convergence-layer
			daemons that invoke simple TCP functionality.

	Author: Scott Burleigh, JPL

	Copyright (c) 2006, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
	
									*/
#include "stcpcla.h"

typedef struct
{
	int	ductSocket;
	char	protocolName[MAX_CL_PROTOCOL_NAME_LEN + 1];
	char	ductName[MAX_CL_DUCT_NAME_LEN + 1];
} StcpOutductId;

static void	deleteOutductId(LystElt elt, void *userdata)
{
	MRELEASE(lyst_data(elt));
}

static int	_stcpOutductId(char *protocolName, char *ductName,
			int ductSocket)
{
	static Lyst	stcpOutductIds = NULL;
	LystElt		elt;
	StcpOutductId	*id = NULL;
	int		foundIt = 0;

	/*	To add new outduct ID, provide protocol name, duct
	 *	name, and socket number.  To query protocol name
	 *	and duct name for a given socket, provide socket
	 *	number and provide zero-length string buffers for
	 *	protocol name and duct name.  To delete an outduct
	 *	ID, provide socket number and provide NULL pointers
	 *	for protocol name and duct name.			*/

	CHKZERO(ductSocket > -1);
	if (stcpOutductIds == NULL)
	{
		stcpOutductIds = lyst_create_using(getIonMemoryMgr());
		CHKERR(stcpOutductIds);
		lyst_delete_set(stcpOutductIds, deleteOutductId, NULL);
	}

	for (elt = lyst_first(stcpOutductIds); elt; elt = lyst_next(elt))
	{
		id = (StcpOutductId *) lyst_data(elt);
		if (id->ductSocket < ductSocket)
		{
			continue;
		}

		if (id->ductSocket == ductSocket)
		{
			foundIt = 1;
		}

		break;
	}

	if (protocolName == NULL)	/*	Deleting outduct ID.	*/
	{
		if (foundIt)
		{
			lyst_delete(elt);
		}

		return 0;
	}

	if (*protocolName == 0)		/*	Retrieving outduct ID.	*/
	{
		if (foundIt)
		{
			istrcpy(protocolName, id->protocolName,
					MAX_CL_PROTOCOL_NAME_LEN);
			istrcpy(ductName, id->ductName, MAX_CL_DUCT_NAME_LEN);
		}

		return 0;
	}

	/*	Recording new STCP-based Outduct ID.			*/

	if (foundIt)
	{
		writeMemoNote("[?] Socket is already in StcpOutductSocket list",
				ductName);
		return 0;
	}

	id = (StcpOutductId *) MTAKE(sizeof(StcpOutductId));
	CHKERR(id);
	istrcpy(id->protocolName, protocolName, MAX_CL_PROTOCOL_NAME_LEN);
	istrcpy(id->ductName, ductName, MAX_CL_DUCT_NAME_LEN);
	id->ductSocket = ductSocket;
	if (elt)
	{
		elt = lyst_insert_before(elt, id);
	}
	else
	{
		elt = lyst_insert_last(stcpOutductIds, id);
	}

	CHKERR(elt);
	return 0;
}

static int	connectToCLI(char *protocolName, char *ductName, int *sock)
{
	if (*protocolName == '\0')
	{
		/*	E.g., brsscla called sendBundleByStcp but
		 *	the socket was closed.				*/

		return 0;	/*	Silently give up on connection.	*/
	}

	switch (itcp_connect(ductName, BpStcpDefaultPortNbr, sock))
	{
	case -1:
		putErrmsg("CLO failed connecting to CLI.", NULL);
		return -1;

	case 0:
		writeMemoNote("[?] CLO unable to connect to CLI", ductName);
		return 0;

	default:
		break;		/*	Out of switch.			*/
	}

	if (watchSocket(*sock) < 0)
	{
		closesocket(*sock);
		putErrmsg("CLO can't watch socket.", ductName);
		return -1;
	}

	if (_stcpOutductId(protocolName, ductName, *sock) < 0)
	{
		putErrmsg("Can't record STCP outduct ID for connection.", NULL);
		return -1;
	}

	return 1;	/*	CLO connected to remote CLI.		*/
}

int	openStcpOutductSocket(char *protocolName, char *ductName, int *sock)
{
	CHKERR(protocolName);
	CHKERR(ductName);
	CHKERR(sock);
	return connectToCLI(protocolName, ductName, sock);
}

void	closeStcpOutductSocket(int *ductSocket)
{
	CHKVOID(ductSocket);
	if (*ductSocket != -1)
	{
		/*	Forget the outduct ID and close the socket.	*/

		oK(_stcpOutductId(NULL, NULL, *ductSocket));
		closesocket(*ductSocket);
		*ductSocket = -1;
	}
}

static int	handleStcpFailure(Object bundleZco)
{
	/*	Make sure the bundle isn't dropped on the floor.	*/

	if (bundleZco == 0)
	{
		return 0;	/*	Just a keep-alive; no bundle.	*/
	}

	/*	Handle the de-queued bundle.				*/

       	if (bpHandleXmitFailure(bundleZco) < 0)
	{
		putErrmsg("Can't handle STCP xmit failure.", NULL);
		return -1;
	}

	return 0;
}

int	sendBundleByStcp(char *protocolName, char *ductName,
		int *sock, unsigned int bundleLength,
		Object bundleZco, char *buffer)
{
	unsigned int	preamble;

	/*	Connect to CLI as necessary.				*/

	if (*sock < 0)
	{
		switch (connectToCLI(protocolName, ductName, sock))
		{
		case -1:
			putErrmsg("STCP connection failure.", ductName);
			return -1;

		case 0:
			/*	Treat I/O error as a transient anomaly.	*/

			return handleStcpFailure(bundleZco);

		default:
			break;	/*	Successful connection.		*/
		}
	}

	/*	Send preamble (length), telling CLI how much to read.	*/

	preamble = htonl(bundleLength);
	switch (itcp_send(sock, (char *) &preamble, sizeof preamble))
	{
	case -1:
		putErrmsg("Failed to send bundle by STCP.", NULL);
		return -1;

	case 0:
		writeMemo("[?] Lost connection to CLI.");
		closeStcpOutductSocket(sock);
		return handleStcpFailure(bundleZco);

	default:
		break;		/*	Out of switch.			*/
	}

	if (bundleLength == 0)	/*	Just a keep-alive.		*/
	{
		return 1;	/*	Impossible length; means "OK".	*/
	}

	/*	Send the bundle itself.					*/

	switch(ionSendZcoByTCP(sock, bundleZco, buffer, STCPCLA_BUFSZ))
	{
	case -1:
		putErrmsg("Failed to send bundle by STCP.", NULL);
		return -1;

	case 0:
		writeMemo("[?] Lost connection to CLI.");
		closeStcpOutductSocket(sock);
		return handleStcpFailure(bundleZco);

	default:
		break;		/*	Out of switch.			*/
	}

	if (bpHandleXmitSuccess(bundleZco) < 0)
	{
		putErrmsg("Can't handle xmit success.", NULL);
		return -1;
	}

	return 0;
}

int	receiveBundleByStcp(int *sock, AcqWorkArea *work, char *buffer,
		ReqAttendant *attendant)
{
	unsigned int	preamble;
	unsigned int	bundleLength = 0;
	int		bytesToReceive;
	int		bytesReceived;
	int		totalBytesToReceive;
	int		extentSize;

	/*	Receive length of transmitted bundle.			*/

	while (bundleLength == 0)
	{
		bytesReceived = itcp_recv(sock, (char *) &preamble,
				sizeof preamble);
		if (bytesReceived < 1)
		{
			return bytesReceived;
		}

		bundleLength = ntohl(preamble);
	}

	/*	Receive the bundle itself, a buffer's worth at a
	 *	time.							*/

	totalBytesToReceive = bundleLength;
	while (totalBytesToReceive > 0)
	{
		bytesToReceive = totalBytesToReceive;
		if (bytesToReceive > STCPCLA_BUFSZ)
		{
			bytesToReceive = STCPCLA_BUFSZ;
		}

		extentSize = bytesToReceive;
		bytesReceived = itcp_recv(sock, buffer, extentSize);
		if (bytesReceived < 1)
		{
			return bytesReceived;
		}

		totalBytesToReceive -= extentSize;

		/*	Acquire the received data.			*/

		if (bpContinueAcq(work, buffer, extentSize, attendant, 0) < 0)
		{
			return -1;
		}
	}

	return bundleLength;
}
