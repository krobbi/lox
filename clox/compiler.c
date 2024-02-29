#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "compiler.h"
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
} Local;

// A compiler for containing the compilation state.
typedef struct {
	// The current defined locals on the stack.
	Local locals[UINT8_COUNT];
	
	// The number of locals in scope.
	int localCount;
	
	// The current scope depth.
	int scopeDepth;
} Compiler;

// The global parser instance.
Parser parser;

// The current compiler.
Compiler *current = NULL;

// The chunk that is being compiled.
Chunk *compilingChunk;

// Get the chunk that is being compiled.
static Chunk *currentChunk() {
	return compilingChunk;
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

// Emit a return instruction.
static void emitReturn() {
	emitByte(OP_RETURN);
}

// Make a constant index from a value.
static uint8_t makeConstant(Value value) {
	uint32_t constant = addConstant(currentChunk(), value);
	
	if (constant > UINT8_MAX) {
		error("Too many constants in one chunk.");
		return 0;
	}
	
	return (uint8_t)constant;
}

// Emit a constant instruction.
static void emitConstant(Value value) {
	emitBytes(OP_CONSTANT, makeConstant(value));
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
static void initCompiler(Compiler *compiler) {
	compiler->localCount = 0;
	compiler->scopeDepth = 0;
	current = compiler;
}

// End the current compiler.
static void endCompiler() {
	emitReturn();
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
		emitByte(OP_POP); // Free local.
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
static uint8_t identifierConstant(Token *name) {
	return makeConstant(OBJ_VAL(copyString(name->start, name->length)));
}

// Get whether two identifier tokens are equal.
static bool identifiersEqual(Token *a, Token *b) {
	if (a->length != b->length) {
		return false;
	}
	
	return memcmp(a->start, b->start, a->length) == 0;
}

// Get a local's stack slot.
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

// Track a local declaration.
static void addLocal(Token name) {
	if (current->localCount == UINT8_COUNT) {
		error("Too many local variables in function.");
		return;
	}
	
	Local *local = &current->locals[current->localCount++];
	local->name = name;
	local->depth = -1; // The local has not finished being initialized.
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

// Parse an identifier constant index.
static uint8_t parseVariable(const char *errorMessage) {
	consume(TOKEN_IDENTIFIER, errorMessage);
	
	declareVariable();
	
	if (current->scopeDepth > 0) {
		return 0; // Do not generate identifier constants for locals.
	}
	
	return identifierConstant(&parser.previous);
}

// Mark the top local as initialized.
static void markInitialized() {
	current->locals[current->localCount - 1].depth = current->scopeDepth;
}

// Compile a variable definition from its identifier constant index.
static void defineVariable(uint8_t global) {
	if (current->scopeDepth > 0) {
		markInitialized();
		return; // Do not define globals for locals.
	}
	
	emitBytes(OP_DEFINE_GLOBAL, global);
}

// Compile a variable set or get expression from an identifier token.
static void namedVariable(Token name, bool canAssign) {
	uint8_t getOp, setOp;
	int arg = resolveLocal(current, &name);
	
	if (arg != -1) {
		getOp = OP_GET_LOCAL;
		setOp = OP_SET_LOCAL;
	} else {
		arg = identifierConstant(&name);
		getOp = OP_GET_GLOBAL;
		setOp = OP_SET_GLOBAL;
	}
	
	if (canAssign && match(TOKEN_EQUAL)) {
		expression();
		emitBytes(setOp, (uint8_t)arg);
	} else {
		emitBytes(getOp, (uint8_t)arg);
	}
}

// Compile a variable set or get expression.
static void variable(bool canAssign) {
	namedVariable(parser.previous, canAssign);
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
	[TOKEN_LEFT_PAREN]    = { grouping, NULL,   PREC_NONE },
	[TOKEN_RIGHT_PAREN]   = { NULL,     NULL,   PREC_NONE },
	[TOKEN_LEFT_BRACE]    = { NULL,     NULL,   PREC_NONE },
	[TOKEN_RIGHT_BRACE]   = { NULL,     NULL,   PREC_NONE },
	[TOKEN_COMMA]         = { NULL,     NULL,   PREC_NONE },
	[TOKEN_DOT]           = { NULL,     NULL,   PREC_NONE },
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
	[TOKEN_SUPER]         = { NULL,     NULL,   PREC_NONE },
	[TOKEN_THIS]          = { NULL,     NULL,   PREC_NONE },
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

// Compile a variable declaration.
static void varDeclaration() {
	uint8_t global = parseVariable("Expect variable name.");
	
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
	if (match(TOKEN_VAR)) {
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

bool compile(const char *source, Chunk *chunk) {
	initScanner(source);
	Compiler compiler;
	initCompiler(&compiler);
	compilingChunk = chunk;
	
	parser.hadError = false;
	parser.panicMode = false;
	
	advance();
	
	while (!match(TOKEN_EOF)) {
		declaration();
	}
	
	endCompiler();
	
#ifdef DEBUG_PRINT_CODE
	if (!parser.hadError) {
		disassembleChunk(currentChunk(), "code");
	}
#endif // DEBUG_PRINT_CODE
	
	return !parser.hadError;
}
