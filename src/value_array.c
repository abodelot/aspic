#include "value_array.h"
#include "utils.h"

void value_array_init(ValueArray* self)
{
    self->capacity = self->count = 0;
    self->values = NULL;
}

void value_array_free(ValueArray* self)
{
    if (self->capacity > 0) {
        free(self->values);
    }

    self->capacity = self->count = 0;
    self->values = NULL;
}

void value_array_push(ValueArray* self, Value value)
{
    if (self->capacity < self->count + 1) {
        self->capacity = self->capacity < 8 ? 8 : self->capacity * 2;
        self->values = xrealloc(self->values, sizeof(Value), self->capacity);
    }
    self->values[self->count++] = value;
}

int value_array_find(const ValueArray* self, Value value)
{
    for (int i = 0; i < self->count; ++i) {
        if (value_equal(self->values[i], value)) {
            return i;
        }
    }
    return -1;
}
