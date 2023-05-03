#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "buffer.h"
#include "lexer.h"

static char *get_lexeme_type_string(Hush_Lexeme_Type type)
{
	switch (type) {
		case HUSH_LEXEME_TYPE_ARGUMENT: return "HUSH_LEXEME_TYPE_ARGUMENT";
		case HUSH_LEXEME_TYPE_LITERAL: return "HUSH_LEXEME_TYPE_LITERAL";
		case HUSH_LEXEME_TYPE_STRING: return "HUSH_LEXEME_TYPE_STRING";
		case HUSH_LEXEME_TYPE_FILE_REDIRECT: return "HUSH_LEXEME_TYPE_FILE_REDIRECT";
		default: return NULL;
	}
}

void print_lexeme(Lexeme lexeme)
{
	printf("Type: %s\n", get_lexeme_type_string(lexeme.type));
	printf("Content: '%s`\n", lexeme.content);
	if (lexeme.type == HUSH_LEXEME_TYPE_FILE_REDIRECT) {
		printf("File Redirect:\n");
		printf("	 input_fd = %d\n", lexeme.file_redirect.input_fd);
		printf("	output_fd = %d\n", lexeme.file_redirect.output_fd);
		printf("	     mode = %d\n", lexeme.file_redirect.mode);
	}
	printf("\n");
}

static bool is_wspace(char chr)
{
	return isspace(chr) || chr == '\0';
}

static bool is_lexeme_term(char chr)
{
	return chr == ';' || chr == '|' || chr == '<' || chr == '>' || is_wspace(chr);
}

static bool is_file_redirect(char *str, File_Redirect *file_redirect)
{
	size_t cursor = 0;
	size_t str_len = strlen(str);
	
	// Get input file descriptor
	for (; cursor < str_len && str[cursor] >= '0' && str[cursor] <= '9'; ++cursor);
	if (cursor > 0) {
		char *first_num = (char *) malloc((cursor + 1) * sizeof (char));
		first_num[cursor] = '\0';
		strncpy(first_num, str, cursor);
		file_redirect->input_fd = (int) strtol(first_num, (char **) NULL, 10);
	}

	// Get file mode
	switch (str[cursor]) {
		case '<': {
			file_redirect->mode = O_RDONLY;
			if (cursor < str_len - 1 && str[++cursor] == '>') {
				file_redirect->mode = O_RDWR;
				++cursor;
			}
			break;
		}
		case '>': {
			file_redirect->mode = O_WRONLY;
			if (cursor == 0) {
				file_redirect->input_fd = STDOUT_FILENO;
			}
			if (cursor < str_len - 1 && str[++cursor] == '>') {
				file_redirect->mode = O_APPEND;
				++cursor;
			}
			break;
		}
		default: {
			return false;
		}
	}
	if (cursor == str_len) {
		return false;
	}
	
	// Get output file descriptor
	if (str[cursor++] == '&') {
		if (cursor == str_len) {
			fprintf(stderr, "hush: parse error after '&`, expected a number\n");
			exit(1);
		}
		size_t output_fd_start = cursor;
		for (; cursor < str_len && str[cursor] >= 48 && str[cursor] <= 57; ++cursor);
		if (cursor != str_len) {
			fprintf(stderr, "hush: parse error after '&`, expected a number\n");
			exit(1);
		}
		file_redirect->output_fd = (int) strtol(&str[output_fd_start], (char **) NULL, 10);
	} else {
		for (; cursor < str_len && is_wspace(str[cursor]); ++cursor);
		if (cursor == str_len) {
			return false;
		}
		if ((file_redirect->output_fd = open(&str[cursor], file_redirect->mode)) == -1) {
			fprintf(stderr, "hush: can't open file '%s`\n", &str[cursor]);
			exit(1);
		}
	}

	return true;
}

Lexeme get_next_lexeme(Buffer *buffer)
{
	Lexeme result = {0};
	if (buffer->cursor == buffer->end) {
		return result; // Check if lexeme.content == NULL outside this function
	}
	switch (*buffer->cursor) {
		case '|': 
		case ';': {
			result.type = HUSH_LEXEME_TYPE_LITERAL;
			if (buffer->cursor + 1 == buffer->end) {
				result.content = buffer->cursor;
			} else if (is_wspace(*(buffer->cursor + 1))) {
				*(buffer->cursor + 1) = '\0';
				result.content = buffer->cursor;
				buffer->cursor += 2;
			} else {
				result.content = (char *) malloc(2 * sizeof (char));
				if (result.content == NULL) {
					fprintf(stderr, "hush: unable to allocate memory\n");
					exit(1);
				}
				result.content[0] = *buffer->cursor;
				result.content[1] = '\0';
				++buffer->cursor;
			}
			break;
		}
		case '"': {
			result.type = HUSH_LEXEME_TYPE_STRING;
			assert(false && "get_next_lexeme: String literal lexing not implemented yet");
			break;
		}
		default: {
			char *begin = buffer->cursor;

			// "Skip" over any starting numbers
			for (; buffer->cursor < buffer->end && *buffer->cursor >= '0' && *buffer->cursor <= '9'; ++buffer->cursor);

			// Then "skip" over any file redirection symbols
			if (buffer->cursor < buffer->end && (*buffer->cursor == '<' || *buffer->cursor == '>')) {
				++buffer->cursor;
				if (buffer->cursor == buffer->end) {
					fprintf(stderr, "hush: parse error near '%c`\n", *(buffer->cursor - 1));
					exit(1);
				}
				if (*buffer->cursor == '>') {
					++buffer->cursor;
				}
				for (; buffer->cursor < buffer->end && is_wspace(*buffer->cursor); ++buffer->cursor);
				if (buffer->cursor == buffer->end) {
					fprintf(stderr, "hush: parse error near '%c`\n", *(buffer->cursor - 1));
					exit(1);
				}
				if (*buffer->cursor == '&') {
					++buffer->cursor;
					for (; buffer->cursor < buffer->end && *buffer->cursor >= '0' && *buffer->cursor <= '9'; ++buffer->cursor);
					if (!is_lexeme_term(*buffer->cursor)) {
						fprintf(stderr, "hush: parse error after '&`, expected numbers\n");
						exit(1);
					}
				}
			}

			for (; buffer->cursor < buffer->end && !is_lexeme_term(*buffer->cursor); ++buffer->cursor);

			// Find the most effiecent way to allocate memory to the lexeme content
			if (buffer->cursor == buffer->end) {
				result.content = begin;
			} else if (is_wspace(*buffer->cursor)) {
				*(buffer->cursor++) = '\0';
				result.content = begin;
			} else {
				size_t content_len = buffer->cursor - begin;
				result.content = (char *) malloc((content_len + 1) * sizeof (char));
				if (result.content == NULL) {
					fprintf(stderr, "hush: unable to allocate memory\n");
					exit(1);
				}
				strncpy(result.content, begin, content_len);
				result.content[content_len] = '\0';
			}
			
			if (is_file_redirect(result.content, &result.file_redirect)) {
				result.type = HUSH_LEXEME_TYPE_FILE_REDIRECT;
			}
		}
	}
	return result;
}
