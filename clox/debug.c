#include <stdio.h>

#include "debug.h"
#include "object.h"
#include "value.h"

// An offset in a chunk.
typedef struct {
	// The cursor's chunk.
	Chunk *chunk;
	
	// The offset in the cursor's chunk.
	int offset;
} Cursor;

// Initialize a cursor.
static void initCursor(Cursor *cursor, Chunk *chunk, int offset) {
	cursor->chunk = chunk;
	cursor->offset = offset;
}

// Get a cursor's constant from a constant index.
static Value cursorGetConstant(Cursor *cursor, ConstantIndex constant) {
	return cursor->chunk->constants.values[constant];
}

// Get a cursor's current line.
static int cursorGetLine(Cursor *cursor) {
	return cursor->chunk->lines[cursor->offset];
}

// Fetch an 8-bit unsigned integer from a cursor.
static uint8_t cursorFetchU8(Cursor *cursor) {
	return cursor->chunk->code[cursor->offset++];
}

// Fetch a big-endian 16-bit unsigned integer from a cursor.
static uint16_t cursorFetchU16(Cursor *cursor) {
	uint8_t high = cursorFetchU8(cursor);
	uint8_t low = cursorFetchU8(cursor);
	return (uint16_t)((high << 8) | low);
}

// Fetch a constant index from a cursor.
static ConstantIndex cursorFetchConstant(Cursor *cursor) {
	return (ConstantIndex)cursorFetchU8(cursor);
}

// Disassemble an instruction with no operands.
static void simpleInstruction(const char *name) {
	printf("%s\n", name);
}

// Disassemble an instruction with a byte operand.
static void byteInstruction(const char *name, Cursor *cursor) {
	uint8_t operand = cursorFetchU8(cursor);
	printf("%-16s %4d\n", name, operand);
}

// Disassemble an instruction with a constant operand.
static void constantInstruction(const char *name, Cursor *cursor) {
	ConstantIndex operand = cursorFetchConstant(cursor);
	printf("%-16s %4d '", name, operand);
	printValue(cursorGetConstant(cursor, operand));
	printf("'\n");
}

// Disassemble an instruction with a jump operand.
static void jumpInstruction(const char *name, int sign, Cursor *cursor) {
	uint16_t operand = cursorFetchU16(cursor);
	printf("%-16s %4d -> %d\n", name, cursor->offset - 3, cursor->offset + sign * operand);
}

// Disassemble an invoke instruction.
static void invokeInstruction(const char *name, Cursor *cursor) {
	ConstantIndex constant = cursorFetchConstant(cursor);
	uint8_t argCount = cursorFetchU8(cursor);
	printf("%-16s (%d args) %4d '", name, argCount, constant);
	printValue(cursorGetConstant(cursor, constant));
	printf("'\n");
}

// Disassemble a closure instruction.
static void closureInstruction(const char *name, Cursor *cursor) {
	ConstantIndex constant = cursorFetchConstant(cursor);
	printf("%-16s %4d ", name, constant);
	printValue(cursorGetConstant(cursor, constant));
	printf("\n");
	
	ObjFunction *function = AS_FUNCTION(cursorGetConstant(cursor, constant));
	
	for (int i = 0; i < function->upvalueCount; i++) {
		uint8_t isLocal = cursorFetchU8(cursor);
		uint8_t index = cursorFetchU8(cursor);
		
		printf(
				"%04d      |                     %s %d\n",
				cursor->offset - 2, isLocal ? "local" : "upvalue", index);
	}
}

