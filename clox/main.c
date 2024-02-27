#include "common.h"
#include "chunk.h"
#include "debug.h"

// Assemble and disassemble a test chunk.
int main(int argc, const char *argv[]) {
	(void)argc; // Unused parameter.
	(void)argv; // Unused parameter.
	
	Chunk chunk;
	initChunk(&chunk);
	
	int constant = addConstant(&chunk, 1.2);
	writeChunk(&chunk, OP_CONSTANT, 123);
	writeChunk(&chunk, constant, 123);
	
	writeChunk(&chunk, OP_RETURN, 123);
	
	disassembleChunk(&chunk, "test chunk");
	freeChunk(&chunk);
	return 0;
}
