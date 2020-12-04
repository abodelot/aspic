#ifndef ASPIC_HASHTABLE_H
#define ASPIC_HASHTABLE_H

#include "value.h"
#include "object.h"

// Hashtable is not the owner of keys and value
// Keys are pointer, they must be kept valid during hashtable lifetime
// Value are shallow-copied, internal pointers must allow be kept valid
typedef struct {
    ObjectString* key;
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
bool hashtable_set(Hashtable* table, ObjectString* key, Value value, bool read_only);

typedef enum {
    HASHTABLE_MISS,
    HASHTABLE_READ_ONLY,
    HASHTABLE_SUCCESS
} HashtableLookup;

HashtableLookup hashtable_update(Hashtable* table, ObjectString* key, Value value);

/**
 * Get value for a given key
 * @return value associated to key, otherwise NULL
 */
Value* hashtable_get(const Hashtable* table, const ObjectString* key);

/**
 * Detect if a key is already present
 * @return key if already present, otherwise NULL
 */
ObjectString* hashtable_has_key_cstr(Hashtable* table, const char* chars, size_t length, uint32_t hash);

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
