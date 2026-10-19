/*
	libtcp4cla.c:	common functions for TCPCLv4 convergence-layer
			daemons.

	Author: iondev33

	Copyright (c) 2026, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
								*/

#include "tcp4cla.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>

int	tcp4_parse_config(const char *ductName, Tcp4Config *cfg)
{
	if (ductName == NULL || cfg == NULL)
	{
		return -1;
	}

	memset(cfg, 0, sizeof(Tcp4Config));
	snprintf(cfg->duct_name, sizeof(cfg->duct_name), "%s", ductName);

	/*	Derive pool names from duct name.			*/

	snprintf(cfg->tx_pool_name, sizeof(cfg->tx_pool_name),
			"/ion_tcp4_tx_%s", ductName);
	snprintf(cfg->rx_pool_name, sizeof(cfg->rx_pool_name),
			"/ion_tcp4_rx_%s", ductName);

	/*	Derive control socket paths from duct name.		*/

	snprintf(cfg->tx_ctrl_path, sizeof(cfg->tx_ctrl_path),
			"%s/tx_%s.sock", TCP4CLA_SOCK_DIR, ductName);
	snprintf(cfg->rx_ctrl_path, sizeof(cfg->rx_ctrl_path),
			"%s/rx_%s.sock", TCP4CLA_SOCK_DIR, ductName);

	/*	Apply defaults.						*/

	cfg->num_slots = TCP4CLA_DEFAULT_NUM_SLOTS;
	cfg->slot_data_capacity = TCP4CLA_DEFAULT_SLOT_SIZE;

	return 0;
}

int	tcp4_apply_pool_overrides(Tcp4Config *cfg, const char *numSlotsStr,
		const char *slotSizeStr)
{
	char	*end;
	char	errbuf[128];

	if (cfg == NULL)
	{
		return -1;
	}

	/*	Optional runtime override of the shared-memory pool
	 *	geometry.  Both arguments may be NULL (keep the
	 *	compile-time default).  The pool is a single fixed
	 *	allocation made once, at daemon start; these values only
	 *	choose its size.  The floors match the compile-time
	 *	guards in tcp4cla.h so the pool cannot be misconfigured
	 *	smaller than a viable transfer slot.			*/

	if (numSlotsStr != NULL)
	{
		unsigned long	v;

		errno = 0;
		v = strtoul(numSlotsStr, &end, 10);
		if (end == numSlotsStr || *end != '\0' || errno != 0
				|| v < 2 || v > UINT32_MAX)
		{
			isprintf(errbuf, sizeof errbuf, "[?] tcp4: num_slots "
					"must be an integer >= 2: %s",
					numSlotsStr);
			writeMemo(errbuf);
			return -1;
		}

		cfg->num_slots = (uint32_t) v;
	}

	if (slotSizeStr != NULL)
	{
		unsigned long long	v;

		errno = 0;
		v = strtoull(slotSizeStr, &end, 10);
		if (end == slotSizeStr || *end != '\0' || errno != 0
				|| v < (64 * 1024) || v > UINT32_MAX)
		{
			isprintf(errbuf, sizeof errbuf, "[?] tcp4: slot size "
					"(bytes) must be an integer >= 65536: %s",
					slotSizeStr);
			writeMemo(errbuf);
			return -1;
		}

		cfg->slot_data_capacity = (uint32_t) v;
	}

	return 0;
}

/*	*	*	In-flight table functions	*	*	*/

void	tcp4_inflight_init(Tcp4InFlight *table, int size)
{
	memset(table, 0, sizeof(Tcp4InFlight) * size);
}

int	tcp4_inflight_add(Tcp4InFlight *table, int size,
		SdrObject bundleZco, uint64_t xfer_id)
{
	int	i;

	for (i = 0; i < size; i++)
	{
		if (!(table[i].in_use))
		{
			table[i].bundleZco = bundleZco;
			table[i].xfer_id = xfer_id;
			table[i].in_use = 1;
			return 0;
		}
	}

	return -1;	/*	Table full.				*/
}

SdrObject tcp4_inflight_remove(Tcp4InFlight *table, int size,
		uint64_t xfer_id)
{
	int	i;

	for (i = 0; i < size; i++)
	{
		if (table[i].in_use && table[i].xfer_id == xfer_id)
		{
			SdrObject zco = table[i].bundleZco;

			table[i].in_use = 0;
			table[i].bundleZco = 0;
			return zco;
		}
	}

	return 0;	/*	Not found.				*/
}

void	tcp4_inflight_fail_all(Tcp4InFlight *table, int size)
{
	int	i;

	for (i = 0; i < size; i++)
	{
		if (table[i].in_use)
		{
			if (table[i].bundleZco != 0)
			{
				oK(bpHandleXmitFailure(table[i].bundleZco));
			}

			table[i].in_use = 0;
			table[i].bundleZco = 0;
		}
	}
}

/*	*	*	Bundle extraction	*	*	*	*/

int	tcp4_extract_bundle(Sdr sdr, SdrObject bundleZco,
		IonTransferPool *pool, int slot_idx)
{
	ZcoReader	reader;
	char		*dest;
	IonPoolSlotHdr	*sh;
	int		bytesToSend;

	dest = pool_slot_data(pool, slot_idx);
	sh = pool_slot_hdr(pool, slot_idx);
	if (dest == NULL || sh == NULL)
	{
		return -1;
	}

	zco_start_transmitting(bundleZco, &reader);
	zco_track_file_offset(&reader);

	CHKERR(sdr_begin_xn(sdr));
	bytesToSend = zco_transmit(sdr, &reader, sh->capacity, dest);

	if (sdr_end_xn(sdr) < 0 || bytesToSend < 0)
	{
		putErrmsg("tcp4cla: can't extract from ZCO.", NULL);
		return -1;
	}

	return bytesToSend;
}
