#include "common.h"
#include "hush_lexer.h"

Hush_Lexer hl_init_lexer(char *buffer, size_t max_buffer_size)
{
	size_t line_size = 0;
	while(line_size < max_buffer_size && buffer[line_size] != '\n'){
		++line_size;
	}

	Hush_Lexer result = {
		.line_size = line_size,
		.remaining = {
			.content = buffer,
			.size = line_size
		}
	};
	return result;
}

char *hl_kind_to_str(Hush_Lexeme_Kind kind)
{
	switch(kind){
		case HUSH_LEXEME_KIND_COMMAND: return "HUSH_LEXEME_KIND_COMMAND";
		case HUSH_LEXEME_KIND_OPTION: return "HUSH_LEXEME_KIND_OPTION";
		case HUSH_LEXEME_KIND_ARGUMENT: return "HUSH_LEXEME_KIND_ARGUMENT";
		case HUSH_LEXEME_KIND_SEMICOLON: return "HUSH_LEXEME_KIND_SEMICOLON";
		case HUSH_LEXEME_KIND_PIPE: return "HUSH_LEXEME_KIND_PIPE";
		case HUSH_LEXEME_KIND_STR_LIT: return "HUSH_LEXEME_KIND_STR_LIT";
		case HUSH_LEXEME_KIND_FILE_IN: return "HUSH_LEXEME_KIND_FILE_IN";
		case HUSH_LEXEME_KIND_FILE_OUT: return "HUSH_LEXEME_KIND_FILE_OUT";
		case HUSH_LEXEME_KIND_FILE_ERR: return "HUSH_LEXEME_KIND_FILE_ERR";
		case HUSH_LEXEME_KIND_FILE_APPEND: return "HUSH_LEXEME_KIND_FILE_APPEND";
	}
}

void hl_print_lexeme(Hush_Lexeme lexeme)
{
	printf("   Kind: %s\n", hl_kind_to_str(lexeme.kind));
	printf("Content: %.*s\n", (int) lexeme.value.size, lexeme.value.content);
	return;
}

static bool is_lexeme_terminator(char c)
{
	return isspace(c) || c == ';' || c == '|' || c == '"' || c == '<' || c == '>';
}

bool hl_is_next_lexeme(Hush_Lexer lexer)
{
	lexer.remaining = sv_trim_left(lexer.remaining);
	return !(lexer.remaining.size == 0);
}

bool hl_get_lexeme(Hush_Lexer *lexer, Hush_Lexeme *result)
{
	if(lexer->remaining.size == lexer->line_size ||
		result->kind == HUSH_LEXEME_KIND_SEMICOLON || 
		result->kind == HUSH_LEXEME_KIND_PIPE){
		result->kind = HUSH_LEXEME_KIND_COMMAND;
	}else{
		result->kind = HUSH_LEXEME_KIND_ARGUMENT;
	}

	lexer->remaining = sv_trim_left(lexer->remaining);
	if(lexer->remaining.size == 0){
		return false;
	}

	switch(lexer->remaining.content[0]){
		case '-':
			result->kind = HUSH_LEXEME_KIND_OPTION;
			sv_pop_char(&lexer->remaining);
			if(lexer->remaining.size == 0){
				fprintf(stderr, "hush: error in parsing option\n");
				result->value.size = 0;
				result->value.content = NULL;
				return false;
			}
			break;
		case ';':
			result->kind = HUSH_LEXEME_KIND_SEMICOLON;
			result->value = sv_pop_char(&lexer->remaining);
			return true;
		case '|':
			result->kind = HUSH_LEXEME_KIND_PIPE;
			result->value = sv_pop_char(&lexer->remaining);
			return true;
		case '"':
			result->kind = HUSH_LEXEME_KIND_STR_LIT;
			sv_pop_char(&lexer->remaining);
			result->value = sv_pop_left_delim(&lexer->remaining, '"');
			if(lexer->remaining.size == 0){
				fprintf(stderr, "hush: error in parsing string literal\n");
				result->value.size = 0;
				result->value.content = NULL;
				return false;
			}
			sv_pop_char(&lexer->remaining);
			return true;
		case '<':
			result->kind = HUSH_LEXEME_KIND_FILE_IN;
			result->value = sv_pop_char(&lexer->remaining);
			return true;
		case '>':
			switch(lexer->remaining.content[1]){
				case '!':
					result->kind = HUSH_LEXEME_KIND_FILE_ERR;
					result->value = sv_pop_num_chars(&lexer->remaining, 2);
					break;
				case '+':
					result->kind = HUSH_LEXEME_KIND_FILE_APPEND;
					result->value = sv_pop_num_chars(&lexer->remaining, 2);
					break;
				default:
					result->kind = HUSH_LEXEME_KIND_FILE_OUT;
					result->value = sv_pop_char(&lexer->remaining);
			}
			return true;
	}

	result->value = sv_pop_until_true(&lexer->remaining, &is_lexeme_terminator);
	return true;
}
