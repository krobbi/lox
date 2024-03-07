#include <string.h>

#include "extension.h"
#include "memory.h"

// The number of command line arguments available to the user.
static int userArgc = 0;

// The command line arguments available to the user.
static const char **userArgv = NULL;

// Define a native extension function as having 0 parameters.
#define PARAMS_0() \
	do { \
		(void)args; \
		\
		if (argCount != 0) { \
			return NIL_VAL; \
		} \
	} while (false)

// Define a native extension function as having 1 parameter.
#define PARAMS_1(p1) \
	do { \
		if (argCount != 1 || !(p1(args[0]))) { \
			return NIL_VAL; \
		} \
	} while (false)

// Define a native extension function as having 2 parameters.
#define PARAMS_2(p1, p2) \
	do { \
		if (argCount != 2 || !(p1(args[0])) || !(p2(args[1]))) { \
			return NIL_VAL; \
		} \
	} while (false)

// The native argc extension function.
static Value argcExtension(int argCount, Value *args) {
	PARAMS_0();
	return NUMBER_VAL((double)userArgc);
}

// The native argv extension function.
static Value argvExtension(int argCount, Value *args) {
	PARAMS_1(IS_NUMBER);
	int index = (int)AS_NUMBER(args[0]);
	
	if (index < 0 || index >= userArgc) {
		return NIL_VAL; // Index out of bounds.
	}
	
	const char *chars = userArgv[index];
	int length = (int)strlen(chars);
	return OBJ_VAL(copyString(chars, length));
}

// The native chrat extension function.
static Value chratExtension(int argCount, Value *args) {
	PARAMS_2(IS_STRING, IS_NUMBER);
	ObjString *text = AS_STRING(args[0]);
	int index = (int)AS_NUMBER(args[1]);
	
	if (index < 0 || index >= text->length) {
		return NIL_VAL; // Index out of bounds.
	}
	
	int byte = (int)(unsigned char)text->chars[index];
	return NUMBER_VAL((double)byte);
}

// The native strlen extension function.
static Value strlenExtension(int argCount, Value *args) {
	PARAMS_1(IS_STRING);
	int length = AS_STRING(args[0])->length;
	return NUMBER_VAL((double)length);
}

// The native strof extension function.
static Value strofExtension(int argCount, Value *args) {
	PARAMS_1(IS_NUMBER);
	int byte = (int)AS_NUMBER(args[0]);
	
	if (byte < 1 || byte > 255) {
		return NIL_VAL; // Byte out of character range.
	}
	
	char *chars = ALLOCATE(char, 2);
	chars[0] = (char)(unsigned char)byte;
	chars[1] = '\0';
	return OBJ_VAL(takeString(chars, 1));
}

#undef PARAMS_0
#undef PARAMS_1
#undef PARAMS_2

void initExtensions(int argc, const char *argv[]) {
	if (argc > 1) {
		userArgc = argc - 1;
		userArgv = &argv[1];
	}
}

void defineExtensions(DefineNativeFn defineNative) {
	defineNative("__argc", argcExtension);
	defineNative("__argv", argvExtension);
	defineNative("__chrat", chratExtension);
	defineNative("__strlen", strlenExtension);
	defineNative("__strof", strofExtension);
}
