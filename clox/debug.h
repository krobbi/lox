#ifndef clox_debug_h
#define clox_debug_h

#include "chunk.h"

// Disassemble a chunk's instructions.
void disassembleChunk(Chunk *chunk, const char *name);

// Disassemble an instruction at an offset in a chunk.
int disassembleInstruction(Chunk *chunk, int offset);

#endif // !clox_debug_h
