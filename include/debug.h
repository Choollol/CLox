#ifndef CLOX_INCLUDE_DEBUG_H
#define CLOX_INCLUDE_DEBUG_H

#include "chunk.h"

/// @brief Outputs a readable representation of the instructions in given chunk.
void disassembleChunk(Chunk* chunk, const char* name);

/// @brief Outputs a readable representation of a single instruction.
/// @returns Offset of the next instruction.
int disassembleInstruction(Chunk* chunk, int offset);

/// @brief Outputs a representation of a constant.
int constantInstruction(const char* name, Chunk* chunk, int offset);
/// @brief Outputs a representation of an instruction whose size is 1 byte.
int simpleInstruction(const char* name, int offset);

#endif