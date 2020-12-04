#include "value_array.h"
#include "utils.h"

void value_array_init(ValueArray* varray)
{
    varray->capacity = varray->count = 0;
    varray->values = NULL;
}

void value_array_free(ValueArray* varray)
{
    if (varray->capacity > 0) {
        free(varray->values);
    }

    varray->capacity = varray->count = 0;
    varray->values = NULL;
}

void value_array_push(ValueArray* varray, Value value)
{
    if (varray->capacity < varray->count + 1) {
        varray->capacity = varray->capacity < 8 ? 8 : varray->capacity * 2;
        varray->values = xrealloc(varray->values, sizeof(Value), varray->capacity);
    }
    varray->values[varray->count++] = value;
}
