#ifndef CLOX_INCLUDE_VM_H
#define CLOX_INCLUDE_VM_H

#include "chunk.h"
#include "value.h"
#include "table.h"
#include "object.h"

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)

typedef struct {
    ObjClosure* closure;
    uint8_t* ip;
    Value* slots;
} CallFrame;

typedef struct {
    CallFrame frames[FRAMES_MAX];
    int frameCount;

    Value stack[STACK_MAX];
    Value* stackTop;
    Table globals;
    Table strings;
    ObjString* initString;
    ObjUpvalue* openUpvalues;

    size_t bytesAllocated;
    size_t nextGC;
    Obj* objects;

    int grayCount;
    int grayCapacity;
    Obj** grayStack;
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