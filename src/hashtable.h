#ifndef ASPIC_HASHTABLE_H
#define ASPIC_HASHTABLE_H

#include "object.h"
#include "value.h"

/**
 * Hashtable<const ObjectString*, Value>
 *
 * The hashtable is not the owner of keys and values.
 * Keys are ObjectString points, they must be kept valid during hashtable lifetime
 * Value are shallow-copied, internal pointers must be kept valid
 *
 * The read_only mecanism prevents from updating a value once it's inserted.
 */

typedef struct {
    const ObjectString* key;
    Value value;
    bool read_only;
} Entry;

typedef struct {
    size_t count_with_tombstones;
    size_t count;
    size_t capacity;
    Entry* entries;
} Hashtable;

/**
 * Hashtable ctor
 */
void hashtable_init(Hashtable* table);

/**
 * Hashtable ctor
 */
void hashtable_free(Hashtable* table);

/**
 * Set a new (key, value) pair
 * @return true if new key was inserted, otherwise false
 */
bool hashtable_set(Hashtable* table, const ObjectString* key, Value value, bool read_only);

typedef enum {
    HASHTABLE_MISS,
    HASHTABLE_READ_ONLY,
    HASHTABLE_SUCCESS
} HashtableLookup;

/**
 * Update a value if key already exists
 */
HashtableLookup hashtable_update(Hashtable* table, const ObjectString* key, Value value);

/**
 * Get value for a given key
 * @return value associated to key, otherwise NULL
 */
Value* hashtable_get(const Hashtable* table, const ObjectString* key);

/**
 * Delete an entry
 * @return true if key was deleted, otherwise false
 */
bool hashtable_delete(Hashtable* table, const ObjectString* key);

/**
 * Copy all entries from source to dest
 */
void hashtable_copy(const Hashtable* source, Hashtable* dest);

/**
 * Print all key-value pair to stdout
 */
void hashtable_print(const Hashtable* table);

#endif
