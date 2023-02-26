#include "common.h"
#include "hush_lexer.h"
#include "hush_parser.h"

void hp_print_command(Hush_Command command)
{
	printf("Name: ");
	sv_print_content(command.name);
	printf("Args:\n");
	Hush_Arg *current_arg = command.args;
	while(current_arg){
		printf("\t%s: ", current_arg->is_str_lit ? "is_str" : "is_arg");
		sv_print_content(current_arg->value);
		current_arg = current_arg->next_arg;
	}
	printf("Opts:\n");
	Hush_Opt *current_opt = command.opts;
	while(current_opt){
		printf("\t");
		sv_print_content(current_opt->value);
		current_arg = current_opt->args;
		while(current_arg){
			printf("\t\t");
			sv_print_content(current_arg->value);
			current_arg = current_arg->next_arg;
		}
		current_opt = current_opt->next_opt;
	}
	printf("pipe_next: %s\n\n", command.pipe_next ? "true" : "false");
}

static void hp_append_argument(Hush_Command *command, Hush_Arg *arg)
{
	assert(arg->next_arg == NULL);
	Hush_Arg *current = command->args;
	if(command->opts){
		Hush_Opt *opt = command->opts;
		while(opt->next_opt != NULL){
			opt = opt->next_opt;
		}
		current = opt->args;
		if(current == NULL){
			opt->args = arg;
			return;
		}
	}else if(current == NULL){
		command->args = arg;
		return;
	}
	while(current->next_arg){
		current = current->next_arg;
	}
	current->next_arg = arg;
}

static void hp_append_option(Hush_Command *command, Hush_Opt *opt)
{
	assert(opt->next_opt == NULL);
	if(command->opts == NULL){
		command->opts = opt;
		return;
	}
	Hush_Opt *current = command->opts;
	while(current->next_opt){
		current = current->next_opt;
	}
	current->next_opt = opt;
}

bool hp_get_command(Hush_Lexer *lexer, Hush_Command *command)
{
	static Hush_Lexeme lexeme = {0};
	if(!hl_get_lexeme(lexer, &lexeme)){
		return false;
	}
	assert(lexeme.kind == HUSH_LEXEME_KIND_COMMAND);
	hl_print_lexeme(lexeme);
	command->name = lexeme.value;
	while(hl_get_lexeme(lexer, &lexeme)){
		hl_print_lexeme(lexeme);
		bool is_str_lit = false;
		switch(lexeme.kind){
			case HUSH_LEXEME_KIND_PIPE:
				if(!hl_is_next_lexeme(*lexer)){
					fprintf(stderr, "hush: parse error near `|'\n");
					return false;
				}
				command->pipe_next = true;
			case HUSH_LEXEME_KIND_SEMICOLON:
				return true;
			case HUSH_LEXEME_KIND_STR_LIT:
				is_str_lit = true;
			case HUSH_LEXEME_KIND_ARGUMENT: {
				Hush_Arg *arg = (Hush_Arg *) malloc(sizeof (Hush_Arg));
				Hush_Arg arg_zero = {0};
				*arg = arg_zero;

				arg->value = lexeme.value;
				arg->is_str_lit = is_str_lit;
				hp_append_argument(command, arg);
				break;
			}
			case HUSH_LEXEME_KIND_OPTION: {
				Hush_Opt *opt = (Hush_Opt *) malloc(sizeof (Hush_Opt));
				Hush_Opt opt_zero = {0};
				*opt = opt_zero;

				opt->value = lexeme.value;
				hp_append_option(command, opt);
				break;
			}
			case HUSH_LEXEME_KIND_FILE_IN:
			case HUSH_LEXEME_KIND_FILE_OUT:
			case HUSH_LEXEME_KIND_FILE_ERR:
			case HUSH_LEXEME_KIND_FILE_APPEND:
				if(!hl_is_next_lexeme(*lexer)){
					fprintf(stderr, "hush: parse error near `%s'\n", sv_to_str(lexeme.value));
					return false;
				}
				break;
			case HUSH_LEXEME_KIND_COMMAND:
			default:
				assert(false && "Unreachable");
				break;
		}
	}
	return (lexeme.value.size != 0);
}

static void hp_flush_args(Hush_Arg *arg)
{
	if(arg){
		Hush_Arg *next_arg;
		while(arg->next_arg){
			next_arg = arg->next_arg;
			free(arg);
			arg = next_arg;
		}
		free(arg);
	}
}

void hp_flush_command(Hush_Command *command)
{
	hp_flush_args(command->args);

	Hush_Opt *opt = command->opts;
	if(opt){
		Hush_Opt *next_opt;
		while(opt->next_opt){
			next_opt = opt->next_opt;
			hp_flush_args(opt->args);
			free(opt);
			opt = next_opt;
		}
		free(opt);
	}

	Hush_Command zero_reinit = {0};
	*command = zero_reinit;
}
