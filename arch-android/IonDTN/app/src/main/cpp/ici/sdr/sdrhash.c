/*
 *	sdrhash.c:	simple data recorder hash table management
 *			library.
 *
 *	Copyright (c) 2008, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
 *	acknowledged.
 *
 *	Author: Scott Burleigh, JPL
 *
 */

#include "sdrP.h"
#include "sdrlist.h"
#include "sdrtable.h"
#include "sdrhash.h"

typedef struct
{
	Address	value;
	char	key[255];
} KvPair;

/*	*	*	Table management functions	*	*	*/

Object	Sdr_hash_create(const char *file, int line, Sdr sdrv, int keyLength,
		int estNbrOfEntries, int meanSearchLength)
{
	/*	Each row of an SDR hash table contains a linked list of 
 	*	key/value pairs.  Given a key, we look up the corresponding
 	*	value by hashing from the key to a row number and then
 	*	searching through the key/value pairs in the linked list at
 	*	that row.  Minimizing the lengths of the lists in the table
 	*	minimizes lookup time, but in so doing it either increases
 	*	the number of rows in the table (increasing the table's fixed
 	*	size) or limits the number of entries.  We try to strike a
 	*	balance by computing the number of rows automatically from
 	*	A = the estimated total number of entries in the table and
 	*	B = the desired mean list length:
 	*
 	*		1.	Raw preferred row count C = A / B.
 	*
 	*		2.	Actual row count must be a prime number
 	*			in order for the hash function to work
 	*			properly, so search the hash dimensions
 	*			table for the smallest row count that is
 	*			greater than or equal to C.		*/

	static const int	hashDimensions[] =
			{ 71, 131, 257, 521, 1031, 2053, 4099, 8209, 16411 };

	/*	Each of the predefined possible hash table dimensions is the
 	*	smallest prime number greater than some power of 2, starting
 	*	with 2**6 = 64.  16411 is the maximum hash table dimension,
 	*	used for any value of C that is greater than 8209; on a 32-bit
 	*	machine, this limit assures that the table itself (excluding
 	*	the lists at its rows) will occupy no more than 65644 bytes
 	*	plus the table header size.				*/

	int	rawRowCount;
	int	i;
	int	rowCount;
	Object	table;
	Address	rowAddr;
	Object	listAddr;

	if (!(sdr_in_xn(sdrv)))
	{
		oK(_iEnd(file, line, _notInXnMsg()));
		return 0;
	}

	joinTrace(sdrv, file, line);
	if (keyLength < 1 || keyLength > 255 || meanSearchLength < 1)
	{
		oK(_xniEnd(file, line, _apiErrMsg(), sdrv));
		return 0;
	}

	rawRowCount = estNbrOfEntries / meanSearchLength;
	i = 0;
	while (1)
	{
		rowCount = hashDimensions[i];
		i++;
		if (rowCount <= rawRowCount
		&& i < (sizeof hashDimensions / sizeof(int)))
		{
			continue;
		}

		break;
	}

	table = Sdr_table_create(file, line, sdrv, sizeof(Object), rowCount);
	if (table == 0)
	{
		oK(_iEnd(file, line, itoa(rowCount)));
		return 0;
	}

	/*	Save key length for hash function.			*/

	Sdr_table_user_data_set(file, line, sdrv, table, (Address) keyLength);

	/*	Create linked lists for all rows of the hash table.	*/

	for (i = 0; i < rowCount; i++)
	{
		rowAddr = sdr_table_row(sdrv, table, i);
		listAddr = Sdr_list_create(file, line, sdrv);
		if (listAddr == 0)
		{
			oK(_iEnd(file, line, "listAddr"));
			return 0;
		}

		_sdrput(file, line, sdrv, rowAddr, (char *) &listAddr,
				sizeof(Object), SystemPut);
	}

	return table;
}

static int	computeRowNbr(int rowCount, int keyLength, char *key)
{
	int		i;
	unsigned int	h = 0;
	unsigned int	g;

	for (i = 0; i < keyLength; i++, key++)
	{
		h = (h << 4) + *key;
		g = h & 0xf0000000;
		if (g)
		{
			h ^= g >> 24;
		}

		h &= ~g;
	}

	return h % rowCount;
}

