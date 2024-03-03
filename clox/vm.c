#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "object.h"
#include "memory.h"
#include "vm.h"

VM vm;

// The native clock function.
static Value clockNative(int argCount, Value *args) {
	(void)argCount; // Unused parameter.
	(void)args; // Unused parameter.
	
	return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}

// Reset the stack.
static void resetStack() {
	vm.stackTop = vm.stack;
	vm.frameCount = 0;
	vm.openUpvalues = NULL;
}

// Log a runtime error.
static void runtimeError(const char *format, ...) {
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fputs("\n", stderr);
	
	for (int i = vm.frameCount - 1; i >= 0; i--) {
		CallFrame *frame = &vm.frames[i];
		ObjFunction *function = frame->closure->function;
		size_t instruction = frame->ip - function->chunk.code - 1;
		fprintf(stderr, "[line %d] in ", function->chunk.lines[instruction]);
		
		if (function->name == NULL) {
			fprintf(stderr, "script\n");
		} else {
			fprintf(stderr, "%s()\n", function->name->chars);
		}
	}
	
	resetStack();
}

// Define a new native from a name and function pointer.
static void defineNative(const char *name, NativeFn function) {
	push(OBJ_VAL(copyString(name, (int)strlen(name))));
	push(OBJ_VAL(newNative(function)));
	tableSet(&vm.globals, AS_STRING(vm.stack[0]), vm.stack[1]);
	pop();
	pop();
}

void initVM() {
	resetStack();
	vm.objects = NULL;
	vm.bytesAllocated = 0;
	vm.nextGC = 1024 * 1024;
	
	vm.grayCount = 0;
	vm.grayCapacity = 0;
	vm.grayStack = NULL;
	
	initTable(&vm.globals);
	initTable(&vm.strings);
	
	defineNative("clock", clockNative);
}

void freeVM() {
	freeTable(&vm.globals);
	freeTable(&vm.strings);
	freeObjects();
}

void push(Value value) {
	*vm.stackTop = value;
	vm.stackTop++;
}

Value pop() {
	vm.stackTop--;
	return *vm.stackTop;
}

// Get the value at a distance from the top of the stack.
static Value peek(int distance) {
	return vm.stackTop[-1 - distance];
}

// Call a function with an argument count and return whether the call is valid.
static bool call(ObjClosure *closure, int argCount) {
	if (argCount != closure->function->arity) {
		runtimeError("Expected %d arguments but got %d.", closure->function->arity, argCount);
		return false;
	}
	
	if (vm.frameCount == FRAMES_MAX) {
		runtimeError("Stack overflow.");
		return false;
	}
	
	CallFrame *frame = &vm.frames[vm.frameCount++];
	frame->closure = closure;
	frame->ip = closure->function->chunk.code;
	frame->slots = vm.stackTop - argCount - 1;
	return true;
}

// Call a value with an argument count and return whether the call is valid.
static bool callValue(Value callee, int argCount) {
	if (IS_OBJ(callee)) {
		switch (OBJ_TYPE(callee)) {
			case OBJ_CLASS: {
				ObjClass *klass = AS_CLASS(callee);
				vm.stackTop[-argCount - 1] = OBJ_VAL(newInstance(klass));
				return true;
			}
			
			case OBJ_CLOSURE: {
					return call(AS_CLOSURE(callee), argCount);
			}
			
			case OBJ_NATIVE: {
				NativeFn native = AS_NATIVE(callee);
				Value result = native(argCount, vm.stackTop - argCount);
				vm.stackTop -= argCount + 1;
				push(result);
				return true;
			}
			
			default: {
				break; // Not a callable object.
			}
		}
	}
	
	runtimeError("Can only call functions and classes.");
	return false;
}

// Capture an upvalue from a stack value.
static ObjUpvalue *captureUpvalue(Value *local) {
	ObjUpvalue *prevUpvalue = NULL;
	ObjUpvalue *upvalue = vm.openUpvalues;
	
	while (upvalue != NULL && upvalue->location > local) {
		prevUpvalue = upvalue;
		upvalue = upvalue->next;
	}
	
	if (upvalue != NULL && upvalue->location == local) {
		return upvalue; // Deduplicate upvalues.
	}
	
	ObjUpvalue *createdUpvalue = newUpvalue(local);
	createdUpvalue->next = upvalue;
	
	if (prevUpvalue == NULL) {
		vm.openUpvalues = createdUpvalue;
	} else {
		prevUpvalue->next = createdUpvalue;
	}
	
	return createdUpvalue;
}

// Close all open upvalues on the stack up to a stack slot.
static void closeUpvalues(Value *last) {
	while (vm.openUpvalues != NULL && vm.openUpvalues->location >= last) {
		ObjUpvalue *upvalue = vm.openUpvalues;
		upvalue->closed = *upvalue->location;
		upvalue->location = &upvalue->closed;
		vm.openUpvalues = upvalue->next;
	}
}

