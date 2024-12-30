#ifndef CLOX_INCLUDE_CHUNK_H
#define CLOX_INCLUDE_CHUNK_H

#include "common.h"
#include "value.h"

// Codes for different types of operations
typedef enum OpCode {
    // Constant
    OP_CONSTANT,
    // Literals
    OP_NIL,
    OP_TRUE,
    OP_FALSE,
    // Boolean
    OP_EQUAL,
    OP_GREATER,
    OP_LESS,
    // Arithmetic
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    // Unary
    OP_NOT,
    OP_NEGATE,
    // Return
    OP_RETURN,
} OpCode;

// Dynamic array
typedef struct {
    int count;
    int capacity;
    uint8_t* code;
    int* lines;
    ValueArray constants;
} Chunk;

/// @brief Initializes an empty chunk.
void initChunk(Chunk* chunk);
/// @brief Add a byte to a chunk.
void writeChunk(Chunk* chunk, uint8_t byte, int line);
/// @brief Frees the given chunk's memory.
void freeChunk(Chunk* chunk);

/// @brief Adds a constant value to the chunk's array of constants.
/// @returns Index of the constant.
int addConstant(Chunk* chunk, Value value);

#endif