/******************************************************************************
* Declarations, global to other of the GIF-HASH.C module.		      *
*									      *
*					Written by Gershon Elber,  Jun 1989   *
*******************************************************************************
* History:								      *
* 14 Jun 89 - Version 1.0 by Gershon Elber.				      *
******************************************************************************/

#ifndef _GIF_HASH_H_
#define _GIF_HASH_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* Find a thirty-two bit int type */
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_BASETSD_H
#include <basetsd.h>
#endif

#define HT_SIZE			8192	   /* 12bits = 4096 or twice as big! */
#define HT_KEY_MASK		0x1FFF			      /* 13bits keys */
#define HT_KEY_NUM_BITS		13			      /* 13bits keys */
#define HT_MAX_KEY		8191	/* 13bits - 1, maximal code possible */
#define HT_MAX_CODE		4095	/* Biggest code possible in 12 bits. */

/* The 32 bits of the long are divided into two parts for the key & code:   */
/* 1. The code is 12 bits as our compression algorithm is limited to 12bits */
/* 2. The key is 12 bits Prefix code + 8 bit new char or 20 bits.	    */
/* The key is the upper 20 bits.  The code is the lower 12. */
#define HT_GET_KEY(l)	(l >> 12)
#define HT_GET_CODE(l)	(l & 0x0FFF)
#define HT_PUT_KEY(l)	(l << 12)
#define HT_PUT_CODE(l)	(l & 0x0FFF)

typedef struct GifHashTableType {
    UINT32 HTable[HT_SIZE];
} GifHashTableType;

GifHashTableType *_InitHashTable(void);
void _ClearHashTable(GifHashTableType *HashTable);
void _InsertHashTable(GifHashTableType *HashTable, UINT32 Key, int Code);
int _ExistsHashTable(GifHashTableType *HashTable, UINT32 Key);

#endif /* _GIF_HASH_H_ */
