/*****************************************************************************
*   "Gif-Lib" - Yet another gif library.				     *
*									     *
* Written by:  Gershon Elber			IBM PC Ver 0.1,	Jun. 1989    *
******************************************************************************
* Module to support the following operations:				     *
*									     *
* 1. InitHashTable - initialize hash table.				     *
* 2. ClearHashTable - clear the hash table to an empty state.		     *
* 2. InsertHashTable - insert one item into data structure.		     *
* 3. ExistsHashTable - test if item exists in data structure.		     *
*									     *
* This module is used to hash the GIF codes during encoding.		     *
******************************************************************************
* History:								     *
* 14 Jun 89 - Version 1.0 by Gershon Elber.				     *
*****************************************************************************/

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

#ifdef __MSDOS__
#include <io.h>
#include <alloc.h>
#include <sys\stat.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#endif /* __MSDOS__ */

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif /* HAVE_FCNTL_H */
#include <stdio.h>
#include <string.h>
#include "gif_lib.h"
#include "gif_hash.h"
#include "gif_lib_private.h"

/* #define  DEBUG_HIT_RATE    Debug number of misses per hash Insert/Exists. */

#ifdef	DEBUG_HIT_RATE
static long NumberOfTests = 0,
	    NumberOfMisses = 0;
#endif	/* DEBUG_HIT_RATE */

static int KeyItem(UINT32 Item);

/******************************************************************************
* Initialize HashTable - allocate the memory needed and clear it.	      *
******************************************************************************/
GifHashTableType *_InitHashTable(void)
{
    GifHashTableType *HashTable;

    if ((HashTable = (GifHashTableType *) malloc(sizeof(GifHashTableType)))
	== NULL)
	return NULL;

    _ClearHashTable(HashTable);

    return HashTable;
}

/******************************************************************************
* Routine to clear the HashTable to an empty state.			      *
* This part is a little machine depended. Use the commented part otherwise.   *
******************************************************************************/
void _ClearHashTable(GifHashTableType *HashTable)
{
    memset(HashTable -> HTable, 0xFF, HT_SIZE * sizeof(UINT32));
}

/******************************************************************************
* Routine to insert a new Item into the HashTable. The data is assumed to be  *
* new one.								      *
******************************************************************************/
void _InsertHashTable(GifHashTableType *HashTable, UINT32 Key, int Code)
{
    int HKey = KeyItem(Key);
    UINT32 *HTable = HashTable -> HTable;

#ifdef DEBUG_HIT_RATE
	NumberOfTests++;
	NumberOfMisses++;
#endif /* DEBUG_HIT_RATE */

    while (HT_GET_KEY(HTable[HKey]) != 0xFFFFFL) {
#ifdef DEBUG_HIT_RATE
	    NumberOfMisses++;
#endif /* DEBUG_HIT_RATE */
	HKey = (HKey + 1) & HT_KEY_MASK;
    }
    HTable[HKey] = HT_PUT_KEY(Key) | HT_PUT_CODE(Code);
}

/******************************************************************************
* Routine to test if given Key exists in HashTable and if so returns its code *
* Returns the Code if key was found, -1 if not.				      *
******************************************************************************/
int _ExistsHashTable(GifHashTableType *HashTable, UINT32 Key)
{
    int HKey = KeyItem(Key);
    UINT32 *HTable = HashTable -> HTable, HTKey;

#ifdef DEBUG_HIT_RATE
	NumberOfTests++;
	NumberOfMisses++;
#endif /* DEBUG_HIT_RATE */

    while ((HTKey = HT_GET_KEY(HTable[HKey])) != 0xFFFFFL) {
#ifdef DEBUG_HIT_RATE
	    NumberOfMisses++;
#endif /* DEBUG_HIT_RATE */
	if (Key == HTKey) return HT_GET_CODE(HTable[HKey]);
	HKey = (HKey + 1) & HT_KEY_MASK;
    }

    return -1;
}

/******************************************************************************
* Routine to generate an HKey for the hashtable out of the given unique key.  *
* The given Key is assumed to be 20 bits as follows: lower 8 bits are the     *
* new postfix character, while the upper 12 bits are the prefix code.	      *
* Because the average hit ratio is only 2 (2 hash references per entry),      *
* evaluating more complex keys (such as twin prime keys) does not worth it!   *
******************************************************************************/
static int KeyItem(UINT32 Item)
{
    return ((Item >> 12) ^ Item) & HT_KEY_MASK;
}

#ifdef	DEBUG_HIT_RATE
/******************************************************************************
* Debugging routine to print the hit ratio - number of times the hash table   *
* was tested per operation. This routine was used to test the KeyItem routine *
******************************************************************************/
void HashTablePrintHitRatio(void)
{
    printf("Hash Table Hit Ratio is %ld/%ld = %ld%%.\n",
	NumberOfMisses, NumberOfTests,
	NumberOfMisses * 100 / NumberOfTests);
}
#endif	/* DEBUG_HIT_RATE */
