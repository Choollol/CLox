#ifndef CLOX_INCLUDE_VM_H
#define CLOX_INCLUDE_VM_H

#include "chunk.h"

typedef struct {
    Chunk* chunk;
    uint8_t* ip;
} VM;

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR,
} InterpretResult;

/// @brief Initialize the global, static virtual machine.
void initVM();
/// @brief Free the global, static virtual machine.
void freeVM();

/// @brief Interprets a chunk of bytecode.
InterpretResult interpret(Chunk* chunk);

#endif