int	Sdr_hash_insert(const char *file, int line, Sdr sdrv, Object hash,
		char *key, Address value, Object *entry)
{
	int	keyLength;
	int	kvpairLength;
	int	rowSize;
	int	rowCount;
	int	rowNbr;
	Address	rowAddr;
	Object	listAddr;
	Object	elt;
	Object	kvpairAddr;
	KvPair	kvpair;
	int	result;
	Object	hashElt;

	if (entry)
	{
		*entry = 0;	/*	Default result.			*/
	}

	if (!(sdr_in_xn(sdrv)))
	{
		oK(_iEnd(file, line, _notInXnMsg()));
		return -1;
	}

	joinTrace(sdrv, file, line);
	if (hash == 0 || key == NULL)
	{
		oK(_xniEnd(file, line, _apiErrMsg(), sdrv));
		return -1;
	}

	keyLength = sdr_table_user_data(sdrv, hash);
	kvpairLength = sizeof(Address) + keyLength;
	sdr_table_dimensions(sdrv, hash, &rowSize, &rowCount);
	rowNbr = computeRowNbr(rowCount, keyLength, key);
	rowAddr = sdr_table_row(sdrv, hash, rowNbr);
	sdr_read(sdrv, (char *) &listAddr, rowAddr, sizeof(Object));
	for (elt = sdr_list_first(sdrv, listAddr); elt;
			elt = sdr_list_next(sdrv, elt))
	{
		kvpairAddr = sdr_list_data(sdrv, elt);
		sdr_read(sdrv, (char *) &kvpair, kvpairAddr, kvpairLength);
		result = memcmp(kvpair.key, key, keyLength);
		if (result < 0)
		{
			continue;
		}

		if (result > 0)
		{
			break;	/*	Insert before this entry.	*/
		}

		return 0;	/*	Duplicate key, can't insert.	*/
	}

	kvpairAddr = Sdr_malloc(file, line, sdrv, kvpairLength);
	if (kvpairAddr == 0)
	{
		oK(_iEnd(file, line, "kvpairAddr"));
		return -1;
	}

	kvpair.value = value;
	memcpy(kvpair.key, key, keyLength);
	_sdrput(file, line, sdrv, kvpairAddr, (char *) &kvpair, kvpairLength,
			SystemPut);
	if (elt)
	{
		hashElt = Sdr_list_insert_before(file, line, sdrv, elt,
				kvpairAddr);
	}
	else
	{
		hashElt = Sdr_list_insert_last(file, line, sdrv, listAddr,
				kvpairAddr);
	}

	if (hashElt == 0)
	{
		oK(_iEnd(file, line, "elt"));
		return -1;
	}

	if (entry)
	{
		*entry = hashElt;
	}

	return 1;		/*	Succeeded.			*/
}

int	Sdr_hash_delete_entry(const char *file, int line, Sdr sdrv,
		Object entry)
{
	Object	kvpairAddr;

	if (!(sdr_in_xn(sdrv)))
	{
		oK(_iEnd(file, line, _notInXnMsg()));
		return -1;
	}

	joinTrace(sdrv, file, line);
	if (entry == 0)
	{
		oK(_xniEnd(file, line, _apiErrMsg(), sdrv));
		return -1;
	}

	kvpairAddr = sdr_list_data(sdrv, entry);
	Sdr_free(file, line, sdrv, kvpairAddr);
	Sdr_list_delete(file, line, sdrv, entry, NULL, NULL);
	return 1;
}

Address	sdr_hash_entry_value(Sdr sdrv, Object hash, Object entry)
{
	int	keyLength;
	int	kvpairLength;
	Object	kvpairAddr;
	KvPair	kvpair;

	CHKERR(sdrFetchSafe(sdrv));
	CHKERR(entry);
	keyLength = sdr_table_user_data(sdrv, hash);
	kvpairLength = sizeof(Address) + keyLength;
	kvpairAddr = sdr_list_data(sdrv, entry);
	sdr_read(sdrv, (char *) &kvpair, kvpairAddr, kvpairLength);
	return kvpair.value;
}