// Disassemble an instruction with a cursor.
static void disassemble(Cursor *cursor) {
	printf("%04d ", cursor->offset);
	
	if (cursor->offset > 0 && cursorGetLine(cursor) == cursor->chunk->lines[cursor->offset - 1]) {
		printf("   | ");
	} else {
		printf("%4d ", cursorGetLine(cursor));
	}
	
	uint8_t instruction = cursorFetchU8(cursor);
	
	switch (instruction) {
		case OP_CONSTANT: constantInstruction("OP_CONSTANT", cursor); break;
		case OP_NIL: simpleInstruction("OP_NIL"); break;
		case OP_TRUE: simpleInstruction("OP_TRUE"); break;
		case OP_FALSE: simpleInstruction("OP_FALSE"); break;
		case OP_POP: simpleInstruction("OP_POP"); break;
		case OP_GET_LOCAL: byteInstruction("OP_GET_LOCAL", cursor); break;
		case OP_SET_LOCAL: byteInstruction("OP_SET_LOCAL", cursor); break;
		case OP_GET_GLOBAL: constantInstruction("OP_GET_GLOBAL", cursor); break;
		case OP_DEFINE_GLOBAL: constantInstruction("OP_DEFINE_GLOBAL", cursor); break;
		case OP_SET_GLOBAL: constantInstruction("OP_SET_GLOBAL", cursor); break;
		case OP_GET_UPVALUE: byteInstruction("OP_GET_UPVALUE", cursor); break;
		case OP_SET_UPVALUE: byteInstruction("OP_SET_UPVALUE", cursor); break;
		case OP_GET_PROPERTY: constantInstruction("OP_GET_PROPERTY", cursor); break;
		case OP_SET_PROPERTY: constantInstruction("OP_SET_PROPERTY", cursor); break;
		case OP_GET_SUPER: constantInstruction("OP_GET_SUPER", cursor); break;
		case OP_EQUAL: simpleInstruction("OP_EQUAL"); break;
		case OP_GREATER: simpleInstruction("OP_GREATER"); break;
		case OP_LESS: simpleInstruction("OP_LESS"); break;
		case OP_ADD: simpleInstruction("OP_ADD"); break;
		case OP_SUBTRACT: simpleInstruction("OP_SUBTRACT"); break;
		case OP_MULTIPLY: simpleInstruction("OP_MULTIPLY"); break;
		case OP_DIVIDE: simpleInstruction("OP_DIVIDE"); break;
		case OP_NOT: simpleInstruction("OP_NOT"); break;
		case OP_NEGATE: simpleInstruction("OP_NEGATE"); break;
		case OP_PRINT: simpleInstruction("OP_PRINT"); break;
		case OP_JUMP: jumpInstruction("OP_JUMP", 1, cursor); break;
		case OP_JUMP_IF_FALSE: jumpInstruction("OP_JUMP_IF_FALSE", 1, cursor); break;
		case OP_LOOP: jumpInstruction("OP_LOOP", -1, cursor); break;
		case OP_CALL: byteInstruction("OP_CALL", cursor); break;
		case OP_INVOKE: invokeInstruction("OP_INVOKE", cursor); break;
		case OP_SUPER_INVOKE: invokeInstruction("OP_SUPER_INVOKE", cursor); break;
		case OP_CLOSURE: closureInstruction("OP_CLOSURE", cursor); break;
		case OP_CLOSE_UPVALUE: simpleInstruction("OP_CLOSE_UPVALUE"); break;
		case OP_RETURN: simpleInstruction("OP_RETURN"); break;
		case OP_CLASS: constantInstruction("OP_CLASS", cursor); break;
		case OP_INHERIT: simpleInstruction("OP_INHERIT"); break;
		case OP_METHOD: constantInstruction("OP_METHOD", cursor); break;
		default: printf("Unknown opcode '%d'.\n", instruction); break;
	}
}

void disassembleChunk(Chunk *chunk, const char *name) {
	Cursor cursor;
	initCursor(&cursor, chunk, 0);
	
	printf("== %s ==\n", name);
	
	while (cursor.offset < cursor.chunk->count) {
		disassemble(&cursor);
	}
}

void disassembleInstruction(Chunk *chunk, int offset) {
	Cursor cursor;
	initCursor(&cursor, chunk, offset);
	disassemble(&cursor);
}
