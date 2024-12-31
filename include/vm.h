#ifndef CLOX_INCLUDE_VM_H
#define CLOX_INCLUDE_VM_H

#include "chunk.h"
#include "value.h"
#include "table.h"

#define STACK_MAX 256

typedef struct {
    Chunk* chunk;
    uint8_t* ip;
    Value stack[STACK_MAX];
    Value* stackTop;
    Table strings;
    Obj* objects;
} VM;

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR,
} InterpretResult;

extern VM vm;

/// @brief Initialize the global virtual machine.
void initVM();
/// @brief Free the global virtual machine.
void freeVM();

/// @brief Interprets a string of source code.
InterpretResult interpret(const char* source);

/// @brief Pushes a value onto the VM's stack.
void push(Value value);
/// @brief Pops a value off of the VM's stack.
Value pop();

#endif