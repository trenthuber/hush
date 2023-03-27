#include "common.h"
#include "hush_interface.h"
#include "hush_lexer.h"
#include "hush_parser.h"

int main(void)
{
	char buffer[BUFF_CAP] = {0};
	hi_init_terminal();
	hi_init_history();

	for(ever){
		if(!hi_fill_buffer(buffer)){
			break;
		}
		printf("BUFFER: |%s|\n", buffer);

		Hush_Lexer lexer = hl_init_lexer(buffer, BUFF_CAP);
		Hush_Command command = {0};
		while(hp_get_command(&lexer, &command)){
			// Execute command in here 
			hp_print_command(command);
			hp_flush_command(&command);
		}
	}

	hi_release_terminal();
	hi_release_history();

	// TODO: Parse error messages
	// TODO: Region based memory management (for easier freeing of variables)
	/* TODO: GLOBBING
		Here's how it would work: you'd only need to look at the arguments since commands and options can't have the '*' in them. So, you find if an argument has the star. If it does, you pass it to a function that locates all the files in your directory and creates a linked list of Hush_Args that it returns that contains all those files! This linked list is then appended to your list of args no problem! Globbing actually isn't too bad. The only hard part would be the making of a function that can create that linked list...but that's for future Trent to figure out
	*/

	return 0;
}
