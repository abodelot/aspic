#include "hashtable.h"

#include <stdio.h>
#include <string.h>

#define HASHTABLE_MAX_LOAD 0.75
#define HASHTABLE_MIN_SIZE 8
#define HASHTABLE_GROW_FACTOR 2

static Entry* find_entry(Entry* entries, size_t capacity, const ObjectString* key)
{
    uint32_t index = key->hash % capacity;
    Entry* tombstone = NULL;

    while (true) {
        Entry* entry = &entries[index];

        // Entry is available
        if (entry->key == NULL) {
            // Check if really empty, or a tombstone
            if (entry->value.type == TYPE_NULL) {
                // Empty entry found
                return tombstone != NULL ? tombstone : entry;
            } else if (tombstone == NULL) {
                // Don't treat tombstone like an empty slot, keep iterating
                tombstone = entry;
            }
        } else if (string_equal(entry->key, key)) {
            //  Key is found
            return entry;
        }

        // Collision: the desired index if already taken by a different key.
        // Loop over all the entries until an available entry is found.
        // If index goes past the end of the array, go back to the beginning.
        index = (index + 1) % capacity;
    }
    return NULL; // Unreachable
}

static void adjust_capacity(Hashtable* table, size_t new_capacity)
{
    Entry* entries = malloc(sizeof(Entry) * new_capacity);
    if (entries == NULL) {
        fprintf(stderr, "Cannot allocate Hashtable entry of size %ld\n", new_capacity);
        exit(1);
    }

    // Initialize every element to an empty entry
    for (size_t i = 0; i < new_capacity; ++i) {
        entries[i].key = NULL;
        entries[i].value = make_null();
    }

    // Recalculate the buckets for each of the existing entry in the hash table.
    // To handle collisions, rebuild the table from scratch by re-inserting
    // every entry into the new empty array.
    table->count = 0;
    table->count_with_tombstones = 0;
    for (size_t i = 0; i < table->capacity; ++i) {
        Entry* current = &table->entries[i];
        if (current->key != NULL) {
            Entry* dest = find_entry(entries, new_capacity, current->key);
            dest->key = current->key;
            dest->value = current->value;
            dest->read_only = current->read_only;
            table->count++;
            table->count_with_tombstones++;
        }
    }

    // Deallocate old entries and replace them with new entries
    free(table->entries);
    table->entries = entries;
    table->capacity = new_capacity;
}

void hashtable_init(Hashtable* table)
{
    table->count = 0;
    table->count_with_tombstones = 0;
    table->capacity = 0;
    table->entries = NULL;
}

void hashtable_free(Hashtable* table)
{
    if (table->entries != NULL) {
        free(table->entries);
    }
    hashtable_init(table);
}

bool hashtable_set(Hashtable* table, const ObjectString* key, Value value, bool read_only)
{
    // Ensure the table capacity is above overload factor
    if (table->count_with_tombstones + 1 > table->capacity * HASHTABLE_MAX_LOAD) {
        size_t capacity = table->capacity < HASHTABLE_MIN_SIZE
            ? HASHTABLE_MIN_SIZE
            : table->capacity * HASHTABLE_GROW_FACTOR;
        adjust_capacity(table, capacity);
    }

    Entry* entry = find_entry(table->entries, table->capacity, key);
    bool new_key = entry->key == NULL;
    if (new_key) {
        table->count++;
        // Increase count_with_tombstones only if the bucket was not already
        // used as a tombstone
        if (entry->value.type == TYPE_NULL) {
            table->count_with_tombstones++;
        }
    }

    // Write entry
    entry->key = key;
    entry->value = value;
    entry->read_only = read_only;
    return new_key;
}

HashtableLookup hashtable_update(Hashtable* table, const ObjectString* key, Value value)
{
    Entry* entry = find_entry(table->entries, table->capacity, key);
    if (entry->key == NULL) {
        return HASHTABLE_MISS;
    }
    if (entry->read_only) {
        return HASHTABLE_READ_ONLY;
    }

    entry->value = value;
    return HASHTABLE_SUCCESS;
}

Value* hashtable_get(const Hashtable* table, const ObjectString* key)
{
    if (table->count == 0) {
        return NULL;
    }
    Entry* entry = find_entry(table->entries, table->capacity, key);
    return entry->key == NULL ? NULL : &entry->value;
}

bool hashtable_delete(Hashtable* table, const ObjectString* key)
{
    if (table->count == 0) {
        return false;
    }

    Entry* entry = find_entry(table->entries, table->capacity, key);
    if (entry->key == NULL) {
        return false;
    }

    // Flag entry with a tombstone. Use bool as a special marker, empty entry
    // have a null value instead.
    entry->key = NULL;
    entry->value = make_bool(true);
    return true;
}

void hashtable_copy(const Hashtable* source, Hashtable* dest)
{
    for (size_t i = 0; i < source->capacity; ++i) {
        Entry* entry = &source->entries[i];
        if (entry->key != NULL) {
            hashtable_set(dest, entry->key, entry->value, entry->read_only);
        }
    }
}

void hashtable_print(const Hashtable* table)
{
    for (size_t i = 0; i < table->capacity; ++i) {
        Entry* entry = &table->entries[i];
        if (entry->key) {
            printf("%s %-16s = ", entry->read_only ? "RO" : "RW", entry->key->chars);
            value_repr(entry->value);
            printf(" [%s]\n", value_type(entry->value));
        }
    }
}
