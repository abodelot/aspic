#ifndef ASPIC_STRINGSET_H
#define ASPIC_STRINGSET_H

#include "object.h"

/**
 * StringSet<const ObjectString*>
 *
 * The stringset is not the owner of strings, strings must be kept valid during
 * the stringset lifetime.
 */

typedef struct {
    const ObjectString* string;
    bool tombstone;
} SetEntry;

typedef struct {
    size_t count_with_tombstones;
    size_t count;
    size_t capacity;
    SetEntry* entries;
} StringSet;

/**
 * StringSet ctor
 */
void stringset_init(StringSet* set);

/**
 * StringSet ctor
 */
void stringset_free(StringSet* set);

/**
 * @return true if new string was inserted, otherwise false
 */
bool stringset_add(StringSet* set, const ObjectString* string);

/**
 * Detect if a key is already present
 * @return key if already present, otherwise NULL
 */
const ObjectString* stringset_has_cstr(StringSet* set, const char* chars, size_t length, uint32_t hash);

/**
 * Delete an entry
 * @return true if key was deleted, otherwise false
 */
bool stringset_delete(StringSet* set, const ObjectString* key);

/**
 * Print all strings to stdout
 */
void stringset_print(const StringSet* set);

#endif
