#ifndef clox_vm_h
#define clox_vm_h

#include "chunk.h"
#include "object.h"
#include "table.h"
#include "value.h"

// The maximum function call depth.
#define FRAMES_MAX 64

// The maximum size of the stack in values.
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)

// A function call's state.
typedef struct {
	// The called closure.
	ObjClosure *closure;
	
	// The pointer to the next byte of bytecode in the function.
	uint8_t *ip;
	
	// The pointer to the call frame's locals.
	Value *slots;
} CallFrame;

// A virtual machine for interpreting bytecode.
typedef struct {
	// The stack of function call frames.
	CallFrame frames[FRAMES_MAX];
	
	// The current function call depth.
	int frameCount;
	
	// The stack of local values.
	Value stack[STACK_MAX];
	
	// The pointer to the next top value of the stack.
	Value *stackTop;
	
	// The table of globals.
	Table globals;
	
	// The set of interned strings.
	Table strings;
	
	// The pointer to the top open upvalue on the stack.
	ObjUpvalue *openUpvalues;
	
	// The pointer to the first garbage collected object.
	Obj *objects;
} VM;

// A result of interpreting bytecode.
typedef enum {
	// No errors occurred.
	INTERPRET_OK,
	
	// An error occurred during compilation.
	INTERPRET_COMPILE_ERROR,
	
	// An error occurred at runtime.
	INTERPRET_RUNTIME_ERROR,
} InterpretResult;

// The global virtual machine instance.
extern VM vm;

// Initialize the virtual machine.
void initVM();

// Free the virtual machine.
void freeVM();

// Interpret source code.
InterpretResult interpret(const char *source);

// Push a value to the stack.
void push(Value value);

// Pop a value from the stack.
Value pop();

#endif // !clox_vm_h
