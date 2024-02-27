#include <stdio.h>
#include <string.h>

#include "common.h"
#include "scanner.h"

// A scanner for scanning tokens from source code.
typedef struct {
	// The pointer to the first character of the lexeme.
	const char *start;
	
	// The pointer to the current character.
	const char *current;
	
	// The current line number.
	int line;
} Scanner;

// The global scanner instance.
Scanner scanner;

void initScanner(const char *source) {
	scanner.start = source;
	scanner.current = source;
	scanner.line = 1;
}

// Get whether a character is a letter or underscore.
static bool isAlpha(char c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

// Get whether a character is a digit.
static bool isDigit(char c) {
	return c >= '0' && c <= '9';
}

// Get whether the scanner is at the end of the source code.
static bool isAtEnd() {
	return *scanner.current == '\0';
}

// Get the current character and advance to the next character.
static char advance() {
	scanner.current++;
	return scanner.current[-1];
}

// Get the current character.
static char peek() {
	return *scanner.current;
}

// Get the next character.
static char peekNext() {
	if (isAtEnd()) {
		return '\0';
	}
	
	return scanner.current[1];
}

// Advance if current character matches an expected character.
static bool match(char expected) {
	if (isAtEnd() || *scanner.current != expected) {
		return false;
	}
	
	scanner.current++;
	return true;
}

// Make a new token from its type.
static Token makeToken(TokenType type) {
	Token token;
	token.type = type;
	token.start = scanner.start;
	token.length = (int)(scanner.current - scanner.start);
	token.line = scanner.line;
	return token;
}

// Make a new error token from its message.
static Token errorToken(const char *message) {
	Token token;
	token.type = TOKEN_ERROR;
	token.start = message;
	token.length = (int)strlen(message);
	token.line = scanner.line;
	return token;
}

// Skip whitespace and comments.
static void skipWhitespace() {
	for (;;) {
		char c = peek();
		
		switch (c) {
			case ' ':
			case '\r':
			case '\t':
				advance();
				break;
			case '\n':
				scanner.line++;
				advance();
				break;
			case '/':
				if (peekNext() == '/') {
					while (peek() != '\n' && !isAtEnd()) {
						advance();
					}
				} else {
					return;
				}
				
				break;
			default:
				return;
		}
	}
}

// Check part of the current lexeme against a keyword and return a token type.
static TokenType checkKeyword(int start, int length, const char *rest, TokenType type) {
	if (
			scanner.current - scanner.start == start + length
			&& memcmp(scanner.start + start, rest, length) == 0) {
		return type;
	}
	
	return TOKEN_IDENTIFIER;
}

// Get the current lexeme's identifier or keyword token type.
static TokenType identifierType() {
	switch (scanner.start[0]) {
		case 'a': return checkKeyword(1, 2, "nd", TOKEN_AND);
		case 'c': return checkKeyword(1, 4, "lass", TOKEN_CLASS);
		case 'e': return checkKeyword(1, 3, "lse", TOKEN_ELSE);
		case 'f':
			if (scanner.current - scanner.start > 1) {
				switch (scanner.start[1]) {
					case 'a': return checkKeyword(2, 3, "lse", TOKEN_FALSE);
					case 'o': return checkKeyword(2, 1, "r", TOKEN_FOR);
					case 'u': return checkKeyword(2, 1, "n", TOKEN_FUN);
				}
			}
			
			break;
		case 'i': return checkKeyword(1, 1, "f", TOKEN_IF);
		case 'n': return checkKeyword(1, 2, "il", TOKEN_NIL);
		case 'o': return checkKeyword(1, 1, "r", TOKEN_OR);
		case 'p': return checkKeyword(1, 4, "rint", TOKEN_PRINT);
		case 'r': return checkKeyword(1, 5, "eturn", TOKEN_RETURN);
		case 's': return checkKeyword(1, 4, "uper", TOKEN_SUPER);
		case 't':
			if (scanner.current - scanner.start > 1) {
				switch (scanner.start[1]) {
					case 'h': return checkKeyword(2, 2, "is", TOKEN_THIS);
					case 'r': return checkKeyword(2, 2, "ue", TOKEN_TRUE);
				}
			}
			
			break;
		case 'v': return checkKeyword(1, 2, "ar", TOKEN_VAR);
		case 'w': return checkKeyword(1, 4, "hile", TOKEN_WHILE);
	}
	
	return TOKEN_IDENTIFIER;
}

// Make a new identifier or keyword token.
static Token identifier() {
	while (isAlpha(peek()) || isDigit(peek())) {
		advance();
	}
	
	return makeToken(identifierType());
}

// Make a new number token.
static Token number() {
	while (isDigit(peek())) {
		advance();
	}
	
	if (peek() == '.' && isDigit(peekNext())) {
		advance();
		
		while (isDigit(peek())) {
			advance();
		}
	}
	
	return makeToken(TOKEN_NUMBER);
}

// Make a new string token.
static Token string() {
	while (peek() != '"' && !isAtEnd()) {
		if (peek() == '\n') {
			scanner.line++;
		}
		
		advance();
	}
	
	if (isAtEnd()) {
		return errorToken("Unterminated string.");
	}
	
	advance();
	return makeToken(TOKEN_STRING);
}

Token scanToken() {
	skipWhitespace();
	scanner.start = scanner.current;
	
	if (isAtEnd()) {
		return makeToken(TOKEN_EOF);
	}
	
	char c = advance();
	
	if (isAlpha(c)) {
		return identifier();
	}
	
	if (isDigit(c)) {
		return number();
	}
	
	switch (c) {
		case '(': return makeToken(TOKEN_LEFT_PAREN);
		case ')': return makeToken(TOKEN_RIGHT_PAREN);
		case '{': return makeToken(TOKEN_LEFT_BRACE);
		case '}': return makeToken(TOKEN_RIGHT_BRACE);
		case ';': return makeToken(TOKEN_SEMICOLON);
		case ',': return makeToken(TOKEN_COMMA);
		case '.': return makeToken(TOKEN_DOT);
		case '-': return makeToken(TOKEN_MINUS);
		case '+': return makeToken(TOKEN_PLUS);
		case '/': return makeToken(TOKEN_SLASH);
		case '*': return makeToken(TOKEN_STAR);
		case '!': return makeToken(match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
		case '=': return makeToken(match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
		case '<': return makeToken(match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
		case '>': return makeToken(match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
		case '"': return string();
	}
	
	return errorToken("Unexpected character.");
}