// Get whether a value is false in a boolean context.
static bool isFalsey(Value value) {
	// Using the macro expression from the book here causes the `!` operator to
	// fail for objects. For some reason this only happens when optimizations
	// are turned on. I never noticed this bug before, so I am not sure if I
	// made a mistake somewhere, or if it is a bug with optimizations in a newer
	// version of GCC. This code feels more efficient anyway.
	switch (value.type) {
		case VAL_BOOL: return !value.as.boolean;
		case VAL_NIL: return true;
		default: return false;
	}
}

// Append the top string of the stack to the second top string.
static void concatenate() {
	ObjString *b = AS_STRING(peek(0));
	ObjString *a = AS_STRING(peek(1));
	
	int length = a->length + b->length;
	char *chars = ALLOCATE(char, length + 1);
	memcpy(chars, a->chars, a->length);
	memcpy(chars + a->length, b->chars, b->length);
	chars[length] = '\0';
	
	ObjString *result = takeString(chars, length);
	pop();
	pop();
	push(OBJ_VAL(result));
}

// Read the next byte of bytecode.
#define READ_BYTE() (*frame->ip++)

// Read the next short of bytecode.
#define READ_SHORT() (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))

// Read the next byte of bytecode as a constant.
#define READ_CONSTANT() (frame->closure->function->chunk.constants.values[READ_BYTE()])

// Read the next byte of bytecode as a constant string object.
#define READ_STRING() AS_STRING(READ_CONSTANT())

// Execute an instruction for an arithmetic binary operator.
#define BINARY_OP(valueType, op) \
	do { \
		if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
			runtimeError("Operands must be numbers."); \
			return INTERPRET_RUNTIME_ERROR; \
		} \
		\
		double b = AS_NUMBER(pop()); \
		double a = AS_NUMBER(pop()); \
		push(valueType(a op b)); \
	} while (false)

