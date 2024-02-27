#ifndef clox_value_h
#define clox_value_h

#include "common.h"

// A dynamically-typed value.
typedef double Value;

// A dynamic array of values.
typedef struct {
	// The current maximum number of values in the value array.
	int capacity;
	
	// The number of values in the value array.
	int count;
	
	// The value array's values.
	Value *values;
} ValueArray;

// Initialize a value array.
void initValueArray(ValueArray *array);

// Write a value to a value array.
void writeValueArray(ValueArray *array, Value value);

// Free a value array.
void freeValueArray(ValueArray *array);

// Print a value.
void printValue(Value value);

#endif // !clox_value_h
