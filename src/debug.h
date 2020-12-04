#ifndef ASPIC_DEBUG_H
#define ASPIC_DEBUG_H

#include "chunk.h"

void chunk_dump(const Chunk* chunk, const char* name);

int instruction_dump(const Chunk* chunk, int offset);

#endif
