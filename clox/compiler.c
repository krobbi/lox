#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "compiler.h"
#include "memory.h"
#include "scanner.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif // DEBUG_PRINT_CODE

// A parser for containing the parsing state.
typedef struct {
	// The current token to match.
	Token current;
	
	// The token that has just been consumed.
	Token previous;
	
	// Whether an error was encountered.
	bool hadError;
	
	// Whether the parser is resynchronizing from an error.
	bool panicMode;
} Parser;

// An expression's precedence level.
typedef enum {
	// No precedence.
	PREC_NONE,
	
	// The precedence of an `x = y` expression.
	PREC_ASSIGNMENT,
	
	// The precedence of an `x or y` expresion.
	PREC_OR,
	
	// The precedence of an `x and y` expression.
	PREC_AND,
	
	// The precedence of an `x == y` or `x != y` expression.
	PREC_EQUALITY,
	
	// The precedence of an `x > y`, `x < y`, `x >= y`, or `x <= y` expression.
	PREC_COMPARISON,
	
	// The precedence of an `x + y` or `x - y` expression.
	PREC_TERM,
	
	// The precedence of an `x * y` or `x / y` expression.
	PREC_FACTOR,
	
	// The precedence of a `!x` or `-x` expression.
	PREC_UNARY,
	
	// The precedence of an `x.y` or `x(y)` expression.
	PREC_CALL,
	
	// The precedence of an `x` or `(x)` expression.
	PREC_PRIMARY,
} Precedence;

// A function that compiles an expression.
typedef void (*ParseFn)(bool canAssign);

// A rule for compiling an expression from an operator token.
typedef struct {
	// The function for compiling the operator as a prefix.
	ParseFn prefix;
	
	// The function for compiling the operator as an infix.
	ParseFn infix;
	
	// The operator's precedence as an infix.
	Precedence precedence;
} ParseRule;

// A local on the stack.
typedef struct {
	// The local's identifier token.
	Token name;
	
	// The local's scope depth.
	int depth;
	
	// Whether the local is captured by an upvalue.
	bool isCaptured;
} Local;

// An upvalue in the compiling function.
typedef struct {
	// The upvalue's slot index.
	uint8_t index;
	
	// Whether the upvalue points to a parent function's local.
	bool isLocal;
} Upvalue;

// A compiling function's type.
typedef enum {
	// A function declaration.
	TYPE_FUNCTION,
	
	// An initializer method declaration.
	TYPE_INITIALIZER,
	
	// A method declaration.
	TYPE_METHOD,
	
	// The top-level script.
	TYPE_SCRIPT,
} FunctionType;

// A compiler for containing the compilation state.
typedef struct Compiler {
	// The parent function's compiler.
	struct Compiler *enclosing;
	
	// The compiling function.
	ObjFunction *function;
	
	// The compiling function's type.
	FunctionType type;
	
	// The current defined locals on the stack.
	Local locals[UINT8_COUNT];
	
	// The number of locals in scope.
	int localCount;
	
	// The upvalues used by the compiling function.
	Upvalue upvalues[UINT8_COUNT];
	
	// The current scope depth.
	int scopeDepth;
} Compiler;

// A compiler for containing the class compilation state.
typedef struct ClassCompiler {
	// The parent class' compiler.
	struct ClassCompiler *enclosing;
	
	// Whether the compiling class has a superclass.
	bool hasSuperclass;
} ClassCompiler;

// The global parser instance.
Parser parser;

// The current compiler.
Compiler *current = NULL;

// The current class compiler.
ClassCompiler *currentClass = NULL;

// Get the chunk that is being compiled.
static Chunk *currentChunk() {
	return &current->function->chunk;
}

// Log an error at a token.
static void errorAt(Token *token, const char *message) {
	if (parser.panicMode) {
		return;
	}
	
	parser.panicMode = true;
	fprintf(stderr, "[line %d] Error", token->line);
	
	if (token->type == TOKEN_EOF) {
		fprintf(stderr, " at end");
	} else if (token->type != TOKEN_ERROR) {
		fprintf(stderr, " at '%.*s'", token->length, token->start);
	}
	
	fprintf(stderr, ": %s\n", message);
	parser.hadError = true;
}

// Log an error at the previous token.
static void error(const char *message) {
	errorAt(&parser.previous, message);
}

