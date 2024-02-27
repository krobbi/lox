#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include "value.h"

// A bytecode instruction's kind.
typedef enum {
	// Load a constant from its index.
	OP_CONSTANT,
	
	// Return from the current function.
	OP_RETURN,
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
