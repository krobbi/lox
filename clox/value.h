#ifndef clox_value_h
#define clox_value_h

#include "common.h"

// A value's type.
typedef enum {
	// A boolean type.
	VAL_BOOL,
	
	// A nil type.
	VAL_NIL,
	
	// A floating point number type.
	VAL_NUMBER,
} ValueType;

// A dynamically-typed value.
typedef struct {
	// The value's type.
	ValueType type;
	
	// The value's data.
	union {
		// The value as a boolean.
		bool boolean;
		
		// The value as a floating point number.
		double number;
	} as;
} Value;

// Get whether a value is a boolean.
#define IS_BOOL(value) ((value).type == VAL_BOOL)

// Get whether a value is nil.
#define IS_NIL(value) ((value).type == VAL_NIL)

// Get whether a value is a number.
#define IS_NUMBER(value) ((value).type == VAL_NUMBER)

// Get a value's boolean.
#define AS_BOOL(value) ((value).as.boolean)

// Get a value's number.
#define AS_NUMBER(value) ((value).as.number)

// Make a new boolean value.
#define BOOL_VAL(value) ((Value){ VAL_BOOL, { .boolean = value } })

// Make a new nil value.
#define NIL_VAL ((Value){ VAL_NIL, { .number = 0 } })

// Make a new number value.
#define NUMBER_VAL(value) ((Value){ VAL_NUMBER, { .number = value } })

// A dynamic array of values.
typedef struct {
	// The current maximum number of values in the value array.
	int capacity;
	
	// The number of values in the value array.
	int count;
	
	// The value array's values.
	Value *values;
} ValueArray;

// Get whether two values are equal.
bool valuesEqual(Value a, Value b);

// Initialize a value array.
void initValueArray(ValueArray *array);

// Write a value to a value array.
void writeValueArray(ValueArray *array, Value value);

// Free a value array.
void freeValueArray(ValueArray *array);

// Print a value.
void printValue(Value value);

#endif // !clox_value_h