// Log an error at the current token.
static void errorAtCurrent(const char *message) {
	errorAt(&parser.current, message);
}

// Advance to the next non-error token.
static void advance() {
	parser.previous = parser.current;
	
	for (;;) {
		parser.current = scanToken();
		
		if (parser.current.type != TOKEN_ERROR) {
			break;
		}
		
		errorAtCurrent(parser.current.start);
	}
}

// Get whether the current token matches a type.
static bool check(TokenType type) {
	return parser.current.type == type;
}

// Advance if the current token matches a type.
static bool match(TokenType type) {
	if (check(type)) {
		advance();
		return true;
	} else {
		return false;
	}
}

// Log an error if the current token could not be matched.
static void consume(TokenType type, const char *message) {
	if (!match(type)) {
		errorAtCurrent(message);
	}
}

// Emit a byte of bytecode.
static void emitByte(uint8_t byte) {
	writeChunk(currentChunk(), byte, parser.previous.line);
}

// Emit two bytes of bytecode.
static void emitBytes(uint8_t byte1, uint8_t byte2) {
	emitByte(byte1);
	emitByte(byte2);
}

// Emit a loop instruction to a start offset.
static void emitLoop(int loopStart) {
	emitByte(OP_LOOP);
	
	uint32_t offset = currentChunk()->count - loopStart + 2;
	
	if (offset > UINT16_MAX) {
		error("Loop body too large.");
	}
	
	emitByte((offset >> 8) & 0xff);
	emitByte(offset & 0xff);
}

// Emit a jump instruction and return its placeholder operand offset.
static int emitJump(uint8_t instruction) {
	emitByte(instruction);
	emitByte(0xff);
	emitByte(0xff);
	return currentChunk()->count - 2;
}

// Emit an implicit return instruction.
static void emitReturn() {
	if (current->type == TYPE_INITIALIZER) {
		emitBytes(OP_GET_LOCAL, 0);
	} else {
		emitByte(OP_NIL);
	}
	
	emitByte(OP_RETURN);
}

// Make a constant index from a value.
static ConstantIndex makeConstant(Value value) {
	uint32_t constant = addConstant(currentChunk(), value);
	
	if (constant > CONSTANT_INDEX_MAX) {
		error("Too many constants in one chunk.");
		return 0;
	}
	
	return (ConstantIndex)constant;
}

// Emit a constant index.
static void emitConstantIndex(ConstantIndex constant) {
#ifdef LONG_CONSTANTS
	emitByte((constant >> 8) & 0xff);
	emitByte(constant & 0xff);
#else // LONG_CONSTANTS
	emitByte(constant);
#endif // !LONG_CONSTANTS
}

// Emit a constant instruction.
static void emitConstant(Value value) {
	emitByte(OP_CONSTANT);
	emitConstantIndex(makeConstant(value));
}

// Patch a jump instruction's operand to the current offset.
static void patchJump(int offset) {
	uint32_t jump = currentChunk()->count - offset - 2;
	
	if (jump > UINT16_MAX) {
		error("Too much code to jump over.");
	}
	
	currentChunk()->code[offset] = (jump >> 8) & 0xff;
	currentChunk()->code[offset + 1] = jump & 0xff;
}

// Initialize a new current compiler.
static void initCompiler(Compiler *compiler, FunctionType type) {
	compiler->enclosing = current;
	compiler->function = NULL;
	compiler->type = type;
	compiler->localCount = 0;
	compiler->scopeDepth = 0;
	compiler->function = newFunction();
	current = compiler;
	
	if (type != TYPE_SCRIPT) {
		current->function->name = copyString(parser.previous.start, parser.previous.length);
	}
	
	Local *local = &current->locals[current->localCount++];
	local->depth = 0;
	local->isCaptured = false;
	
	if (type != TYPE_FUNCTION) {
		local->name.start = "this";
		local->name.length = 4;
	} else {
		local->name.start = "";
		local->name.length = 0;
	}
}

// End the current compiler and return its function.
static ObjFunction *endCompiler() {
	emitReturn();
	ObjFunction *function = current->function;
	
#ifdef DEBUG_PRINT_CODE
	if (!parser.hadError) {
		disassembleChunk(
				currentChunk(), function->name != NULL ? function->name->chars : "<script>");
	}
#endif // DEBUG_PRINT_CODE
	
	current = current->enclosing;
	return function;
}

