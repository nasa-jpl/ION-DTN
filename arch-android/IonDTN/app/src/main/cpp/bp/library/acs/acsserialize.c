/*
	acsserialize.c: Serialize an ACS into a block of working memory.
	Authors: Andrew Jenkins, Sebastian Kuzminsky,
				University of Colorado at Boulder

	Copyright (c) 2008-2011, Regents of the University of Colorado.
	This work was supported by NASA contracts NNJ05HE10G, NNC06CB40C, and
	NNC07CB47C.
 											*/

#include "acsP.h"


/* FIXME: Can this go in sdrlist? */
static void	sdr_list_foreach(Sdr sdr, Object list,
			void (*callback)(Sdr, Object, void *), void *args)
{
	Object	listElt;
	Object	listData;

	for(listElt = sdr_list_first(sdr, list);
		listElt;
		listElt = sdr_list_next(sdr, listElt))
	{
		listData = sdr_list_data(sdr, listElt);
		callback(sdr, listData, args);
	}
}

typedef struct {
	unsigned char	*buf;
	unsigned long	iBuf;
	unsigned long	bufSize;
	unsigned long 	lastFill;
} SerializeForeachArgs_t;

static void serializeFill(Sdr sdr, Object fillAddr, void *argsAsVoid)
{
	SerializeForeachArgs_t	*args = (SerializeForeachArgs_t *)(argsAsVoid);
	SdrAcsFill		fill;
	Sdnv			encoded;

	assert(sdr_in_xn(sdr));

	sdr_peek(sdr, fill, fillAddr);

	/* Write a start-delta. */
	encodeSdnv(&encoded, fill.start - args->lastFill);
	memcpy(args->buf + args->iBuf, encoded.text, encoded.length);
	args->iBuf += encoded.length;
	assert(args->bufSize >= args->iBuf);

	/* Write a length. */
	encodeSdnv(&encoded, fill.length);
	memcpy(args->buf + args->iBuf, encoded.text, encoded.length);
	args->iBuf += encoded.length;
	assert(args->bufSize >= args->iBuf);

	args->lastFill = fill.start + fill.length - 1;
}

unsigned long serializeAcs(Object signalAddr, Object *serializedZco,
		unsigned long lastSerializedSize)
{
	SdrAcsSignal		signal;
	SerializeForeachArgs_t	args;
	Object                  serializedSdrAddr;
	Object                  serializedZcoAddr;
	Sdr			bpSdr = getIonsdr();
	Sdr			acsSdr = getAcssdr();
	vast			extentLength;

	/* We rely on incremental updates (reserialization) to serializedZco
	 * to determine the right size of buffer to allocate:
	 *  - If there is no existing serializedZco, then there is no more
	 *    than one signal in the list, so the maximum size of buf is known.
	 *
	 *  - If there is an existing serializedZco, then we are adding at most
	 *    one more fill to the serialization, so the maximum increase in
	 *	  size of buf is known.
	 *
	 * This calculation is pessimistic, but it's just our working memory;
	 * we actually allocate a ZCO of the exact right size after we're done
	 * serializing. */
	if (lastSerializedSize == 0)
	{
		args.bufSize = 1    /* Admin Record header byte */
				+ 1         /* reasonCode, succeeded byte */
				+ 20;       /* 1 start, length SDNV double */
	} else {
		args.bufSize = lastSerializedSize
				+ 20;       /* Possibly new start, length SDNV double. */
	}

	if((args.buf = MTAKE(args.bufSize)) == 0)
	{
		putErrmsg("Can't allocate for ACS serialization", itoa(args.bufSize));
		return 0;
	}
	args.iBuf = 0;

	ASSERT_BPSDR_XN;
	ASSERT_ACSSDR_XN;
	sdr_peek(acsSdr, signal, signalAddr);

	/* Assign the admin record header byte. */
	args.buf[args.iBuf] = (BP_AGGREGATE_CUSTODY_SIGNAL << 4) | 0x00;
	args.iBuf++;
	assert(args.bufSize >= args.iBuf);

	/* Assign the status byte. */
	args.buf[args.iBuf] = (signal.succeeded << 7) | (signal.reasonCode & 0x7F);
	args.iBuf++;
	assert(args.bufSize >= args.iBuf);

	/* Serialize the acsFills. */
	args.lastFill = 0;
	sdr_list_foreach(acsSdr, signal.acsFills, serializeFill, &args);

	serializedSdrAddr = sdr_malloc(bpSdr, args.iBuf);
	if (serializedSdrAddr == 0)
	{
		putErrmsg("Can't sdr_malloc() ACS payload", itoa(args.iBuf));
		MRELEASE(args.buf);
		return 0;
	}
	sdr_write(bpSdr, serializedSdrAddr, (char *)(args.buf), args.iBuf);
	MRELEASE(args.buf);

	/*	Pass additive inverse of length to zco_create to
	 *	indicate that allocating this ZCO space is non-
	 *	negotiable: for custody signals, allocation of ZCO
	 *	space can never be denied or delayed.			*/

	extentLength = args.iBuf;
	serializedZcoAddr = zco_create(bpSdr, ZcoSdrSource,
			serializedSdrAddr, 0, 0 - extentLength, ZcoOutbound);
	if (serializedZcoAddr == (Object) ERROR || serializedZcoAddr == 0)
	{
		putErrmsg("Can't put ACS payload into a ZCO",
				itoa(serializedZcoAddr));
		return 0;
	}

	/* Assign output args and return. */
	*serializedZco = serializedZcoAddr;
	return args.iBuf;
}
