#ifndef CLOX_INCLUDE_CHUNK_H
#define CLOX_INCLUDE_CHUNK_H

#include "common.h"
#include "value.h"

// Codes for different types of operations
typedef enum OpCode {
    OP_CONSTANT,

    OP_NIL,
    OP_TRUE,
    OP_FALSE,

    OP_POP,

    OP_DEFINE_GLOBAL,
    OP_GET_LOCAL,
    OP_SET_LOCAL,
    OP_GET_GLOBAL,
    OP_SET_GLOBAL,
    OP_GET_UPVALUE,
    OP_SET_UPVALUE,
    OP_GET_PROPERTY,
    OP_SET_PROPERTY,
    OP_GET_SUPER,

    OP_EQUAL,
    OP_GREATER,
    OP_LESS,

    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,

    OP_NOT,
    OP_NEGATE,

    OP_PRINT,
    OP_JUMP,
    OP_JUMP_IF_FALSE,

    OP_LOOP,

    OP_CALL,
    OP_INVOKE,
    OP_SUPER_INVOKE,
    OP_CLOSURE,
    OP_CLOSE_UPVALUE,

    OP_RETURN,

    OP_CLASS,
    OP_INHERIT,
    OP_METHOD,
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