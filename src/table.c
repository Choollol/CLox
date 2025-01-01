#include <stdlib.h>
#include <string.h>

#include "../include/memory.h"
#include "../include/object.h"
#include "../include/table.h"
#include "../include/value.h"

#ifdef DEBUG_PRINT
#include <stdio.h>
#endif

/// @brief A hash table's maximum load factor.
#define TABLE_MAX_LOAD 0.75

void initTable(Table* table) {
    table->count = 0;
    table->capacity = 0;
    table->entries = NULL;
}

void freeTable(Table* table) {
    FREE_ARRAY(Entry, table->entries, table->capacity);
    initTable(table);
}

/// @returns If found, the entry with the given key. Otherwise, an entry whose key is NULL and is suitable for insertion.
static Entry* findEntry(Entry* entries, int capacity, ObjString* key) {
    uint32_t index = key->hash % capacity;
    Entry* tombstone = NULL;

    while (true) {
        Entry* entry = &entries[index];

        if (entry->key == NULL) {
            // Empty entry
            if (IS_NIL(entry->value)) {
                return tombstone != NULL ? tombstone : entry;
            }
            // Tombstone
            else {
                if (tombstone == NULL) {
                    tombstone = entry;
                }
            }
        }
        // Key found
        else if (entry->key == key) {
            return entry;
        }

        index = (index + 1) % capacity;
    }
}

bool tableGet(Table* table, ObjString* key, Value* out) {
    if (table->count == 0) {
        return false;
    }

    Entry* entry = findEntry(table->entries, table->capacity, key);
    if (entry->key == NULL) {
        return false;
    }
    *out = entry->value;
    return true;
}

/// @brief Resizes the given hash table to the new capacity.
static void adjustCapacity(Table* table, int capacity) {
    Entry* entries = ALLOCATE(Entry, capacity);
    for (int i = 0; i < capacity; ++i) {
        entries[i].key = NULL;
        entries[i].value = NIL_VAL;
    }

    table->count = 0;
    for (int i = 0; i < table->capacity; ++i) {
        Entry* entry = &table->entries[i];
        if (entry->key == NULL) {
            continue;
        }

        Entry* dest = findEntry(entries, capacity, entry->key);
        dest->key = entry->key;
        dest->value = entry->value;
        ++table->count;
    }
    FREE_ARRAY(Entry, table->entries, table->capacity);

    table->entries = entries;
    table->capacity = capacity;
}

bool tableSet(Table* table, ObjString* key, Value value) {
    if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
        int capacity = GROW_CAPACITY(table->capacity);
        adjustCapacity(table, capacity);
    }

    Entry* entry = findEntry(table->entries, table->capacity, key);
    bool isNewKey = entry->key == NULL;
    // Increment count only if a new key was added into a truly empty slot, not just a tombstone.
    if (isNewKey && IS_NIL(entry->value)) {
        ++table->count;
    }

    entry->key = key;
    entry->value = value;

    return isNewKey;
}

bool tableDelete(Table* table, ObjString* key) {
    if (table->count == 0) {
        return false;
    }

    Entry* entry = findEntry(table->entries, table->capacity, key);
    if (entry == NULL) {
        return false;
    }

    entry->key = NULL;
    entry->value = BOOL_VAL(true);
    return true;
}

void tableAddAll(Table* from, Table* to) {
    for (int i = 0; i < from->capacity; ++i) {
        Entry* entry = &from->entries[i];
        if (entry != NULL) {
            tableSet(to, entry->key, entry->value);
        }
    }
}

ObjString* tableFindString(Table* table, const char* chars, int length, uint32_t hash) {
    if (table->count == 0) {
        return NULL;
    }

    uint32_t index = hash % table->capacity;
    while (true) {
        Entry* entry = &table->entries[index];
        if (entry->key == NULL) {
            // Only return NULL if a truly empty slot was found, not just a tombstone.
            if (IS_NIL(entry->value)) {
                return NULL;
            }
        }
        else if (entry->key->length == length && entry->key->hash == hash && memcmp(entry->key->chars, chars, length) == 0) {
            return entry->key;
        }
        index = (index + 1) % table->capacity;
    }
}

void printTable(Table* table) {
    printf("====\n");

    for (int i = 0; i < table->capacity; ++i) {
        Entry* entry = &table->entries[i];
        if (entry->key == NULL) {
            printf("NULL");
        }
        else {
            printf("%s ", entry->key->chars);
            printValue(entry->value);
        }
        printf("\n");
    }

    printf("====\n");
}