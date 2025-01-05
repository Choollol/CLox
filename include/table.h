#ifndef CLOX_INCLUDE_TABLE_H
#define CLOX_INCLUDE_TABLE_H

#include "common.h"
#include "value.h"

typedef struct {
    ObjString* key;
    Value value;
} Entry;

typedef struct {
    int count;
    int capacity;
    Entry* entries;
} Table;

/// @brief Initializes a hash table.
void initTable(Table* table);
/// @brief Frees a hash table.
void freeTable(Table* table);
/// @brief Looks for the entry with the given key and puts it in the out value.
/// @returns Whether the entry was found.
bool tableGet(Table* table, ObjString* key, Value* out);
/// @brief Sets the given key's value.
/// @returns Whether a new key was added to the table.
bool tableSet(Table* table, ObjString* key, Value value);
/// @brief "Deletes" an entry from the table by replacing it with a tombstone.
/// @returns whether the entry was found and deleted.
bool tableDelete(Table* table, ObjString* key);
/// @brief Copies all entries from one hash table to another.
void tableAddAll(Table* from, Table* to);
/// @brief Finds an entry whose key contains the given string.
ObjString* tableFindString(Table* table, const char* chars, int length, uint32_t hash);

/// @brief Deletes unmarked entries from the given table.
void tableRemoveWhite(Table* table);
/// @brief GC-Marks all keys and values in a table as reachable.
void markTable(Table* table);

/// @brief Debug prints a table.
void printTable(Table* table);

#endif