// Begin a new current scope.
static void beginScope() {
	current->scopeDepth++;
}

// End the current scope.
static void endScope() {
	current->scopeDepth--;
	
	while (
			current->localCount > 0
			&& current->locals[current->localCount - 1].depth > current->scopeDepth) {
		
		if (current->locals[current->localCount - 1].isCaptured) {
			emitByte(OP_CLOSE_UPVALUE); // Move local to heap.
		} else {
			emitByte(OP_POP); // Free local.
		}
		
		current->localCount--;
	}
}

// Compile an expression.
static void expression();

// Compile a statement.
static void statement();

// Compile a declaration.
static void declaration();

// Get a parse rule from its token type.
static ParseRule *getRule(TokenType type);

// Compile an expression with a precedence level.
static void parsePrecedence(Precedence precedence);

// Make a constant index from an identifier token.
static ConstantIndex identifierConstant(Token *name) {
	return makeConstant(OBJ_VAL(copyString(name->start, name->length)));
}

// Get whether two identifier tokens are equal.
static bool identifiersEqual(Token *a, Token *b) {
	if (a->length != b->length) {
		return false;
	}
	
	return memcmp(a->start, b->start, a->length) == 0;
}

// Resolve a variable's local stack slot.
static int resolveLocal(Compiler *compiler, Token *name) {
	for (int i = compiler->localCount - 1; i >= 0; i--) {
		Local *local = &compiler->locals[i];
		
		if (identifiersEqual(name, &local->name)) {
			if (local->depth == -1) {
				error("Can't read local variable in its own initializer.");
			}
			
			return i;
		}
	}
	
	return -1;
}

// Add an upvalue to a compiling function and return its upvalue slot.
static int addUpvalue(Compiler *compiler, uint8_t index, bool isLocal) {
	int upvalueCount = compiler->function->upvalueCount;
	
	for (int i = 0; i < upvalueCount; i++) {
		Upvalue *upvalue = &compiler->upvalues[i];
		
		if (upvalue->index == index && upvalue->isLocal == isLocal) {
			return i; // Share the same upvalue between each use.
		}
	}
	
	if (upvalueCount == UINT8_COUNT) {
		error("Too many closure variables in function.");
		return 0;
	}
	
	compiler->upvalues[upvalueCount].isLocal = isLocal;
	compiler->upvalues[upvalueCount].index = index;
	return compiler->function->upvalueCount++;
}

// Resolve a variable's upvalue slot.
static int resolveUpvalue(Compiler *compiler, Token *name) {
	if (compiler->enclosing == NULL) {
		return -1; // No more parent functions to check.
	}
	
	int local = resolveLocal(compiler->enclosing, name);
	
	if (local != -1) {
		compiler->enclosing->locals[local].isCaptured = true;
		return addUpvalue(compiler, (uint8_t)local, true);
	}
	
	int upvalue = resolveUpvalue(compiler->enclosing, name);
	
	if (upvalue != -1) {
		return addUpvalue(compiler, (uint8_t)upvalue, false);
	}
	
	return -1;
}

// Track a local declaration.
static void addLocal(Token name) {
	if (current->localCount == UINT8_COUNT) {
		error("Too many local variables in function.");
		return;
	}
	
	Local *local = &current->locals[current->localCount++];
	local->name = name;
	local->depth = -1; // The local has not finished being initialized.
	local->isCaptured = false;
}

// Track a variable declaration.
static void declareVariable() {
	if (current->scopeDepth == 0) {
		return; // Do not track global declarations.
	}
	
	Token* name = &parser.previous;
	
	for (int i = current->localCount - 1; i >= 0; i--) {
		Local *local = &current->locals[i];
		
		if (local->depth != -1 && local->depth < current->scopeDepth) {
			break; // Don't check for name conflicts with outer scopes.
		}
		
		if (identifiersEqual(name, &local->name)) {
			error("Already a variable with this name in this scope.");
		}
	}
	
	addLocal(*name);
}

// Declare an identifier and return a constant index for globals.
static ConstantIndex parseVariable(const char *errorMessage) {
	consume(TOKEN_IDENTIFIER, errorMessage);
	
	declareVariable();
	
	if (current->scopeDepth > 0) {
		return 0; // Do not generate identifier constants for locals.
	}
	
	return identifierConstant(&parser.previous);
}

