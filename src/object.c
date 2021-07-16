#include "object.h"
#include "utils.h"
#include "value_array.h"
#include "vm.h"

#include <stdio.h>
#include <string.h>

// Object
//------------------------------------------------------------------------------

static void* object_new(ObjectType type, size_t size)
{
    Object* object = malloc(size);
    if (object == NULL) {
        fprintf(stderr, "Cannot allocate object of size %zu", size);
        exit(1);
    }
    object->type = type;

    // Register for GC (linked list vm.objects_head)
    object->next = NULL;
    vm_register_object(object);
    return object;
}

void object_free(Object* object)
{
    switch (object->type) {
    case OBJECT_ARRAY: {
        ObjectArray* array = (ObjectArray*)object;
        value_array_free(&array->array);
        free(array);
        break;
    }
    case OBJECT_FUNCTION: {
        // Note: no need to destroy function->name, Object pointers are already
        // tracked by the VM.
        ObjectFunction* function = (ObjectFunction*)object;
        // Destroy the chunk
        chunk_free(&function->chunk);

        free(function);
        break;
    }
    case OBJECT_STRING: {
        ObjectString* string = (ObjectString*)object;
        // Destroy the allocated string buffer
        free(string->chars);
        string->chars = NULL;

        free(string);
        break;
    }
    }
}

bool object_equal(const Object* a, const Object* b)
{
    if (a->type == b->type) {
        switch (a->type) {
        case OBJECT_ARRAY:
            return value_array_equal(
                &((const ObjectArray*)a)->array,
                &((const ObjectArray*)b)->array);
        case OBJECT_STRING:
            return string_equal((const ObjectString*)a, (const ObjectString*)b);
        case OBJECT_FUNCTION:
            return a == b;
        }
    }
    return false;
}

// ObjectString
//------------------------------------------------------------------------------

static uint32_t hash_string(const char* str, size_t length)
{
    // FNV-1a hash function
    uint32_t hash = 2166136261u;

    for (size_t i = 0; i < length; ++i) {
        hash ^= str[i];
        hash *= 16777619;
    }

    return hash;
}

const ObjectString* string_new(const char* chars, size_t length)
{
    uint32_t hash = hash_string(chars, length);

    // Check if string is already interned in the VM
    const ObjectString* interned = vm_find_string(chars, length, hash);
    if (interned) {
        return interned;
    }

    // Allocate new string object and copy string
    ObjectString* string = object_new(OBJECT_STRING, sizeof(ObjectString));
    string->chars = alloc_string(length);
    memcpy(string->chars, chars, length);
    string->length = length;
    string->hash = hash;

    return vm_intern_string(string);
}

static const ObjectString* string_ctor(char* chars, size_t length, uint32_t hash)
{
    // Check if string was already interned in the VM
    const ObjectString* interned = vm_find_string(chars, length, hash);
    if (interned) {
        // Destroy buffer and return interned string instead
        free(chars);
        return interned;
    }

    // Allocate new ObjectString and set attributes
    ObjectString* string = object_new(OBJECT_STRING, sizeof(ObjectString));
    string->chars = chars;
    string->length = length;
    string->hash = hash;

    // Ensure string is interned by the VM
    return vm_intern_string(string);
}

const ObjectString* string_concat(const ObjectString* a, const ObjectString* b)
{
    size_t length = a->length + b->length;

    // Concat a + b into buffer
    char* buffer = alloc_string(length);
    memcpy(buffer, a->chars, a->length);
    memcpy(buffer + a->length, b->chars, b->length);
    uint32_t hash = hash_string(buffer, length);

    return string_ctor(buffer, length, hash);
}

const ObjectString* string_multiply(const ObjectString* source, size_t n)
{
    size_t length = source->length * n;

    // Build buffer with new source * n
    char* buffer = alloc_string(length);
    for (size_t i = 0; i < n; ++i) {
        memcpy(buffer + i * source->length, source->chars, source->length);
    }
    uint32_t hash = hash_string(buffer, length);

    return string_ctor(buffer, length, hash);
}

bool string_equal(const ObjectString* a, const ObjectString* b)
{
    // Because all strings are deduplicated and interned (vm.string_pool),
    // simply compare pointers
    return a == b;
}

int string_compare(const ObjectString* a, const ObjectString* b)
{
    return strcmp(a->chars, b->chars);
}

// ObjectFunction
//------------------------------------------------------------------------------

ObjectFunction* function_new()
{
    ObjectFunction* function = object_new(OBJECT_FUNCTION, sizeof(ObjectFunction));
    function->arity = 0;
    function->name = NULL;
    chunk_init(&function->chunk);
    return function;
}

// ObjectArray
//------------------------------------------------------------------------------

ObjectArray* array_new()
{
    ObjectArray* self = object_new(OBJECT_ARRAY, sizeof(ObjectArray));
    value_array_init(&self->array);
    return self;
}
