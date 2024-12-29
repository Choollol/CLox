#ifndef CLOX_INCLUDE_COMPILER_H
#define CLOX_INCLUDE_COMPILER_H

#include "chunk.h"

/// @brief Compiles source code.
/// @returns Bool reporting the compilation's success status.
bool compile(const char* source, Chunk* chunk);

#endif