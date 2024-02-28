#ifndef clox_table_h
#define clox_table_h

#include "common.h"
#include "value.h"

// A key-value pair entry of a hash table.
typedef struct {
	// The entry's key.
	ObjString *key;
	
	// The entry's value.
	Value value;
} Entry;

// A hash table.
typedef struct {
	// The number of entries in the hash table.
	int count;
	
	// The current maximum number of entries in the hash table.
	int capacity;
	
	// The hash table's entries.
	Entry *entries;
} Table;

// Initialize a hash table.
void initTable(Table *table);

// Free a hash table.
void freeTable(Table *table);

// Get a value from a hash table in a pointer and return whether it exists.
bool tableGet(Table *table, ObjString *key, Value *value);

// Set an entry in a hash table and return whether it is new.
bool tableSet(Table *table, ObjString *key, Value value);

// Delete an entry in a hash table and return whether it existed.
bool tableDelete(Table *table, ObjString *key);

// Copy all entries from one hash table to another hash table.
void tableAddAll(Table *from, Table *to);

// Find a key in a hash table.
ObjString *tableFindString(Table *table, const char *chars, int length, uint32_t hash);

#endif // !clox_table_h
