#ifndef clox_object_h
#define clox_object_h

#include "common.h"
#include "value.h"

// Get a value's object type.
#define OBJ_TYPE(value) (AS_OBJ(value)->type)

// Get whether a value is a string object.
#define IS_STRING(value) isObjType(value, OBJ_STRING)

// Get a string value as a string object.
#define AS_STRING(value) ((ObjString*)AS_OBJ(value))

// Get a string value as a character pointer.
#define AS_CSTRING(value) (((ObjString*)AS_OBJ(value))->chars)

// An object's type.
typedef enum {
	// A string object's type.
	OBJ_STRING,
} ObjType;

struct Obj {
	// The object's type.
	ObjType type;
	
	// The pointer to the next object for garbage collection.
	struct Obj *next;
};

struct ObjString {
	// The string's parent object.
	Obj obj;
	
	// The string's length.
	int length;
	
	// The string's characters.
	char *chars;
};

// Make a new string object from an owned string.
ObjString *takeString(char *chars, int length);

// Make a new string object from a copied slice of a string.
ObjString *copyString(const char *chars, int length);

// Print an object value.
void printObject(Value value);

// Get whether a value is an object with an object type.
static inline bool isObjType(Value value, ObjType type) {
	return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif // !clox_object_h
