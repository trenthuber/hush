#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "buffer.h"
#include "lexer.h"
#include "parser.h"

typedef struct Array_LL Array_LL;
struct Array_LL {
	void *content;
	Array_LL *next;
};

static bool fr_equals_zero(File_Redirect fr)
{
	return fr.input_fd == 0 && fr.output_fd == 0 && fr.mode == 0;
}

void print_command(Command command)
{
	printf("Name: %s\n", command.name);
	printf("Args:\n");
	char **temp_arg = command.args;
	while (*temp_arg != NULL) {
		printf("\t%s\n", *temp_arg);
		++temp_arg;
	}
	File_Redirect *temp_fr = command.redirects;
	if (temp_fr != NULL) {
		printf("Redirects:\n");
		while (!fr_equals_zero(*temp_fr)) {
			printf("\t%d -> %d (%d)\n", temp_fr->input_fd, temp_fr->output_fd, temp_fr->mode);
			++temp_fr;
		}
	}
	printf("Pipeline: %s\n\n", command.has_pipe ? "true" : "false");
}

Command get_next_command(Buffer *buffer)
{
	Command command = {0};
	Lexeme lexeme = get_next_lexeme(buffer);
	if (lexeme.type == HUSH_LEXEME_TYPE_END_OF_BUFFER) {
		return command;
	}
	if (lexeme.type != HUSH_LEXEME_TYPE_ARGUMENT) {
		fprintf(stderr, "hush: parse error near '%s`\n", lexeme.content);
		return command;
	}

	command.name = lexeme.content;
	Array_LL *args_ll = (Array_LL *) malloc(sizeof (Array_LL));
	args_ll->content = (void *) lexeme.content;
	args_ll->next = NULL;
	Array_LL **arg_temp = &args_ll->next, *frs_ll = NULL, **fr_temp = &frs_ll;
	size_t num_args = 1, num_frs = 0;

	while (lexeme.type != HUSH_LEXEME_TYPE_END_OF_COMMAND && lexeme.type != HUSH_LEXEME_TYPE_END_OF_BUFFER) {
		lexeme = get_next_lexeme(buffer);
		switch (lexeme.type) {
			case HUSH_LEXEME_TYPE_ARGUMENT: {
				Array_LL *node = (Array_LL *) malloc(sizeof (Array_LL));
				node->content = (void *) lexeme.content;
				node->next = NULL;
				*arg_temp = node;
				arg_temp = &node->next;
				++num_args;
				break;
			}
			case HUSH_LEXEME_TYPE_STRING: {
				// TODO
				break;
			}
			case HUSH_LEXEME_TYPE_FILE_REDIRECT: {
				File_Redirect *fr_malloc = (File_Redirect *) malloc(sizeof (File_Redirect));
				*fr_malloc = lexeme.file_redirect;
				Array_LL *node = (Array_LL *) malloc(sizeof (Array_LL));
				if (num_frs == 0) {
					frs_ll = node;
				}
				node->content = (void *) fr_malloc;
				node->next = NULL;
				*fr_temp = node;
				fr_temp = &node->next;
				++num_frs;
				break;
			}
			case HUSH_LEXEME_TYPE_END_OF_COMMAND: {
				if (strcmp(lexeme.content, "|") == 0) {
					command.has_pipe = true;
				}
			}
			case HUSH_LEXEME_TYPE_END_OF_BUFFER: {
				break;
			}
			default: {
				assert(false && "Unreachable");
			}
		}
	}
	command.args = (char **) calloc(num_args + 1, sizeof (char *));
	arg_temp = &args_ll;
	for (size_t i = 0; i < num_args; ++i) {
		command.args[i] = (char *) (*arg_temp)->content;
		arg_temp = &(*arg_temp)->next;
	}
	command.args[num_args] = NULL;
	if (num_frs > 0) {
		command.redirects = (File_Redirect *) calloc(num_frs + 1, sizeof (File_Redirect));
		fr_temp = &frs_ll;
		for (size_t i = 0; i < num_frs; ++i) {
			command.redirects[i] = *(File_Redirect *) (*fr_temp)->content;
			fr_temp = &(*fr_temp)->next;
		}
		File_Redirect fr_struct_zero = {0};
		command.redirects[num_frs] = fr_struct_zero;
	}
	return command;
}
