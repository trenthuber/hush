#ifndef HUSH_LEXER_H_
#define HUSH_LEXER_H_

typedef enum {
	HUSH_LEXEME_KIND_COMMAND,
	HUSH_LEXEME_KIND_OPTION,
	HUSH_LEXEME_KIND_ARGUMENT,
	HUSH_LEXEME_KIND_SEMICOLON,
	HUSH_LEXEME_KIND_PIPE,
	HUSH_LEXEME_KIND_STR_LIT,
	HUSH_LEXEME_KIND_FILE_IN,
	HUSH_LEXEME_KIND_FILE_OUT,
	HUSH_LEXEME_KIND_FILE_ERR,
	HUSH_LEXEME_KIND_FILE_APPEND,
} Hush_Lexeme_Kind;

typedef struct {
	Hush_Lexeme_Kind kind;
	String_View value;
} Hush_Lexeme;

typedef struct {
	const size_t line_size;
	String_View remaining;
} Hush_Lexer;

Hush_Lexer hl_init_lexer(char *buffer, size_t max_buffer_size);
char *hl_kind_to_str(Hush_Lexeme_Kind kind);
void hl_print_lexeme(Hush_Lexeme lexeme);
bool hl_is_next_lexeme(Hush_Lexer lexer);
bool hl_get_lexeme(Hush_Lexer *lexer, Hush_Lexeme *lexeme);

#endif
