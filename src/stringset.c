#include "stringset.h"

#include <stdio.h>
#include <string.h>

#define HASHTABLE_MAX_LOAD 0.75
#define HASHTABLE_MIN_SIZE 8
#define HASHTABLE_GROW_FACTOR 2

static SetEntry* find_entry(SetEntry* entries, size_t capacity, const ObjectString* string)
{
    uint32_t index = string->hash % capacity;
    SetEntry* tombstone = NULL;

    while (true) {
        SetEntry* entry = &entries[index];

        // Entry is available
        if (entry->string == NULL) {
            // Check if really empty, or a tombstone
            if (!entry->tombstone) {
                // Empty entry found
                return tombstone != NULL ? tombstone : entry;
            } else if (tombstone == NULL) {
                // Don't treat tombstone like an empty slot, keep iterating
                tombstone = entry;
            }
        } else if (entry->string == string) {
            //  Key is found
            return entry;
        }

        // Collision: the desired index if already taken by a different string.
        // Loop over all the entries until an available entry is found.
        // If index goes past the end of the array, go back to the beginning.
        index = (index + 1) % capacity;
    }
    return NULL; // Unreachable
}

static void adjust_capacity(StringSet* set, size_t new_capacity)
{
    SetEntry* entries = malloc(sizeof(SetEntry) * new_capacity);
    if (entries == NULL) {
        fprintf(stderr, "Cannot allocate StringSet entry of size %ld\n", new_capacity);
        exit(1);
    }

    // Initialize every element to an empty entry
    for (size_t i = 0; i < new_capacity; ++i) {
        entries[i].string = NULL;
        entries[i].tombstone = false;
    }

    // Recalculate the buckets for each of the existing entry in the set.
    // To handle collisions, rebuild the set from scratch by re-inserting
    // every entry into the new empty array.
    set->count = 0;
    set->count_with_tombstones = 0;
    for (size_t i = 0; i < set->capacity; ++i) {
        SetEntry* current = &set->entries[i];
        if (current->string != NULL) {
            SetEntry* dest = find_entry(entries, new_capacity, current->string);
            dest->string = current->string;
            set->count++;
            set->count_with_tombstones++;
        }
    }

    // Deallocate old entries and replace them with new entries
    free(set->entries);
    set->entries = entries;
    set->capacity = new_capacity;
}

void stringset_init(StringSet* set)
{
    set->count = 0;
    set->count_with_tombstones = 0;
    set->capacity = 0;
    set->entries = NULL;
}

void stringset_free(StringSet* set)
{
    if (set->entries != NULL) {
        free(set->entries);
    }
    stringset_init(set);
}

bool stringset_add(StringSet* set, const ObjectString* string)
{
    // Ensure the set capacity is above overload factor
    if (set->count_with_tombstones + 1 > set->capacity * HASHTABLE_MAX_LOAD) {
        size_t capacity = set->capacity < HASHTABLE_MIN_SIZE
            ? HASHTABLE_MIN_SIZE
            : set->capacity * HASHTABLE_GROW_FACTOR;
        adjust_capacity(set, capacity);
    }

    SetEntry* entry = find_entry(set->entries, set->capacity, string);
    bool new_key = entry->string == NULL;
    if (new_key) {
        set->count++;
        // Increase count_with_tombstones only if the bucket was not already
        // used as a tombstone
        if (!entry->tombstone) {
            set->count_with_tombstones++;
        }
    }

    // Write entry
    entry->string = string;
    return new_key;
}

const ObjectString* stringset_has_cstr(StringSet* set, const char* chars, size_t length, uint32_t hash)
{
    if (set->count == 0) {
        return NULL;
    }

    uint32_t index = hash % set->capacity;

    for (;;) {
        SetEntry* entry = &set->entries[index];

        if (entry->string == NULL) {
            // Stop if we find an empty non-tombstone entry
            if (!entry->tombstone) {
                return NULL;
            }
        } else if (entry->string->hash == hash
            && entry->string->length == (int)length
            && memcmp(entry->string->chars, chars, length) == 0) {
            // We found it
            return entry->string;
        }
        index = (index + 1) % set->capacity;
    }
    return NULL;
}

bool stringset_delete(StringSet* set, const ObjectString* string)
{
    if (set->count == 0) {
        return false;
    }

    SetEntry* entry = find_entry(set->entries, set->capacity, string);
    if (entry->string == NULL) {
        return false;
    }

    // Flag entry with a tombstone. Use bool as a special marker, empty entry
    // have a null value instead.
    entry->string = NULL;
    entry->tombstone = true;
    return true;
}

void stringset_print(const StringSet* set)
{
    for (size_t i = 0; i < set->capacity; ++i) {
        SetEntry* entry = &set->entries[i];
        if (entry->string) {
            printf("\"%s\"\n", entry->string->chars);
        }
    }
}
