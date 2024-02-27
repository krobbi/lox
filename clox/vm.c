#include <stdio.h>

#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "vm.h"

// The global virtual machine instance.
VM vm;

// Reset the stack.
static void resetStack() {
	vm.stackTop = vm.stack;
}

void initVM() {
	resetStack();
}

void freeVM() {}

// Read the next byte of bytecode.
#define READ_BYTE() (*vm.ip++)

// Read the next byte of bytecode as a constant.
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])

// Execute an instruction for a numeric binary operator.
#define BINARY_OP(op) \
	do { \
		double b = pop(); \
		double a = pop(); \
		push(a op b); \
	} while (false)

// Run the virtual machine's bytecode.
static InterpretResult run() {
	for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
		printf("          ");
		
		for (Value *slot = vm.stack; slot < vm.stackTop; slot++) {
			printf("[ ");
			printValue(*slot);
			printf(" ]");
		}
		
		printf("\n");
		disassembleInstruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
#endif // DEBUG_TRACE_EXECUTION
		
		uint8_t instruction;
		
		switch (instruction = READ_BYTE()) {
			case OP_CONSTANT: {
				Value constant = READ_CONSTANT();
				push(constant);
				break;
			}
			
			case OP_ADD: {
				BINARY_OP(+);
				break;
			}
			
			case OP_SUBTRACT: {
				BINARY_OP(-);
				break;
			}
			
			case OP_MULTIPLY: {
				BINARY_OP(*);
				break;
			}
			
			case OP_DIVIDE: {
				BINARY_OP(/);
				break;
			}
			
			case OP_NEGATE: {
				push(-pop());
				break;
			}
			
			case OP_RETURN: {
				printValue(pop());
				printf("\n");
				return INTERPRET_OK;
			}
		}
	}
}

#undef READ_BYTE
#undef READ_CONSTANT
#undef BINARY_OP

InterpretResult interpret(const char *source) {
	Chunk chunk;
	initChunk(&chunk);
	
	if (!compile(source, &chunk)) {
		freeChunk(&chunk);
		return INTERPRET_COMPILE_ERROR;
	}
	
	vm.chunk = &chunk;
	vm.ip = vm.chunk->code;
	
	InterpretResult result = run();
	
	freeChunk(&chunk);
	return result;
}

void push(Value value) {
	*vm.stackTop = value;
	vm.stackTop++;
}

Value pop() {
	vm.stackTop--;
	return *vm.stackTop;
}
