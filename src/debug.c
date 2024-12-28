#include <stdio.h>

#include "../include/debug.h"
#include "../include/value.h"


void disassembleChunk(Chunk* chunk, const char* name) {
    printf("== %s ==\n", name);

    for (int offset = 0; offset < chunk->count;) {
        // Instructions have different sizes
        offset = disassembleInstruction(chunk, offset);
    }
}

int disassembleInstruction(Chunk* chunk, int offset) {
    printf("%04d ", offset);

    // Same line as previous instruction
    if (offset > 0 && chunk->lines[offset] == chunk->lines[offset - 1]) {
        printf("   | ");
    }
    // Different line from previous instruction
    else {
        printf("%4d ", chunk->lines[offset]);
    }

    uint8_t instruction = chunk->code[offset];
    switch (instruction) {
        case OP_CONSTANT:
            return constantInstruction("OP_CONSTANT", chunk, offset);
        case OP_RETURN:
            return simpleInstruction("OP_RETURN", offset);
        default:
            printf("Unknown opcode %d", instruction);
            return offset + 1;
    }
}

int constantInstruction(const char* name, Chunk* chunk, int offset) {
    int constantIndex = chunk->code[offset];
    printf("%-16s %4d '", name, constantIndex);
    printValue(chunk->constants.values[constantIndex]);
    printf("'\n");
    return offset + 2;
}
int simpleInstruction(const char* name, int offset) {
    printf("%s\n", name);
    return offset + 1;
}