// Mark the top local as initialized.
static void markInitialized() {
	if (current->scopeDepth == 0) {
		return; // Do not mark locals for globals.
	}
	
	current->locals[current->localCount - 1].depth = current->scopeDepth;
}

// Compile a variable definition from its identifier constant index.
static void defineVariable(ConstantIndex global) {
	if (current->scopeDepth > 0) {
		markInitialized();
		return; // Do not define globals for locals.
	}
	
	emitByte(OP_DEFINE_GLOBAL);
	emitConstantIndex(global);
}

// Compile an argument list and return a number of arguments.
static uint8_t argumentList() {
	uint8_t argCount = 0;
	
	if (!check(TOKEN_RIGHT_PAREN)) {
		do {
			expression();
			
			if (argCount == 255) {
				error("Can't have more than 255 arguments.");
			}
			
			argCount++;
		} while (match(TOKEN_COMMA));
	}
	
	consume(TOKEN_RIGHT_PAREN, "Expect ')' after arguments.");
	return argCount;
}

// Compile a variable set or get expression from an identifier token.
static void namedVariable(Token name, bool canAssign) {
	uint8_t getOp, setOp;
	int arg = resolveLocal(current, &name);
	bool isConstant = false;
	
	if (arg != -1) {
		getOp = OP_GET_LOCAL;
		setOp = OP_SET_LOCAL;
	} else if ((arg = resolveUpvalue(current, &name)) != -1) {
		getOp = OP_GET_UPVALUE;
		setOp = OP_SET_UPVALUE;
	} else {
		arg = identifierConstant(&name);
		isConstant = true;
		getOp = OP_GET_GLOBAL;
		setOp = OP_SET_GLOBAL;
	}
	
	if (canAssign && match(TOKEN_EQUAL)) {
		expression();
		emitByte(setOp);
	} else {
		emitByte(getOp);
	}
	
	if (isConstant) {
		emitConstantIndex((ConstantIndex)arg);
	} else {
		emitByte((uint8_t)arg);
	}
}

// Compile a variable set or get expression.
static void variable(bool canAssign) {
	namedVariable(parser.previous, canAssign);
}

// Create a synthetic token from its text.
static Token syntheticToken(const char *text) {
	Token token;
	token.start = text;
	token.length = (int)strlen(text);
	return token;
}

// Compile a super expression.
static void super_(bool canAssign) {
	(void)canAssign; // Unused parameter.
	
	if (currentClass == NULL) {
		error("Can't use 'super' outside of a class.");
	} else if (!currentClass->hasSuperclass) {
		error("Can't use 'super' in a class with no superclass.");
	}
	
	consume(TOKEN_DOT, "Expect '.' after 'super'.");
	consume(TOKEN_IDENTIFIER, "Expect superclass method name.");
	ConstantIndex name = identifierConstant(&parser.previous);
	
	namedVariable(syntheticToken("this"), false);
	
	if (match(TOKEN_LEFT_PAREN)) {
		uint8_t argCount = argumentList();
		namedVariable(syntheticToken("super"), false);
		emitByte(OP_SUPER_INVOKE);
		emitConstantIndex(name);
		emitByte(argCount);
	} else {
		namedVariable(syntheticToken("super"), false);
		emitByte(OP_GET_SUPER);
		emitConstantIndex(name);
	}
}

// Compile a this expression.
static void this_(bool canAssign) {
	(void)canAssign; // Unused parameter.
	
	if (currentClass == NULL) {
		error("Can't use 'this' outside of a class.");
		return;
	}
	
	variable(false);
}

// Compile a binary expression.
static void binary(bool canAssign) {
	(void)canAssign; // Unused parameter.
	
	TokenType operatorType = parser.previous.type;
	ParseRule *rule = getRule(operatorType);
	parsePrecedence((Precedence)(rule->precedence + 1));
	
	switch (operatorType) {
		case TOKEN_BANG_EQUAL: emitBytes(OP_EQUAL, OP_NOT); break;
		case TOKEN_EQUAL_EQUAL: emitByte(OP_EQUAL); break;
		case TOKEN_GREATER: emitByte(OP_GREATER); break;
		case TOKEN_GREATER_EQUAL: emitBytes(OP_LESS, OP_NOT); break;
		case TOKEN_LESS: emitByte(OP_LESS); break;
		case TOKEN_LESS_EQUAL: emitBytes(OP_GREATER, OP_NOT); break;
		case TOKEN_PLUS: emitByte(OP_ADD); break;
		case TOKEN_MINUS: emitByte(OP_SUBTRACT); break;
		case TOKEN_STAR: emitByte(OP_MULTIPLY); break;
		case TOKEN_SLASH: emitByte(OP_DIVIDE); break;
		default: error("Parser bug: Illegal binary operator."); break;
	}
}