// Run the virtual machine's bytecode.
static InterpretResult run() {
	CallFrame *frame = &vm.frames[vm.frameCount - 1];
	
	for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
		printf("          ");
		
		for (Value *slot = vm.stack; slot < vm.stackTop; slot++) {
			printf("[ ");
			printValue(*slot);
			printf(" ]");
		}
		
		printf("\n");
		disassembleInstruction(
				&frame->closure->function->chunk,
				(int)(frame->ip - frame->closure->function->chunk.code));
#endif // DEBUG_TRACE_EXECUTION
		
		uint8_t instruction;
		
		switch (instruction = READ_BYTE()) {
			case OP_CONSTANT: {
				Value constant = READ_CONSTANT();
				push(constant);
				break;
			}
			
			case OP_NIL: {
				push(NIL_VAL);
				break;
			}
			
			case OP_TRUE: {
				push(BOOL_VAL(true));
				break;
			}
			
			case OP_FALSE: {
				push(BOOL_VAL(false));
				break;
			}
			
			case OP_POP: {
				pop();
				break;
			}
			
			case OP_GET_LOCAL: {
				uint8_t slot = READ_BYTE();
				push(frame->slots[slot]);
				break;
			}
			
			case OP_SET_LOCAL: {
				uint8_t slot = READ_BYTE();
				frame->slots[slot] = peek(0);
				break;
			}
			
			case OP_GET_GLOBAL: {
				ObjString *name = READ_STRING();
				Value value;
				
				if (!tableGet(&vm.globals, name, &value)) {
					runtimeError("Undefined variable '%s'.", name->chars);
					return INTERPRET_RUNTIME_ERROR;
				}
				
				push(value);
				break;
			}
			
			case OP_DEFINE_GLOBAL: {
				ObjString *name = READ_STRING();
				tableSet(&vm.globals, name, peek(0));
				pop();
				break;
			}
			
			case OP_SET_GLOBAL: {
				ObjString *name = READ_STRING();
				
				if (tableSet(&vm.globals, name, peek(0))) {
					tableDelete(&vm.globals, name);
					runtimeError("Undefined variable '%s'.", name->chars);
					return INTERPRET_RUNTIME_ERROR;
				}
				
				break;
			}
			
			case OP_GET_UPVALUE: {
				uint8_t slot = READ_BYTE();
				push(*frame->closure->upvalues[slot]->location);
				break;
			}
			
			case OP_SET_UPVALUE: {
				uint8_t slot = READ_BYTE();
				*frame->closure->upvalues[slot]->location = peek(0);
				break;
			}
			
			case OP_GET_PROPERTY: {
				if (!IS_INSTANCE(peek(0))) {
					runtimeError("Only instances have properties.");
					return INTERPRET_RUNTIME_ERROR;
				}
				
				ObjInstance *instance = AS_INSTANCE(peek(0));
				ObjString *name = READ_STRING();
				
				Value value;
				
				if (tableGet(&instance->fields, name, &value)) {
					pop();
					push(value);
					break;
				}
				
				runtimeError("Undefined property '%s'.", name->chars);
				return INTERPRET_RUNTIME_ERROR;
			}
			
			case OP_SET_PROPERTY: {
				if (!IS_INSTANCE(peek(1))) {
					runtimeError("Only instances have fields.");
					return INTERPRET_RUNTIME_ERROR;
				}
				
				ObjInstance *instance = AS_INSTANCE(peek(1));
				tableSet(&instance->fields, READ_STRING(), peek(0));
				Value value = pop();
				pop();
				push(value);
				break;
			}
			
			case OP_EQUAL: {
				Value b = pop();
				Value a = pop();
				push(BOOL_VAL(valuesEqual(a, b)));
				break;
			}
			
			case OP_GREATER: {
				BINARY_OP(BOOL_VAL, >);
				break;
			}
			
			case OP_LESS: {
				BINARY_OP(BOOL_VAL, <);
				break;
			}
			
			case OP_ADD: {
				if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
					concatenate();
				} else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
					double b = AS_NUMBER(pop());
					double a = AS_NUMBER(pop());
					push(NUMBER_VAL(a + b));
				} else {
					runtimeError("Operands must be two numbers or two strings.");
					return INTERPRET_RUNTIME_ERROR;
				}
				
				break;
			}
			
			case OP_SUBTRACT: {
				BINARY_OP(NUMBER_VAL, -);
				break;
			}
			
			case OP_MULTIPLY: {
				BINARY_OP(NUMBER_VAL, *);
				break;
			}
			
			case OP_DIVIDE: {
				BINARY_OP(NUMBER_VAL, /);
				break;
			}
			
			case OP_NOT: {
				push(BOOL_VAL(isFalsey(pop())));
				break;
			}
			
			case OP_NEGATE: {
				if (!IS_NUMBER(peek(0))) {
					runtimeError("Operand must be a number.");
					return INTERPRET_RUNTIME_ERROR;
				}
				
				push(NUMBER_VAL(-AS_NUMBER(pop())));
				break;
			}
			
			case OP_PRINT: {
				printValue(pop());
				printf("\n");
				break;
			}
			
			case OP_JUMP: {
				uint16_t offset = READ_SHORT();
				frame->ip += offset;
				break;
			}
			
			case OP_JUMP_IF_FALSE: {
				uint16_t offset = READ_SHORT();
				
				if (isFalsey(peek(0))) {
					frame->ip += offset;
				}
				
				break;
			}
			
			case OP_LOOP: {
				uint16_t offset = READ_SHORT();
				frame->ip -= offset;
				break;
			}
			
			case OP_CALL: {
				int argCount = READ_BYTE();
				
				if (!callValue(peek(argCount), argCount)) {
					return INTERPRET_RUNTIME_ERROR;
				}
				
				frame = &vm.frames[vm.frameCount - 1];
				break;
			}
			
			case OP_CLOSURE: {
				ObjFunction *function = AS_FUNCTION(READ_CONSTANT());
				ObjClosure *closure = newClosure(function);
				push(OBJ_VAL(closure));
				
				for (int i = 0; i < closure->upvalueCount; i++) {
					uint8_t isLocal = READ_BYTE();
					uint8_t index = READ_BYTE();
					
					if (isLocal) {
						closure->upvalues[i] = captureUpvalue(frame->slots + index);
					} else {
						closure->upvalues[i] = frame->closure->upvalues[index];
					}
				}
				
				break;
			}
			
			case OP_CLOSE_UPVALUE: {
				closeUpvalues(vm.stackTop - 1);
				pop();
				break;
			}
			
			case OP_RETURN: {
				Value result = pop();
				closeUpvalues(frame->slots); // Close parameter upvalues.
				vm.frameCount--;
				
				if (vm.frameCount == 0) {
					pop();
					return INTERPRET_OK;
				}
				
				vm.stackTop = frame->slots;
				push(result);
				frame = &vm.frames[vm.frameCount - 1];
				break;
			}
			
			case OP_CLASS: {
				push(OBJ_VAL(newClass(READ_STRING())));
				break;
			}
		}
	}
}

#undef READ_BYTE
#undef READ_SHORT
#undef READ_CONSTANT
#undef READ_STRING
#undef BINARY_OP

InterpretResult interpret(const char *source) {
	ObjFunction *function = compile(source);
	
	if (function == NULL) {
		return INTERPRET_COMPILE_ERROR;
	}
	
	push(OBJ_VAL(function));
	ObjClosure *closure = newClosure(function);
	pop();
	push(OBJ_VAL(closure));
	call(closure, 0);
	
	return run();
}
