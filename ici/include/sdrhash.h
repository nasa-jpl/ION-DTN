/*

	sdrhash.h:	definitions supporting use of SDR hash tables.

			Hash tables associate values with keys.  A
			value is always in the form of an SDR Address,
			nominally the address of some stored object
			identified by the associated key, but the
			actual significance of a value may be
			anything that fits into an unsigned long.
			A key is always an array of from 1 to 255
			bytes, which may have any semantics at all.

			Keys must be unique; no two distinct entries
			in an SDR hash table may have the same key.
			Any attempt to insert a duplicate entry will
			be rejected.

			All keys must be of the same length, and that
			length must be declared at the time the hash
			table is created.  Invoking a hash table
			function with a key that is shorter than the
			declared length will have unpredictable
			results.

	Author: Scott Burleigh, JPL
	
	Copyright (c) 2008 California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.

									*/
#ifndef _SDRHASH_H_
#define _SDRHASH_H_

#ifdef __cplusplus
extern "C" {
#endif

/*	Functions for operating on hash tables in SDR.			*/

#define sdr_hash_create(sdr, keyLength, estNbrOfEntries, meanSearchLength) \
Sdr_hash_create(__FILE__, __LINE__, sdr, keyLength, estNbrOfEntries, \
meanSearchLength)
extern Object		Sdr_hash_create(const char *file, int line, Sdr sdr,
				int keyLength, int estNbrOfEntries,
				int meanSearchLength);

#define sdr_hash_insert(sdr, hash, key, value, entry) \
Sdr_hash_insert(__FILE__, __LINE__, sdr, hash, key, value, entry)
extern int		Sdr_hash_insert(const char *file, int line, Sdr sdr,
				Object hash, char *key, Address value,
				Object *entry);

#define sdr_hash_delete_entry(sdr, entry) \
Sdr_hash_delete_entry(__FILE__, __LINE__, sdr, entry)
extern int		Sdr_hash_delete_entry(const char *file, int line,
				Sdr sdr, Object entry);

extern Address		sdr_hash_entry_value(Sdr sdr,
				Object hash, Object entry);

extern int		sdr_hash_retrieve(Sdr sdr,
				Object hash, char *key, Address *value,
				Object *entry);

typedef void	(*sdr_hash_callback)(Sdr sdr, Object hash,
				char *key, Address value, void *args);

extern int 		sdr_hash_foreach(Sdr sdrv, Object hash,
				sdr_hash_callback callback, void *args);

extern int		sdr_hash_count(Sdr sdr,
				Object hash);

#define sdr_hash_revise(sdr, hash, key, value) \
Sdr_hash_revise(__FILE__, __LINE__, sdr, hash, key, value)
extern int		Sdr_hash_revise(const char *file, int line, Sdr sdr,
				Object hash, char *key, Address value);

#define sdr_hash_remove(sdr, hash, key, value) \
Sdr_hash_remove(__FILE__, __LINE__, sdr, hash, key, value)
extern int		Sdr_hash_remove(const char *file, int line, Sdr sdr,
				Object hash, char *key, Address *value);

#define sdr_hash_destroy(sdr, hash) \
Sdr_hash_destroy(__FILE__, __LINE__, sdr, hash)
extern void		Sdr_hash_destroy(const char *file, int line, Sdr sdr,
				Object hash);

#ifdef __cplusplus
}
#endif

#endif  /* _SDRHASH_H_ */
