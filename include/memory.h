#ifndef CLOX_INCLUDE_MEMORY_H
#define CLOX_INCLUDE_MEMORY_H

#include "common.h"

/// @brief Calculate new capacity based on old capacity.
#define GROW_CAPACITY(capacity) ((capacity < 8) ? 8 : capacity * 2)
/// @brief Resizes a dynamic array by delegating to reallocate().
#define GROW_ARRAY(type, pointer, oldCount, newCount) \
    ((type*)reallocate(pointer, sizeof(type) * oldCount, sizeof(type) * newCount))
/// @brief Frees a dynamic array by delegating to reallocate().
#define FREE_ARRAY(type, pointer, count) reallocate(pointer, sizeof(type) * count, 0)

/// @brief Used for all dynamic memory management including allocation, freeing, and resizing.
/// @param pointer Pointer to dynamic memory.
/// @param oldSize If 0, allocate a new block.
/// @param newSize If 0, free allocation. Otherwise, resize.
/// @return Pointer that points to the newly allocated block.
void* reallocate(void* pointer, size_t oldSize, size_t newSize);

#endif