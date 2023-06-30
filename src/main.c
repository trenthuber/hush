#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "buffer.h"
#include "lexer.h"
#include "parser.h"

int main(void)
{
	init_terminal();
	init_history();

	while (true) {
		
		// Get the next buffer from the user
		Buffer *buffer = get_next_buffer();
		if (buffer == NULL) {
			break;
		}
		// printf("BUFFER: '%s`\n", buffer->text);

		// Parse the commands based on the buffer
		Command command = get_next_command(buffer);
		while (command.name != NULL) {
			print_command(command);
			command = get_next_command(buffer);
		}
	}

	release_terminal();
	release_history();

	return 0;
}

// TODO: Implement strings
/* TODO: GLOBBING
	Here's how it would work: you'd only need to look at the arguments since commands and options can't have the '*' in them. So, you find if an argument has the star. If it does, you pass it to a function that locates all the files in your directory and creates a linked list of Hush_Args that it returns that contains all those files! This linked list is then appended to your list of args no problem! Globbing actually isn't too bad. The only hard part would be the making of a function that can create that linked list...but that's for future Trent to figure out
*/
