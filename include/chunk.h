#ifndef CLOX_INCLUDE_CHUNK_H
#define CLOX_INCLUDE_CHUNK_H

#include "common.h"

// Codes for different types of operations
typedef enum {
    OP_RETURN,
} OpCode;

// Dynamic array
typedef struct {
    int count;
    int capacity;
    uint8_t* code;
} Chunk;

/// @brief Initializes an empty chunk.
void initChunk(Chunk* chunk);
/// @brief Frees the given chunk's memory.
void freeChunk(Chunk* chunk);
/// @brief Add a byte to a chunk.
void writeChunk(Chunk* chunk, uint8_t byte);

#endif