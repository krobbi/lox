#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "extension.h"
#include "memory.h"

// The index of the user's standard input stream.
#define USER_STDIN 0

// The index of the user's standard output stream.
#define USER_STDOUT 1

// The index of the user's standard error stream.
#define USER_STDERR 2

// The minimum index of a user's file stream.
#define USER_FILE_MIN 3

// The maximum index of a user's file stream.
#define USER_FILE_MAX 7

// The buffer size for the native ftoa extension function.
#define FTOA_SIZE 64

// The number of command line arguments available to the user.
static int userArgc = 0;

// The command line arguments available to the user.
static const char **userArgv = NULL;

// The I/O streams available to the user.
static FILE *userStreams[] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

// Open a file from a path and mode.
static Value openFile(const char *path, const char *mode) {
	int index = USER_FILE_MIN;
	
	while (userStreams[index] != NULL) {
		index++;
		
		if (index > USER_FILE_MAX) {
			return NIL_VAL; // No available streams.
		}
	}
	
	FILE *file = fopen(path, mode);
	
	if (file == NULL) {
		return NIL_VAL; // Could not open file.
	}
	
	userStreams[index] = file;
	return NUMBER_VAL((double)index);
}

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

// The native exit extension function.
static Value exitExtension(int argCount, Value *args) {
	if (argCount != 1 || !IS_NUMBER(args[0])) {
		exit(70);
	}
	
	int status = (int)AS_NUMBER(args[0]);
	exit(status);
	return NIL_VAL; // Unreachable.
}

// The native fclose extension function.
static Value fcloseExtension(int argCount, Value *args) {
	PARAMS_1(IS_NUMBER);
	int index = (int)AS_NUMBER(args[0]);
	
	if (index < USER_FILE_MIN || index > USER_FILE_MAX) {
		return BOOL_VAL(false); // Not a file stream.
	}
	
	FILE *file = userStreams[index];
	
	if (file == NULL) {
		return BOOL_VAL(false); // File already closed.
	}
	
	int result = fclose(file);
	userStreams[index] = NULL;
	
	if (result == EOF) {
		return BOOL_VAL(false); // Could not close file.
	}
	
	return BOOL_VAL(true);
}

// The native fgetc extension function.
static Value fgetcExtension(int argCount, Value *args) {
	PARAMS_1(IS_NUMBER);
	int index = (int)AS_NUMBER(args[0]);
	
	if (index < 0 || index > USER_FILE_MAX) {
		return NIL_VAL; // Not a stream.
	}
	
	FILE *stream = userStreams[index];
	
	if (stream == NULL) {
		return NIL_VAL; // Stream not open.
	}
	
	int result = fgetc(stream);
	
	if (result == EOF) {
		return NIL_VAL; // Could not read from stream.
	}
	
	return NUMBER_VAL((double)result);
}

// The native fopenr extension function.
static Value fopenrExtension(int argCount, Value *args) {
	PARAMS_1(IS_STRING);
	const char *path = AS_CSTRING(args[0]);
	return openFile(path, "rb");
}

// The native fopenw extension function.
static Value fopenwExtension(int argCount, Value *args) {
	PARAMS_1(IS_STRING);
	const char *path = AS_CSTRING(args[0]);
	return openFile(path, "wb");
}

// The native fputc extension function.
static Value fputcExtension(int argCount, Value *args) {
	PARAMS_2(IS_NUMBER, IS_NUMBER);
	int byte = (int)AS_NUMBER(args[0]);
	int index = (int)AS_NUMBER(args[1]);
	
	if (byte < 0 || byte > 255 || index < 0 || index > USER_FILE_MAX) {
		return NIL_VAL; // Parameters out of range.
	}
	
	FILE *stream = userStreams[index];
	
	if (stream == NULL) {
		return NIL_VAL; // Stream not open.
	}
	
	int result = fputc(byte, stream);
	
	if (result == EOF) {
		return NIL_VAL; // Could not write to stream.
	}
	
	return NUMBER_VAL((double)result);
}

// The native ftoa extension function.
static Value ftoaExtension(int argCount, Value *args) {
	PARAMS_1(IS_NUMBER);
	double number = AS_NUMBER(args[0]);
	char *chars = ALLOCATE(char, FTOA_SIZE);
	int length = snprintf(chars, FTOA_SIZE, "%f", number);
	
	if (length >= FTOA_SIZE) {
		length = FTOA_SIZE - 1;
	}
	
	char *point = strchr(chars, '.');
	
	if (point != NULL) {
		char *tail = &chars[length - 1];
		
		// Strip trailing zeroes.
		while (tail > point && *tail == '0') {
			*tail-- = '\0';
			length--;
		}
		
		// Strip trailing decimal point.
		if (tail == point) {
			*point = '\0';
			length--;
		}
	}
	
	// Don't return negative zero.
	if (length == 2 && chars[0] == '-' && chars[1] == '0') {
		chars[0] = '0';
		chars[1] = '\0';
		length = 1;
	}
	
	chars = GROW_ARRAY(char, chars, FTOA_SIZE, length + 1);
	return OBJ_VAL(takeString(chars, length));
}

// The native stderr extension function.
static Value stderrExtension(int argCount, Value *args) {
	PARAMS_0();
	return NUMBER_VAL((double)USER_STDERR);
}

// The native stdin extension function.
static Value stdinExtension(int argCount, Value *args) {
	PARAMS_0();
	return NUMBER_VAL((double)USER_STDIN);
}

// The native stdout extension function.
static Value stdoutExtension(int argCount, Value *args) {
	PARAMS_0();
	return NUMBER_VAL((double)USER_STDOUT);
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

// The native trunc extension function.
static Value truncExtension(int argCount, Value *args) {
	PARAMS_1(IS_NUMBER);
	double number = trunc(AS_NUMBER(args[0]));
	return NUMBER_VAL(number);
}

#undef PARAMS_0
#undef PARAMS_1
#undef PARAMS_2

void initExtensions(int argc, const char *argv[]) {
	if (argc > 1) {
		userArgc = argc - 1;
		userArgv = &argv[1];
	}
	
	userStreams[USER_STDIN] = stdin;
	userStreams[USER_STDOUT] = stdout;
	userStreams[USER_STDERR] = stderr;
}

void defineExtensions(DefineNativeFn defineNative) {
	defineNative("__argc", argcExtension);
	defineNative("__argv", argvExtension);
	defineNative("__chrat", chratExtension);
	defineNative("__exit", exitExtension);
	defineNative("__fclose", fcloseExtension);
	defineNative("__fgetc", fgetcExtension);
	defineNative("__fopenr", fopenrExtension);
	defineNative("__fopenw", fopenwExtension);
	defineNative("__fputc", fputcExtension);
	defineNative("__ftoa", ftoaExtension);
	defineNative("__stderr", stderrExtension);
	defineNative("__stdin", stdinExtension);
	defineNative("__stdout", stdoutExtension);
	defineNative("__strlen", strlenExtension);
	defineNative("__strof", strofExtension);
	defineNative("__trunc", truncExtension);
}

void freeExtensions() {
	for (int i = USER_FILE_MIN; i <= USER_FILE_MAX; i++) {
		FILE *file = userStreams[i];
		
		if (file != NULL) {
			fclose(file);
			userStreams[i] = NULL;
		}
	}
}
