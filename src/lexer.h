#ifndef LEXER_H_
#define LEXER_H_

typedef enum {
	HUSH_LEXEME_TYPE_ARGUMENT = 0,
	HUSH_LEXEME_TYPE_FILE_REDIRECT,
	HUSH_LEXEME_TYPE_END_OF_COMMAND, // ';' and '|' lexemes
	HUSH_LEXEME_TYPE_END_OF_BUFFER,
} Hush_Lexeme_Type;

typedef struct {
	int input_fd;
	int output_fd;
	int mode;
} File_Redirect;

typedef struct {
	Hush_Lexeme_Type type;
	char *content;
	File_Redirect file_redirect;
} Lexeme;

void print_lexeme(Lexeme lexeme);
Lexeme get_next_lexeme(Buffer *buffer);

#endif // LEXER_H_
