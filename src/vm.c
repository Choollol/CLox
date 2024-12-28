#include <stdio.h>

#include "../include/common.h"
#include "../include/debug.h"
#include "../include/vm.h"

VM vm;

/// @brief "Clear"s the stack by resetting the stackTop pointer.
static void resetStack() {
    vm.stackTop = vm.stack;
}

void initVM() {
    resetStack();
}

void freeVM() {
}

#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])

void printStack() {
    printf("          ");
    for (Value* slot = vm.stack; slot < vm.stackTop; ++slot) {
        printf("[ ");
        printValue(*slot);
        printf(" ]");
    }
    printf("\n");
}

/// @brief Executes instructions in a chunk.
static InterpretResult run() {
    while (true) {
#ifdef DEBUG_TRACE_EXECUTION
        printStack();
        disassembleInstruction(vm.chunk, /* (int) */(vm.ip - vm.chunk->code));
#endif
        uint8_t instruction;
        switch (instruction = READ_BYTE()) {
            case OP_CONSTANT:
                Value constant = READ_CONSTANT();
                push(constant);
                break;
            case OP_RETURN:
                printValue(pop());
                printf("\n");
                return INTERPRET_OK;
        }
    }
}

#undef READ_BYTE
#undef READ_CONSTANT

InterpretResult interpret(Chunk* chunk) {
    vm.chunk = chunk;
    vm.ip = chunk->code;
    return run();
}

void push(Value value) {
    *vm.stackTop = value;
    ++vm.stackTop;
}

Value pop() {
    return *--vm.stackTop;
}