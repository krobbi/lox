#ifndef clox_scanner_h
#define clox_scanner_h

// A token's type.
typedef enum {
	// A `(` token.
	TOKEN_LEFT_PAREN,
	
	// A `)` token.
	TOKEN_RIGHT_PAREN,
	
	// A `{` token.
	TOKEN_LEFT_BRACE,
	
	// A `}` token.
	TOKEN_RIGHT_BRACE,
	
	// A `,` token.
	TOKEN_COMMA,
	
	// A `.` token.
	TOKEN_DOT,
	
	// A `-` token.
	TOKEN_MINUS,
	
	// A `+` token.
	TOKEN_PLUS,
	
	// A `;` token.
	TOKEN_SEMICOLON,
	
	// A `/` token.
	TOKEN_SLASH,
	
	// A `*` token.
	TOKEN_STAR,
	
	// A `!` token.
	TOKEN_BANG,
	
	// A `!=` token.
	TOKEN_BANG_EQUAL,
	
	// An `=` token.
	TOKEN_EQUAL,
	
	// An `==` token.
	TOKEN_EQUAL_EQUAL,
	
	// A `>` token.
	TOKEN_GREATER,
	
	// A `>=` token.
	TOKEN_GREATER_EQUAL,
	
	// A `<` token.
	TOKEN_LESS,
	
	// A `<=` token.
	TOKEN_LESS_EQUAL,
	
	// An identifier token.
	TOKEN_IDENTIFIER,
	
	// A string token.
	TOKEN_STRING,
	
	// A number token.
	TOKEN_NUMBER,
	
	// An `and` token.
	TOKEN_AND,
	
	// A `class` token.
	TOKEN_CLASS,
	
	// An `else` token.
	TOKEN_ELSE,
	
	// A `false` token.
	TOKEN_FALSE,
	
	// A `for` token.
	TOKEN_FOR,
	
	// A `fun` token.
	TOKEN_FUN,
	
	// An `if` token.
	TOKEN_IF,
	
	// A `nil` token.
	TOKEN_NIL,
	
	// An `or` token.
	TOKEN_OR,
	
	// A `print` token.
	TOKEN_PRINT,
	
	// A `return` token.
	TOKEN_RETURN,
	
	// A `super` token.
	TOKEN_SUPER,
	
	// A `this` token.
	TOKEN_THIS,
	
	// A `true` token.
	TOKEN_TRUE,
	
	// A `var` token.
	TOKEN_VAR,
	
	// A `while` token.
	TOKEN_WHILE,
	
	// A syntax error token.
	TOKEN_ERROR,
	
	// An end-of-file token.
	TOKEN_EOF,
} TokenType;

// A source lexeme with a type and line number.
typedef struct {
	// The token's type.
	TokenType type;
	
	// The pointer to the token's first character.
	const char *start;
	
	// The token's length.
	int length;
	
	// The token's line number.
	int line;
} Token;

// Initialize the scanner.
void initScanner(const char *source);

// Scan the next token from the scanner.
Token scanToken();

#endif // !clox_scanner_h