int	sdr_hash_retrieve(Sdr sdrv, Object hash, char *key, Address *value,
		Object *entry)
{
	int		keyLength;
	int		kvpairLength;
	int		rowSize;
	int		rowCount;
	int		rowNbr;
	Address		rowAddr;
	Object		listAddr;
	Object		elt;
	Address		kvpairAddr;
	KvPair		kvpair;
	int		result;

	if (entry)
	{
		*entry = 0;	/*	Default result.			*/
	}

	CHKERR(sdrFetchSafe(sdrv));
	CHKERR(hash);
	CHKERR(key);
	CHKERR(value);
	keyLength = sdr_table_user_data(sdrv, hash);
	kvpairLength = sizeof(Address) + keyLength;
	sdr_table_dimensions(sdrv, hash, &rowSize, &rowCount);
	rowNbr = computeRowNbr(rowCount, keyLength, key);
	rowAddr = sdr_table_row(sdrv, hash, rowNbr);
	sdr_read(sdrv, (char *) &listAddr, rowAddr, sizeof(Object));
	for (elt = sdr_list_first(sdrv, listAddr); elt;
			elt = sdr_list_next(sdrv, elt))
	{
		kvpairAddr = sdr_list_data(sdrv, elt);
		sdr_read(sdrv, (char *) &kvpair, kvpairAddr, kvpairLength);
		result = memcmp(kvpair.key, key, keyLength);
		if (result < 0)
		{
			continue;
		}

		if (result > 0)
		{
			break;	/*	Not found.			*/
		}

		*value = kvpair.value;
		if (entry)
		{
			*entry = elt;
		}

		return 1;	/*	Got it.				*/
	}

	return 0;		/*	Unable to retrieve value.	*/
}

int	sdr_hash_count(Sdr sdrv, Object hash)
{
	int	count = 0;
	int	rowSize;
	int	rowCount;
	int	i;
	Address	rowAddr;
	Object	listAddr;

	CHKERR(sdrv);
	CHKERR(hash);
	sdr_table_dimensions(sdrv, hash, &rowSize, &rowCount);
	for (i = 0; i < rowCount; i++)
	{
		rowAddr = sdr_table_row(sdrv, hash, i);
		sdr_read(sdrv, (char *) &listAddr, rowAddr, sizeof(Object));
		count += sdr_list_length(sdrv, listAddr);
	}

	return count;
}

int	sdr_hash_foreach(Sdr sdrv, Object hash, sdr_hash_callback callback,
		void *args)
{
	int		keyLength;
	int		kvpairLength;
	int		rowSize;
	int		rowCount;
	int		rowNbr;
	Address		rowAddr;
	Object		listAddr;
	Object		elt;
	Object		kvpairAddr;
	KvPair		kvpair;

	CHKERR(sdrFetchSafe(sdrv));
	CHKERR(hash);
	CHKERR(callback);
	//Passing NULL args is OK (passed through to callback)
	keyLength = sdr_table_user_data(sdrv, hash);
	kvpairLength = sizeof(Address) + keyLength;
	sdr_table_dimensions(sdrv, hash, &rowSize, &rowCount);

	/*	Iterate over each row/bucket, loading the sdrlist
	 *	of members of each.					*/

	for (rowNbr = 0; rowNbr < rowCount; rowNbr++)
	{
		rowAddr = sdr_table_row(sdrv, hash, rowNbr);
		sdr_read(sdrv, (char *) &listAddr, rowAddr, sizeof(Object));

		/*	Iterate over each member of this bucket.	*/

		for (elt = sdr_list_first(sdrv, listAddr); elt;
				elt = sdr_list_next(sdrv, elt))
		{
			kvpairAddr = sdr_list_data(sdrv, elt);
			sdr_read(sdrv, (char *) &kvpair, kvpairAddr,
					kvpairLength);

			/*	Call the callback passed to us with
			 *	the key, value pair.			*/

			callback(sdrv, hash, kvpair.key, kvpair.value, args);
		}
	}

	return 0;
}

