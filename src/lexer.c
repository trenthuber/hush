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
		case HUSH_LEXEME_TYPE_STRING: return "HUSH_LEXEME_TYPE_STRING";
		case HUSH_LEXEME_TYPE_FILE_REDIRECT: return "HUSH_LEXEME_TYPE_FILE_REDIRECT";
		case HUSH_LEXEME_TYPE_END_OF_COMMAND: return "HUSH_LEXEME_TYPE_END_OF_COMMAND";
		case HUSH_LEXEME_TYPE_END_OF_BUFFER: return "HUSH_LEXEME_TYPE_END_OF_BUFFER";
		default: return NULL;
	}
}

void print_lexeme(Lexeme lexeme)
{
	printf("Type: %s\n", get_lexeme_type_string(lexeme.type));
	if (lexeme.type != HUSH_LEXEME_TYPE_FILE_REDIRECT) {
		printf("Content: '%s`\n", lexeme.content);
	} else {
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

static char *get_file_redirect_mode_string(int mode)
{
	switch (mode) {
		case O_RDONLY: return "<";
		case O_WRONLY: return ">";
		case O_RDWR: return "<>";
		case O_APPEND: return ">>";
		default: return NULL;
	}
}

Lexeme get_next_lexeme(Buffer *buffer)
{
	Lexeme result = {0};
	for (; buffer->cursor < buffer->end && is_wspace(*buffer->cursor); ++buffer->cursor);
	if (buffer->cursor == buffer->end) {
		result.type = HUSH_LEXEME_TYPE_END_OF_BUFFER;
		return result;
	}

	switch (*buffer->cursor) {
		case '|': 
		case ';': {
			result.type = HUSH_LEXEME_TYPE_END_OF_COMMAND;
			if (++buffer->cursor == buffer->end) {
				result.content = buffer->cursor - 1;
			} else if (is_wspace(*buffer->cursor)) {
				*buffer->cursor = '\0';
				result.content = buffer->cursor - 1;
				++buffer->cursor;
			} else {
				result.content = (char *) malloc(2 * sizeof (char));
				if (result.content == NULL) {
					fprintf(stderr, "hush: unable to allocate memory\n");
					result.type = HUSH_LEXEME_TYPE_END_OF_BUFFER;
					return result;
				}
				result.content[0] = *(buffer->cursor - 1);
				result.content[1] = '\0';
			}
			break;
		}
		case '\'':
		case '"': {
			result.type = HUSH_LEXEME_TYPE_STRING;
			assert(false && "get_next_lexeme: string literal lexing not implemented yet");
			break;
		}
		default: {
			result.type = HUSH_LEXEME_TYPE_FILE_REDIRECT;
			char *begin = buffer->cursor;

			// "Skip" over any starting numbers
			for (; buffer->cursor < buffer->end && isdigit(*buffer->cursor); ++buffer->cursor);

			// Record the input file descriptor
			if (buffer->cursor != begin) {
				size_t input_fd_len = buffer->cursor - begin;
				char *input_fd_str = (char *) malloc((input_fd_len + 1) * sizeof (char));
				if (input_fd_str == NULL) {
					fprintf(stderr, "hush: couldn't allocate memory to 'input_fd_str`\n");
					result.type = HUSH_LEXEME_TYPE_END_OF_BUFFER;
					return result;
				}
				input_fd_str[input_fd_len] = '\0';
				strncpy(input_fd_str, begin, input_fd_len);
				result.file_redirect.input_fd = (int) strtol(input_fd_str, (char **) NULL, 10);
				free(input_fd_str);
			}

			// Check if the current lexeme is a file redirection
			result.file_redirect.mode = O_WRONLY;
			switch (*buffer->cursor) {
				case '<': {
					result.file_redirect.mode = O_RDONLY;
				}
				case '>': {
					if (buffer->cursor++ == begin && result.file_redirect.mode == O_WRONLY) {
						result.file_redirect.input_fd = STDOUT_FILENO;
					}
					if (buffer->cursor == buffer->end) {
						fprintf(stderr, "hush: parse error after '%s'\n", get_file_redirect_mode_string(result.file_redirect.mode));
						result.type = HUSH_LEXEME_TYPE_END_OF_BUFFER;
						return result;
					}
					if (*buffer->cursor == '>') {
						++buffer->cursor;
						if (result.file_redirect.mode == O_WRONLY) {
							result.file_redirect.mode = O_APPEND;
						} else {
							result.file_redirect.mode = O_RDWR;
						}
					}
					if (buffer->cursor == buffer->end) {
						fprintf(stderr, "hush: parse error after '%s`\n", get_file_redirect_mode_string(result.file_redirect.mode));
						result.type = HUSH_LEXEME_TYPE_END_OF_BUFFER;
						return result;
					}

					if (*buffer->cursor == '&') {
						begin = ++buffer->cursor;
						for (; buffer->cursor < buffer->end && isdigit(*buffer->cursor); ++buffer->cursor);
						if (buffer->cursor == begin || *buffer->cursor == '<' || *buffer->cursor == '>' || \
								!(buffer->cursor == buffer->end || is_lexeme_term(*buffer->cursor))) {
							fprintf(stderr, "hush: parse error after '%s&`\n", get_file_redirect_mode_string(result.file_redirect.mode));
							result.type = HUSH_LEXEME_TYPE_END_OF_BUFFER;
							return result;
						}
						size_t output_fd_len = buffer->cursor - begin;
						char *output_fd_str = (char *) malloc((output_fd_len + 1) * sizeof (char));
						output_fd_str[output_fd_len] = '\0';
						strncpy(output_fd_str, begin, output_fd_len);
						result.file_redirect.output_fd = (int) strtol(output_fd_str, (char **) NULL, 10);
						free(output_fd_str);
						return result;
					} else {
						for (; buffer->cursor < buffer->end && is_wspace(*buffer->cursor); ++buffer->cursor);
						if (buffer->cursor == buffer->end) {
							fprintf(stderr, "hush: parse error after '%s`\n", get_file_redirect_mode_string(result.file_redirect.mode));
							result.type = HUSH_LEXEME_TYPE_END_OF_BUFFER;
							return result;
						}
						begin = buffer->cursor;
					}
					break;
				}
				default: {
					result.type = HUSH_LEXEME_TYPE_ARGUMENT;
					break;
				}
			}

			for (; buffer->cursor < buffer->end && !is_lexeme_term(*buffer->cursor); ++buffer->cursor);

			// Find the most efficient way to allocate memory to the lexeme content
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
					result.type = HUSH_LEXEME_TYPE_END_OF_BUFFER;
					return result;
				}
				strncpy(result.content, begin, content_len);
				result.content[content_len] = '\0';
			}
			
			if (result.type == HUSH_LEXEME_TYPE_FILE_REDIRECT) {
				if ((result.file_redirect.output_fd = open(result.content, result.file_redirect.mode)) == -1) {
					fprintf(stderr, "hush: file '%s` cannot be opened\n", result.content);
					result.type = HUSH_LEXEME_TYPE_END_OF_BUFFER;
					return result;
				}

				// Free the content if it was allocated
				if (result.content < buffer->text || result.content > buffer->end) {
					free(result.content);
					result.content = NULL;
				}
			}
		}
	}
	return result;
}
