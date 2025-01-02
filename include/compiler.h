#ifndef CLOX_INCLUDE_COMPILER_H
#define CLOX_INCLUDE_COMPILER_H

#include "chunk.h"

/// @brief Compiles source code.
/// @returns The compiler's current function. If a compile-time error occurred, returns NULL.
ObjFunction* compile(const char* source);

#endif