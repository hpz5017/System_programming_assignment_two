#ifndef CMPSC311_HASHTABLE_INCLUDED
#define CMPSC311_HASHTABLE_INCLUDED

////////////////////////////////////////////////////////////////////////////////
//
//  File          : cmpsc311_hashtable.h
//  Description   : This is a generic hashtable implementation used for
//                  data structure storage and access.
//
//  Author   : Patrick McDaniel
//  Last Modified  : Sat Sep 2nd 08:56:10 EDT 2017
//
////////////////////////////////////////////////////////////////////////////////
//
// STUDENTS MUST ADD COMMENTS BELOW
//


// Includes
#include <stdint.h>

// Defines
#define HT_COOKIE_VALUE 0xa3a3
typedef unsigned long HtIndexValue;

// Hash table entry structure
typedef struct HtEntry {
	uint16_t 	    cookie;  // This is a cookie value to detect memory corruption
	HtIndexValue	index;   // This is the "key value" index of the object
	void           *block;   // This is the data block of the stored item
	struct HtEntry *prev;    // This is the previous item in the local chain
	struct HtEntry *next;    // This is the next item in the local chain
} HtEntryData;

// Hash table structure
typedef struct  {
	uint16_t         htTableSize;  // The the bits in the hash values
	HtEntryData    **hasHTable;   // This is the hash table itself
} HTable;

// Hash table iterator
typedef struct {
	HTable      *table; // The table we are iterating through
	uint16_t 	 idx;   // The current index into the hash table
	HtEntryData *ptr;   // The pointer into the linked list at the index
} HtIterator;

//
// Hashtable Interface

int initHasHTable( HTable *ht, uint16_t bits );
	// This function initializes the hash table to a width of 2^(bits) width

int cleanupHasHTable( HTable *ht );
	// Cleanup the hash table

int insertValueInHasHTable( HTable *ht, HtIndexValue idx, void *blk );
	// Insert a value into the hashtable of value idx, block size blk

void * findValueInHasHTable( HTable *ht, HtIndexValue idx );
	// Find a block for a particular index value in the table

void * deleteValueFromHasHTable( HTable *ht, HtIndexValue idx );
	// Delete a value from the hashtable of value idx, return it

//
// Iterator Functions

int initHasHTableIterator( HTable *ht, HtIterator *it );
	// Initialize the iterator

void * iterateHasHTable( HtIterator *it );
	// Iterate through the hash table (pass NULL to start at beginning),
	// returns the next value in the table

//
// Unit Testing

int hashTableUnitTest( void );
	// Perform a test of the hash table functionality

#endif
