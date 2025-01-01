#ifndef CLOX_INCLUDE_DEBUG_H
#define CLOX_INCLUDE_DEBUG_H

#include "chunk.h"

/// @brief Outputs a readable representation of the instructions in given chunk.
void disassembleChunk(Chunk* chunk, const char* name);

/// @brief Outputs a readable representation of a single instruction.
/// @returns Offset of the next instruction.
int disassembleInstruction(Chunk* chunk, int offset);

#endif