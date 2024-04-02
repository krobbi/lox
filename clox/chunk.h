#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include "value.h"

// An 8-bit constant index.
typedef uint8_t ConstantIndex;

// A bytecode instruction's type.
typedef enum {
	// Push a constant value to the stack from its index.
	OP_CONSTANT,
	
	// Push a nil value to the stack.
	OP_NIL,
	
	// Push a true boolean value to the stack.
	OP_TRUE,
	
	// Push a false boolean value to the stack.
	OP_FALSE,
	
	// Pop the top value from the stack.
	OP_POP,
	
	// Push a local to the stack from a stack slot.
	OP_GET_LOCAL,
	
	// Peek the top value of the stack and set a local from a stack slot.
	OP_SET_LOCAL,
	
	// Push a global to the stack from a constant.
	OP_GET_GLOBAL,
	
	// Pop and define the top value of the stack as a global from a constant.
	OP_DEFINE_GLOBAL,
	
	// Peek the top value of the stack and set a global from a constant.
	OP_SET_GLOBAL,
	
	// Push an upvalue to the stack from an upvalue slot.
	OP_GET_UPVALUE,
	
	// Peek the top value of the stack and set an upvalue from an upvalue slot.
	OP_SET_UPVALUE,
	
	// Replace the top instance value of the stack with a field from a constant.
	OP_GET_PROPERTY,
	
	// Set a field on the second top instance value of the stack to the top.
	OP_SET_PROPERTY,
	
	// Leave a super method value on the stack from an instance and superclass.
	OP_GET_SUPER,
	
	// Compare the second top value of the stack as equal to the top value.
	OP_EQUAL,
	
	// Compare the second top value of the stack as greater than the top value.
	OP_GREATER,
	
	// Compare the second top value of the stack as less than the top value.
	OP_LESS,
	
	// Add the top value of the stack to the second top value.
	OP_ADD,
	
	// Subtract the top value of the stack from the second top value.
	OP_SUBTRACT,
	
	// Multiply the second top value of the stack by the top value.
	OP_MULTIPLY,
	
	// Divide the second top value of the stack by the top value.
	OP_DIVIDE,
	
	// Logically negate the top value of the stack.
	OP_NOT,
	
	// Arithmetically negate the top value of the stack.
	OP_NEGATE,
	
	// Pop and print the top value from the stack.
	OP_PRINT,
	
	// Jump forwards.
	OP_JUMP,
	
	// Peek the top value of the stack and jump forwards if it is falsey.
	OP_JUMP_IF_FALSE,
	
	// Jump backwards.
	OP_LOOP,
	
	// Call an argument list with a number of arguments.
	OP_CALL,
	
	// Invoke a method call with a name constant and number of arguments.
	OP_INVOKE,
	
	// Invoke a super method with a name constant and number of arguments.
	OP_SUPER_INVOKE,
	
	// Push a closure value to the stack from a function constant and upvalues.
	OP_CLOSURE,
	
	// Pop the top value from the stack and close it in an upvalue.
	OP_CLOSE_UPVALUE,
	
	// Return from the current function.
	OP_RETURN,
	
	// Push a class value to the stack from a name constant.
	OP_CLASS,
	
	// Inherit the top class of the stack from the second top class.
	OP_INHERIT,
	
	// Bind the top method of the stack to the second top class.
	OP_METHOD,
} OpCode;

// A chunk of bytecode for a script.
typedef struct {
	// The number of bytes in the chunk's bytecode.
	int count;
	
	// The current maximum number of bytes in the chunk's bytecode.
	int capacity;
	
	// The chunk's bytecode.
	uint8_t *code;
	
	// The chunk's lines at each bytecode offset.
	int *lines;
	
	// The chunk's constant values.
	ValueArray constants;
} Chunk;

// Initialize a chunk.
void initChunk(Chunk *chunk);

// Free a chunk.
void freeChunk(Chunk *chunk);

// Write a byte of bytecode to a chunk.
void writeChunk(Chunk *chunk, uint8_t byte, int line);

// Add a new constant value to a chunk and return its index.
int addConstant(Chunk *chunk, Value value);

#endif // !clox_chunk_h
