#ifndef clox_object_h
#define clox_object_h

#include "common.h"
#include "chunk.h"
#include "value.h"

// Get a value's object type.
#define OBJ_TYPE(value) (AS_OBJ(value)->type)

// Get whether a value is a function object.
#define IS_FUNCTION(value) isObjType(value, OBJ_FUNCTION)

// Get whether a value is a native object.
#define IS_NATIVE(value) isObjType(value, OBJ_NATIVE)

// Get whether a value is a string object.
#define IS_STRING(value) isObjType(value, OBJ_STRING)

// Get a function value as a function object.
#define AS_FUNCTION(value) ((ObjFunction*)AS_OBJ(value))

// Get a native value as a native function pointer.
#define AS_NATIVE(value) (((ObjNative*)AS_OBJ(value))->function)

// Get a string value as a string object.
#define AS_STRING(value) ((ObjString*)AS_OBJ(value))

// Get a string value as a character pointer.
#define AS_CSTRING(value) (((ObjString*)AS_OBJ(value))->chars)

// An object's type.
typedef enum {
	// A function object's type.
	OBJ_FUNCTION,
	
	// A native object's type.
	OBJ_NATIVE,
	
	// A string object's type.
	OBJ_STRING,
} ObjType;

struct Obj {
	// The object's type.
	ObjType type;
	
	// The pointer to the next object for garbage collection.
	struct Obj *next;
};

// A function heap object.
typedef struct {
	// The function's parent object.
	Obj obj;
	
	// The function's number of parameters.
	int arity;
	
	// The function's bytecode chunk.
	Chunk chunk;
	
	// The function's name.
	ObjString *name;
} ObjFunction;

// An externally-defined function that can be called from Lox.
typedef Value (*NativeFn)(int argCount, Value *args);

// A native heap object.
typedef struct {
	// The native's parent object.
	Obj obj;
	
	// The native's function.
	NativeFn function;
} ObjNative;

struct ObjString {
	// The string's parent object.
	Obj obj;
	
	// The string's length.
	int length;
	
	// The string's characters.
	char *chars;
	
	// The string's hash.
	uint32_t hash;
};

// Make a new function object.
ObjFunction *newFunction();

// Make a new native object.
ObjNative *newNative(NativeFn function);

// Get a string object from an owned string.
ObjString *takeString(char *chars, int length);

// Get a string object from a copied slice of a string.
ObjString *copyString(const char *chars, int length);

// Print an object value.
void printObject(Value value);

// Get whether a value is an object with an object type.
static inline bool isObjType(Value value, ObjType type) {
	return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif // !clox_object_h
