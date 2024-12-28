#include <stdio.h>

#include "../include/common.h"
#include "../include/debug.h"
#include "../include/vm.h"

VM vm;

void initVM() {
}

void freeVM() {
}

#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])

/// @brief Executes instructions in a chunk.
static InterpretResult run() {
    while (true) {
#ifdef DEBUG_TRACE_EXECUTION
        disassembleInstruction(vm.chunk, /* (int) */(vm.ip - vm.chunk->code));
#endif
        uint8_t instruction;
        switch (instruction = READ_BYTE()) {
            case OP_CONSTANT:
                Value constant = READ_CONSTANT();
                printValue(constant);
                printf("\n");
                break;
            case OP_RETURN:
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