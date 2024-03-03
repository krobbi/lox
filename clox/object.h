#ifndef clox_object_h
#define clox_object_h

#include "common.h"
#include "chunk.h"
#include "table.h"
#include "value.h"

// Get a value's object type.
#define OBJ_TYPE(value) (AS_OBJ(value)->type)

// Get whether a value is a class object.
#define IS_CLASS(value) isObjType(value, OBJ_CLASS)

// Get whether a value is a closure object.
#define IS_CLOSURE(value) isObjType(value, OBJ_CLOSURE)

// Get whether a value is a function object.
#define IS_FUNCTION(value) isObjType(value, OBJ_FUNCTION)

// Get whether a value is an instance object.
#define IS_INSTANCE(value) isObjType(value, OBJ_INSTANCE)

// Get whether a value is a native object.
#define IS_NATIVE(value) isObjType(value, OBJ_NATIVE)

// Get whether a value is a string object.
#define IS_STRING(value) isObjType(value, OBJ_STRING)

// Get a class value as a class object.
#define AS_CLASS(value) ((ObjClass*)AS_OBJ(value))

// Get a closure value as a closure object.
#define AS_CLOSURE(value) ((ObjClosure*)AS_OBJ(value))

// Get a function value as a function object.
#define AS_FUNCTION(value) ((ObjFunction*)AS_OBJ(value))

// Get an instance value as an instance object.
#define AS_INSTANCE(value) ((ObjInstance*)AS_OBJ(value))

// Get a native value as a native function pointer.
#define AS_NATIVE(value) (((ObjNative*)AS_OBJ(value))->function)

// Get a string value as a string object.
#define AS_STRING(value) ((ObjString*)AS_OBJ(value))

// Get a string value as a character pointer.
#define AS_CSTRING(value) (((ObjString*)AS_OBJ(value))->chars)

// An object's type.
typedef enum {
	// A class object's type.
	OBJ_CLASS,
	
	// A closure object's type.
	OBJ_CLOSURE,
	
	// A function object's type.
	OBJ_FUNCTION,
	
	// An instance object's type.
	OBJ_INSTANCE,
	
	// A native object's type.
	OBJ_NATIVE,
	
	// A string object's type.
	OBJ_STRING,
	
	// An upvalue object's type.
	OBJ_UPVALUE,
} ObjType;

struct Obj {
	// The object's type.
	ObjType type;
	
	// Whether the object is marked as reachable.
	bool isMarked;
	
	// The pointer to the next object for garbage collection.
	struct Obj *next;
};

// A function heap object.
typedef struct {
	// The function's parent object.
	Obj obj;
	
	// The function's number of parameters.
	int arity;
	
	// The number of upvalues used by the function.
	int upvalueCount;
	
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

// An upvalue heap object.
typedef struct ObjUpvalue {
	// The upvalue's parent object.
	Obj obj;
	
	// The pointer to the upvalue's value.
	Value *location;
	
	// The upvalue's closed value.
	Value closed;
	
	// The pointer to the next upvalue on the stack.
	struct ObjUpvalue *next;
} ObjUpvalue;

// A closure heap object.
typedef struct {
	// The closure's parent object.
	Obj obj;
	
	// The closure's function.
	ObjFunction *function;
	
	// The closure's upvalues.
	ObjUpvalue **upvalues;
	
	// The number of upvalues used by the closure's function.
	int upvalueCount;
} ObjClosure;

// A class heap object.
typedef struct {
	// The class' parent object.
	Obj obj;
	
	// The class' name.
	ObjString *name;
} ObjClass;

// An instance heap object.
typedef struct {
	// The instance's parent object.
	Obj obj;
	
	// The instance's class.
	ObjClass *klass;
	
	// The instance's fields.
	Table fields;
} ObjInstance;

// Make a new class object.
ObjClass *newClass(ObjString *name);

// Make a new closure object.
ObjClosure *newClosure(ObjFunction *function);

// Make a new function object.
ObjFunction *newFunction();

// Make a new instance object.
ObjInstance *newInstance(ObjClass *klass);

// Make a new native object.
ObjNative *newNative(NativeFn function);

// Get a string object from an owned string.
ObjString *takeString(char *chars, int length);

// Get a string object from a copied slice of a string.
ObjString *copyString(const char *chars, int length);

// Make a new upvalue object.
ObjUpvalue *newUpvalue(Value *slot);

// Print an object value.
void printObject(Value value);

// Get whether a value is an object with an object type.
static inline bool isObjType(Value value, ObjType type) {
	return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif // !clox_object_h
