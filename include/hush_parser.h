#ifndef HUSH_PARSER_H_
#define HUSH_PARSER_H_

typedef struct Hush_Arg Hush_Arg;
typedef struct Hush_Opt Hush_Opt;
typedef struct Hush_Command Hush_Command;

struct Hush_Arg {
	String_View value;
	bool is_str_lit;
	Hush_Arg *next_arg;
};

struct Hush_Opt {
	String_View value;
	Hush_Arg *args;
	Hush_Opt *next_opt;
};

struct Hush_Command {
	String_View name;
	Hush_Arg *args;
	Hush_Opt *opts;
	bool pipe_next;
};

void hp_print_command(Hush_Command command);
bool hp_get_command(Hush_Lexer *lexer, Hush_Command *command);
void hp_flush_command(Hush_Command *command);

#endif