// Compile a call expression.
static void call(bool canAssign) {
	(void)canAssign; // Unused parameter.
	
	uint8_t argCount = argumentList();
	emitBytes(OP_CALL, argCount);
}

// Compile a dot expression.
static void dot(bool canAssign) {
	consume(TOKEN_IDENTIFIER, "Expect property name after '.'.");
	ConstantIndex name = identifierConstant(&parser.previous);
	
	if (canAssign && match(TOKEN_EQUAL)) {
		expression();
		emitByte(OP_SET_PROPERTY);
		emitConstantIndex(name);
	} else if (match(TOKEN_LEFT_PAREN)) {
		uint8_t argCount = argumentList();
		emitByte(OP_INVOKE);
		emitConstantIndex(name);
		emitByte(argCount);
	} else {
		emitByte(OP_GET_PROPERTY);
		emitConstantIndex(name);
	}
}

// Compile a literal expression.
static void literal(bool canAssign) {
	(void)canAssign; // Unused parameter.
	
	switch (parser.previous.type) {
		case TOKEN_FALSE: emitByte(OP_FALSE); break;
		case TOKEN_NIL: emitByte(OP_NIL); break;
		case TOKEN_TRUE: emitByte(OP_TRUE); break;
		default: error("Parser bug: Illegal literal token."); break;
	}
}

