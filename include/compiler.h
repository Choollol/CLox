#ifndef CLOX_INCLUDE_COMPILER_H
#define CLOX_INCLUDE_COMPILER_H

#include "chunk.h"
#include "object.h"

/// @brief Compiles source code.
/// @returns A function that contains the top-level code. If a compile-time error occurred, returns NULL.
ObjFunction* compile(const char* source);

#endif