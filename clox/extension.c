#include <string.h>

#include "extension.h"

// The number of command line arguments available to the user.
static int userArgc = 0;

// The command line arguments available to the user.
static const char **userArgv = NULL;

// The native argc extension function.
static Value argcExtension(int argCount, Value *args) {
	(void)args; // Unused parameter.
	
	if (argCount != 0) {
		return NIL_VAL; // Invalid arguments.
	}
	
	return NUMBER_VAL((double)userArgc);
}

// The native argv extension function.
static Value argvExtension(int argCount, Value *args) {
	if (argCount != 1 || !IS_NUMBER(args[0])) {
		return NIL_VAL; // Invalid arguments.
	}
	
	int index = (int)AS_NUMBER(args[0]);
	
	if (index < 0 || index >= userArgc) {
		return NIL_VAL; // Index out of bounds.
	}
	
	const char *chars = userArgv[index];
	int length = (int)strlen(chars);
	return OBJ_VAL(copyString(chars, length));
}

void initExtensions(int argc, const char *argv[]) {
	if (argc > 1) {
		userArgc = argc - 1;
		userArgv = &argv[1];
	}
}

void defineExtensions(DefineNativeFn defineNative) {
	defineNative("__argc", argcExtension);
	defineNative("__argv", argvExtension);
}
