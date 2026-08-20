/*
	bpsendmeta.c:	a bundle source that attaches a metadata
			extension block.

			The metadata block is defined in the extension
			definitions array but not in the extension
			specifications array, so it is never offered on
			a locally sourced bundle unless the sending
			application asks for it.  An application asks by
			passing the block's specification in the
			extensions member of BpAncillaryData, which is
			the supported way to request an extension block
			on a per-bundle basis.

			The block's content comes entirely from the
			metadata members of BpAncillaryData, so it is
			fully determined when the bundle is sourced,
			even though ION does not serialize it until the
			bundle is dequeued for transmission.  An
			extension block of that shape can be protected
			by a BIB or BCB that is sourced while the bundle
			is being forwarded, but not by one sourced when
			the bundle is created, since at that point the
			block has no content to protect.
									*/
/*									*/
/*	Copyright (c) 2026, California Institute of Technology.		*/
/*	ALL RIGHTS RESERVED.  U.S. Government Sponsorship		*/
/*	acknowledged.							*/
/*									*/

#include <bp.h>

#define	DEFAULT_TTL 300

#if defined (ION_LWT)
int	bpsendmeta(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char		*ownEid = (char *) a1;
	char		*destEid = (char *) a2;
	char		*text = (char *) a3;
	int		metadataType = a4 ? strtol((char *) a4, NULL, 0) : 1;
	int		ttl = a5 ? strtol((char *) a5, NULL, 0) : 0;
#else
int	main(int argc, char **argv)
{
	char		*ownEid = argc > 1 ? argv[1] : NULL;
	char		*destEid = argc > 2 ? argv[2] : NULL;
	char		*text = argc > 3 ? argv[3] : NULL;
	int		metadataType = argc > 4 ? strtol(argv[4], NULL, 0) : 1;
	int		ttl = argc > 5 ? strtol(argv[5], NULL, 0) : 0;
#endif
	BpAncillaryData	ancillaryData;
	ExtensionSpec	metadataSpec = { MetadataBlk, 0, NoCRC };
	BpSAP		sap;
	Sdr		sdr;
	int		textLength;
	int		metadataLength;
	SdrObject	extent;
	SdrObject	bundleZco;
	SdrObject	newBundle;

	if (ownEid == NULL || destEid == NULL || text == NULL)
	{
		PUTS("Usage: bpsendmeta <own EID> <dest EID> <text> \
[metadata type] [TTL]");
		PUTS("  Sends one bundle carrying a metadata extension \
block.");
		PUTS("  The text is both the payload and the metadata \
content.");
		return 1;
	}

	if (ttl <= 0)
	{
		ttl = DEFAULT_TTL;
	}

	/*	A metadata type of zero would make meb_offer decline the
	 *	block, which would defeat the purpose of this program.	*/

	if (metadataType <= 0 || metadataType > 255)
	{
		putErrmsg("Metadata type must be from 1 to 255.",
				itoa(metadataType));
		return 1;
	}

	if (bp_attach() < 0)
	{
		putErrmsg("bpsendmeta can't attach to BP.", NULL);
		return 1;
	}

	/*	Open the source endpoint so that the bundle carries a
	 *	source EID.  A bundle sent without one is anonymous, and
	 *	a security policy rule that selects bundles by source
	 *	could never match it.					*/

	if (bp_open(ownEid, &sap) < 0)
	{
		bp_detach();
		putErrmsg("bpsendmeta can't open own endpoint.", ownEid);
		return 1;
	}

	/*	The payload is sent with its terminating NUL, as the
	 *	other sending utilities do, but the metadata is a counted
	 *	byte string in the block and so carries only the text.	*/

	textLength = strlen(text) + 1;
	metadataLength = textLength - 1;
	if (metadataLength > BP_MAX_METADATA_LEN)
	{
		metadataLength = BP_MAX_METADATA_LEN;
	}

	/*	Requesting the block, and providing its content, are two
	 *	separate things: the extensions member names the block to
	 *	be offered, and the metadata members supply the content
	 *	that meb_serialize will encode at dequeue time.		*/

	memset((char *) &ancillaryData, 0, sizeof(BpAncillaryData));
	ancillaryData.extensions = &metadataSpec;
	ancillaryData.extensionsCt = 1;
	ancillaryData.metadataType = metadataType;
	ancillaryData.metadataLen = metadataLength;
	memcpy(ancillaryData.metadata, text, metadataLength);

	sdr = bp_get_sdr();
	CHKZERO(sdr_begin_xn(sdr));
	extent = sdr_malloc(sdr, textLength);
	if (extent)
	{
		sdr_write(sdr, extent, text, textLength);
	}

	if (sdr_end_xn(sdr) < 0 || extent == 0)
	{
		bp_close(sap);
		bp_detach();
		putErrmsg("No space for bpsendmeta payload.", NULL);
		return 1;
	}

	bundleZco = ionCreateZco(ZcoSdrSource, extent, 0, textLength,
			BP_STD_PRIORITY, 0, ZcoOutbound, NULL);
	if (bundleZco == 0 || bundleZco == (SdrObject) ERROR)
	{
		bp_close(sap);
		bp_detach();
		putErrmsg("bpsendmeta can't create ZCO.", NULL);
		return 1;
	}

	if (bp_send(sap, destEid, NULL, ttl, BP_STD_PRIORITY,
			NoCustodyRequested, 0, 0, &ancillaryData,
			bundleZco, &newBundle) < 1)
	{
		putErrmsg("bpsendmeta can't send ADU.", NULL);
		fprintf(stderr, "bpsendmeta: bundle send failed.\n");
		CHKZERO(sdr_begin_xn(sdr));
		zco_destroy(sdr, bundleZco);
		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("Can't destroy ZCO.", NULL);
		}

		bp_close(sap);
		bp_detach();
		return 1;
	}

	writeMemo("[i] bpsendmeta sent a bundle with a metadata block.");
	bp_close(sap);
	bp_detach();
	return 0;
}