// Compile a grouping expression.
static void grouping(bool canAssign) {
	(void)canAssign; // Unused parameter.
	
	expression();
	consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

// Compile a number expression.
static void number(bool canAssign) {
	(void)canAssign; // Unused parameter.
	
	double value = strtod(parser.previous.start, NULL);
	emitConstant(NUMBER_VAL(value));
}

// Compile a string expression.
static void string(bool canAssign) {
	(void)canAssign; // Unused parameter.
	
	emitConstant(OBJ_VAL(copyString(parser.previous.start + 1, parser.previous.length - 2)));
}

// Compile a unary expression.
static void unary(bool canAssign) {
	(void)canAssign; // Unused parameter.
	
	TokenType operatorType = parser.previous.type;
	
	parsePrecedence(PREC_UNARY);
	
	switch (operatorType) {
		case TOKEN_BANG: emitByte(OP_NOT); break;
		case TOKEN_MINUS: emitByte(OP_NEGATE); break;
		default: error("Parser bug: Illegal unary operator."); break;
	}
}

// Compile an and expression.
static void and_(bool canAssign) {
	(void)canAssign; // Unused parameter.
	
	int endJump = emitJump(OP_JUMP_IF_FALSE);
	
	emitByte(OP_POP);
	parsePrecedence(PREC_AND);
	
	patchJump(endJump);
}

// Compile an or expression.
static void or_(bool canAssign) {
	(void)canAssign; // Unused parameter.
	
	int elseJump = emitJump(OP_JUMP_IF_FALSE);
	int endJump = emitJump(OP_JUMP);
	
	patchJump(elseJump);
	emitByte(OP_POP);
	
	parsePrecedence(PREC_OR);
	patchJump(endJump);
}

// The global table of tokens to parse rules.
ParseRule rules[] = {
	[TOKEN_LEFT_PAREN]    = { grouping, call,   PREC_CALL },
	[TOKEN_RIGHT_PAREN]   = { NULL,     NULL,   PREC_NONE },
	[TOKEN_LEFT_BRACE]    = { NULL,     NULL,   PREC_NONE },
	[TOKEN_RIGHT_BRACE]   = { NULL,     NULL,   PREC_NONE },
	[TOKEN_COMMA]         = { NULL,     NULL,   PREC_NONE },
	[TOKEN_DOT]           = { NULL,     dot,    PREC_CALL },
	[TOKEN_MINUS]         = { unary,    binary, PREC_TERM },
	[TOKEN_PLUS]          = { NULL,     binary, PREC_TERM },
	[TOKEN_SEMICOLON]     = { NULL,     NULL,   PREC_NONE },
	[TOKEN_SLASH]         = { NULL,     binary, PREC_FACTOR },
	[TOKEN_STAR]          = { NULL,     binary, PREC_FACTOR },
	[TOKEN_BANG]          = { unary,    NULL,   PREC_NONE },
	[TOKEN_BANG_EQUAL]    = { NULL,     binary, PREC_EQUALITY },
	[TOKEN_EQUAL]         = { NULL,     NULL,   PREC_NONE },
	[TOKEN_EQUAL_EQUAL]   = { NULL,     binary, PREC_EQUALITY },
	[TOKEN_GREATER]       = { NULL,     binary, PREC_COMPARISON },
	[TOKEN_GREATER_EQUAL] = { NULL,     binary, PREC_COMPARISON },
	[TOKEN_LESS]          = { NULL,     binary, PREC_COMPARISON },
	[TOKEN_LESS_EQUAL]    = { NULL,     binary, PREC_COMPARISON },
	[TOKEN_IDENTIFIER]    = { variable, NULL,   PREC_NONE },
	[TOKEN_STRING]        = { string,   NULL,   PREC_NONE },
	[TOKEN_NUMBER]        = { number,   NULL,   PREC_NONE },
	[TOKEN_AND]           = { NULL,     and_,   PREC_AND },
	[TOKEN_CLASS]         = { NULL,     NULL,   PREC_NONE },
	[TOKEN_ELSE]          = { NULL,     NULL,   PREC_NONE },
	[TOKEN_FALSE]         = { literal,  NULL,   PREC_NONE },
	[TOKEN_FOR]           = { NULL,     NULL,   PREC_NONE },
	[TOKEN_FUN]           = { NULL,     NULL,   PREC_NONE },
	[TOKEN_IF]            = { NULL,     NULL,   PREC_NONE },
	[TOKEN_NIL]           = { literal,  NULL,   PREC_NONE },
	[TOKEN_OR]            = { NULL,     or_,    PREC_OR },
	[TOKEN_PRINT]         = { NULL,     NULL,   PREC_NONE },
	[TOKEN_RETURN]        = { NULL,     NULL,   PREC_NONE },
	[TOKEN_SUPER]         = { super_,   NULL,   PREC_NONE },
	[TOKEN_THIS]          = { this_,    NULL,   PREC_NONE },
	[TOKEN_TRUE]          = { literal,  NULL,   PREC_NONE },
	[TOKEN_VAR]           = { NULL,     NULL,   PREC_NONE },
	[TOKEN_WHILE]         = { NULL,     NULL,   PREC_NONE },
	[TOKEN_ERROR]         = { NULL,     NULL,   PREC_NONE },
	[TOKEN_EOF]           = { NULL,     NULL,   PREC_NONE },
};

static ParseRule *getRule(TokenType type) {
	return &rules[type];
}

static void parsePrecedence(Precedence precedence) {
	advance();
	ParseFn prefixRule = getRule(parser.previous.type)->prefix;
	
	if (prefixRule == NULL) {
		error("Expect expression.");
		return;
	}
	
	bool canAssign = precedence <= PREC_ASSIGNMENT;
	prefixRule(canAssign);
	
	while (precedence <= getRule(parser.current.type)->precedence) {
		advance();
		ParseFn infixRule = getRule(parser.previous.type)->infix;
		infixRule(canAssign);
	}
	
	if (canAssign && match(TOKEN_EQUAL)) {
		error("Invalid assignment target.");
	}
}

static void expression() {
	parsePrecedence(PREC_ASSIGNMENT);
}

// Compile a block statement.
static void block() {
	while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
		declaration();
	}
	
	consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

// Compile a function declaration from its type.
static void function(FunctionType type) {
	Compiler compiler;
	initCompiler(&compiler, type);
	beginScope();
	
	consume(TOKEN_LEFT_PAREN, "Expect '(' after function name.");
	
	if (!check(TOKEN_RIGHT_PAREN)) {
		do {
			current->function->arity++;
			
			if (current->function->arity > 255) {
				errorAtCurrent("Can't have more than 255 parameters.");
			}
			
			ConstantIndex constant = parseVariable("Expect parameter name.");
			defineVariable(constant);
		} while (match(TOKEN_COMMA));
	}
	
	consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");
	consume(TOKEN_LEFT_BRACE, "Expect '{' before function body.");
	block();
	
	ObjFunction *function = endCompiler();
	emitByte(OP_CLOSURE);
	emitConstantIndex(makeConstant(OBJ_VAL(function)));
	
	for (int i = 0; i < function->upvalueCount; i++) {
		emitByte(compiler.upvalues[i].isLocal ? 1 : 0);
		emitByte(compiler.upvalues[i].index);
	}
}

// Compile a method declaration.
static void method() {
	consume(TOKEN_IDENTIFIER, "Expect method name.");
	ConstantIndex constant = identifierConstant(&parser.previous);
	FunctionType type = TYPE_METHOD;
	
	if (parser.previous.length == 4 && memcmp(parser.previous.start, "init", 4) == 0) {
		type = TYPE_INITIALIZER;
	}
	
	function(type);
	emitByte(OP_METHOD);
	emitConstantIndex(constant);
}

// Compile a class declaration.
static void classDeclaration() {
	consume(TOKEN_IDENTIFIER, "Expect class name.");
	Token className = parser.previous;
	ConstantIndex nameConstant = identifierConstant(&parser.previous);
	declareVariable();
	
	emitByte(OP_CLASS);
	emitConstantIndex(nameConstant);
	defineVariable(nameConstant);
	
	ClassCompiler classCompiler;
	classCompiler.hasSuperclass = false;
	classCompiler.enclosing = currentClass;
	currentClass = &classCompiler;
	
	if (match(TOKEN_LESS)) {
		consume(TOKEN_IDENTIFIER, "Expect superclass name.");
		variable(false);
		
		if (identifiersEqual(&className, &parser.previous)) {
			error("A class can't inherit from itself.");
		}
		
		beginScope();
		addLocal(syntheticToken("super"));
		defineVariable(0);
		
		namedVariable(className, false);
		emitByte(OP_INHERIT);
		classCompiler.hasSuperclass = true;
	}
	
	namedVariable(className, false); // Push class to stack for method binding.
	consume(TOKEN_LEFT_BRACE, "Expect '{' before class body.");
	
	while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
		method();
	}
	
	consume(TOKEN_RIGHT_BRACE, "Expect '}' after class body.");
	emitByte(OP_POP); // End method binding.
	
	if (classCompiler.hasSuperclass) {
		endScope();
	}
	
	currentClass = currentClass->enclosing;
}

