#ifndef PARSER_H_
#define PARSER_H_

typedef struct {
	char *name;
	char **args;
	File_Redirect *redirects;
	bool has_pipe;
} Command;

void print_command(Command command);
Command get_next_command(Buffer *buffer);

#endif // PARSER_H_