int	Sdr_hash_revise(const char *file, int line, Sdr sdrv, Object hash,
		char *key, Address value)
{
	int	keyLength;
	int	kvpairLength;
	int	rowSize;
	int	rowCount;
	int	rowNbr;
	Address	rowAddr;
	Object	listAddr;
	Object	elt;
	Object	kvpairAddr;
	KvPair	kvpair;
	int	result;

	if (!(sdr_in_xn(sdrv)))
	{
		oK(_iEnd(file, line, _notInXnMsg()));
		return -1;
	}

	joinTrace(sdrv, file, line);
	if (hash == 0 || key == NULL)
	{
		oK(_xniEnd(file, line, _apiErrMsg(), sdrv));
		return -1;
	}

	keyLength = sdr_table_user_data(sdrv, hash);
	kvpairLength = sizeof(Address) + keyLength;
	sdr_table_dimensions(sdrv, hash, &rowSize, &rowCount);
	rowNbr = computeRowNbr(rowCount, keyLength, key);
	rowAddr = sdr_table_row(sdrv, hash, rowNbr);
	sdr_read(sdrv, (char *) &listAddr, rowAddr, sizeof(Object));
	for (elt = sdr_list_first(sdrv, listAddr); elt;
			elt = sdr_list_next(sdrv, elt))
	{
		kvpairAddr = sdr_list_data(sdrv, elt);
		sdr_read(sdrv, (char *) &kvpair, kvpairAddr, kvpairLength);
		result = memcmp(kvpair.key, key, keyLength);
		if (result < 0)
		{
			continue;
		}

		if (result > 0)
		{
			break;	/*	Not found.			*/
		}

		kvpair.value = value;
		_sdrput(file, line, sdrv, kvpairAddr, (char *) &kvpair,
				kvpairLength, SystemPut);
		return 1;	/*	Succeeded.			*/
	}

	return 0;		/*	Unable to revise value.		*/
}

int	Sdr_hash_remove(const char *file, int line, Sdr sdrv, Object hash,
		char *key, Address *value)
{
	int	keyLength;
	int	kvpairLength;
	int	rowSize;
	int	rowCount;
	int	rowNbr;
	Address	rowAddr;
	Object	listAddr;
	Object	elt;
	Object	kvpairAddr;
	KvPair	kvpair;
	int	result;

	if (!(sdr_in_xn(sdrv)))
	{
		oK(_iEnd(file, line, _notInXnMsg()));
		return -1;
	}

	joinTrace(sdrv, file, line);
	if (hash == 0 || key == NULL)
	{
		oK(_xniEnd(file, line, _apiErrMsg(), sdrv));
		return -1;
	}

	keyLength = sdr_table_user_data(sdrv, hash);
	kvpairLength = sizeof(Address) + keyLength;
	sdr_table_dimensions(sdrv, hash, &rowSize, &rowCount);
	rowNbr = computeRowNbr(rowCount, keyLength, key);
	rowAddr = sdr_table_row(sdrv, hash, rowNbr);
	sdr_read(sdrv, (char *) &listAddr, rowAddr, sizeof(Object));
	for (elt = sdr_list_first(sdrv, listAddr); elt;
			elt = sdr_list_next(sdrv, elt))
	{
		kvpairAddr = sdr_list_data(sdrv, elt);
		sdr_read(sdrv, (char *) &kvpair, kvpairAddr, kvpairLength);
		result = memcmp(kvpair.key, key, keyLength);
		if (result < 0)
		{
			continue;
		}

		if (result > 0)
		{
			break;	/*	Not found.			*/
		}

		if (value)
		{
			*value = kvpair.value;
		}

		Sdr_free(file, line, sdrv, kvpairAddr);
		Sdr_list_delete(file, line, sdrv, elt, NULL, NULL);
		return 1;	/*	Succeeded.			*/
	}

	return 0;		/*	Unable to remove entry.		*/
}

static void	deleteHashEntry(Sdr sdrv, Object eltData, void *arg)
{
	sdr_free(sdrv, eltData);
}

void	Sdr_hash_destroy(const char *file, int line, Sdr sdrv, Object hash)
{
	int	rowSize;
	int	rowCount;
	int	i;
	Address	rowAddr;
	Object	listAddr;

	if (!(sdr_in_xn(sdrv)))
	{
		oK(_iEnd(file, line, _notInXnMsg()));
		return;
	}

	joinTrace(sdrv, file, line);
	if (hash == 0)
	{
		oK(_xniEnd(file, line, "hash", sdrv));
		return;
	}

	sdr_table_dimensions(sdrv, hash, &rowSize, &rowCount);
	for (i = 0; i < rowCount; i++)
	{
		rowAddr = sdr_table_row(sdrv, hash, i);
		sdr_read(sdrv, (char *) &listAddr, rowAddr, sizeof(Object));
		Sdr_list_destroy(file, line, sdrv, listAddr, deleteHashEntry,
				NULL);
	}

	Sdr_table_destroy(file, line, sdrv, hash);
}