// Compile a function declaration.
static void funDeclaration() {
	ConstantIndex global = parseVariable("Expect function name.");
	markInitialized();
	function(TYPE_FUNCTION);
	defineVariable(global);
}

// Compile a variable declaration.
static void varDeclaration() {
	ConstantIndex global = parseVariable("Expect variable name.");
	
	if (match(TOKEN_EQUAL)) {
		expression();
	} else {
		emitByte(OP_NIL);
	}
	
	consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");
	defineVariable(global);
}

// Compile an expression statement.
static void expressionStatement() {
	expression();
	consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
	emitByte(OP_POP);
}

// Compile a for statement.
static void forStatement() {
	beginScope();
	consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");
	
	if (match(TOKEN_VAR)) {
		varDeclaration();
	} else if (!match(TOKEN_SEMICOLON)) {
		expressionStatement();
	}
	
	int loopStart = currentChunk()->count;
	int exitJump = -1;
	
	if (!match(TOKEN_SEMICOLON)) {
		expression(); // Condition.
		consume(TOKEN_SEMICOLON, "Expect ';' after loop condition.");
		
		exitJump = emitJump(OP_JUMP_IF_FALSE); // Skip loop body.
		emitByte(OP_POP); // Free condition for loop body.
	}
	
	if (!match(TOKEN_RIGHT_PAREN)) {
		int bodyJump = emitJump(OP_JUMP); // Skip increment.
		int incrementStart = currentChunk()->count;
		expression(); // Increment.
		emitByte(OP_POP); // Free increment.
		consume(TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");
		
		emitLoop(loopStart); // Jump to condition.
		loopStart = incrementStart; // Jump to increment after loop body.
		patchJump(bodyJump); // End of increment.
	}
	
	statement(); // Loop body.
	emitLoop(loopStart); // Jump to condition or increment.
	
	if (exitJump != -1) {
		patchJump(exitJump); // End of loop body.
		emitByte(OP_POP); // Free condition for loop exit.
	}
	
	endScope();
}

// Compile an if statement.
static void ifStatement() {
	consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
	expression(); // Condition.
	consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");
	
	int thenJump = emitJump(OP_JUMP_IF_FALSE); // Skip then clause.
	emitByte(OP_POP); // Free condition for then clause.
	statement(); // Then clause.
	
	int elseJump = emitJump(OP_JUMP); // Skip else clause.
	patchJump(thenJump); // End of then clause.
	emitByte(OP_POP); // Free condition for else clause.
	
	if (match(TOKEN_ELSE)) {
		statement(); // Else clause.
	}
	
	patchJump(elseJump); // End of else clause.
}

// Compile a print statement.
static void printStatement() {
	expression();
	consume(TOKEN_SEMICOLON, "Expect ';' after value.");
	emitByte(OP_PRINT);
}

// Compile a return statement.
static void returnStatement() {
	if (current->type == TYPE_SCRIPT) {
		error("Can't return from top-level code.");
	}
	
	if (match(TOKEN_SEMICOLON)) {
		emitReturn();
	} else {
		if (current->type == TYPE_INITIALIZER) {
			error("Can't return a value from an initializer.");
		}
		
		expression();
		consume(TOKEN_SEMICOLON, "Expect ';' after return value.");
		emitByte(OP_RETURN);
	}
}

// Compile a while statement.
static void whileStatement() {
	int loopStart = currentChunk()->count;
	consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
	expression(); // Condition.
	consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");
	
	int exitJump = emitJump(OP_JUMP_IF_FALSE); // Skip loop body.
	emitByte(OP_POP); // Free condition for loop body.
	statement(); // Loop body.
	emitLoop(loopStart); // Jump to condition.
	
	patchJump(exitJump); // End of loop body.
	emitByte(OP_POP); // Free condition for loop exit.
}

// Resynchronize the parser from an error.
static void synchronize() {
	parser.panicMode = false;
	
	// Skip to the next likely statement boundary.
	while (parser.current.type != TOKEN_EOF) {
		if (parser.previous.type == TOKEN_SEMICOLON) {
			return;
		}
		
		switch (parser.current.type) {
			case TOKEN_CLASS:
			case TOKEN_FUN:
			case TOKEN_VAR:
			case TOKEN_FOR:
			case TOKEN_IF:
			case TOKEN_WHILE:
			case TOKEN_PRINT:
			case TOKEN_RETURN:
				return;
			
			default:
				; // Keep searching.
		}
		
		advance();
	}
}

static void declaration() {
	if (match(TOKEN_CLASS)) {
		classDeclaration();
	} else if (match(TOKEN_FUN)) {
		funDeclaration();
	} else if (match(TOKEN_VAR)) {
		varDeclaration();
	} else {
		statement();
	}
	
	if (parser.panicMode) {
		synchronize();
	}
}

static void statement() {
	if (match(TOKEN_PRINT)) {
		printStatement();
	} else if (match(TOKEN_FOR)) {
		forStatement();
	} else if (match(TOKEN_IF)) {
		ifStatement();
	} else if (match(TOKEN_RETURN)) {
		returnStatement();
	} else if (match(TOKEN_WHILE)) {
		whileStatement();
	} else if (match(TOKEN_LEFT_BRACE)) {
		beginScope();
		block();
		endScope();
	} else {
		expressionStatement();
	}
}

ObjFunction *compile(const char *source) {
	initScanner(source);
	Compiler compiler;
	initCompiler(&compiler, TYPE_SCRIPT);
	
	parser.hadError = false;
	parser.panicMode = false;
	
	advance();
	
	while (!match(TOKEN_EOF)) {
		declaration();
	}
	
	ObjFunction *function = endCompiler();	
	return parser.hadError ? NULL : function;
}

void markCompilerRoots() {
	Compiler *compiler = current;
	
	while (compiler != NULL) {
		markObject((Obj*)compiler->function);
		compiler = compiler->enclosing;
	}
}
