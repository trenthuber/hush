#include <termios.h>

#include "common.h"

#define KEY_CAP 5

typedef struct termios Termios;

static Termios original;

void hi_init_terminal(int file_des)
{
	tcgetattr(file_des, &original);

	Termios raw = original;
	raw.c_iflag |= ICRNL;
	raw.c_lflag &= ~(ICANON | ECHO | ISIG);
	tcsetattr(file_des, TCSANOW, &raw);
	return;
}

void hi_release_terminal(int file_des)
{
	tcsetattr(file_des, TCSANOW, &original);
	return;
}

bool hi_fill_buffer(int file_des, char *buffer, size_t buffer_cap)
{
	size_t cursor = 0;
	char key[KEY_CAP] = {0};
	const char zero[KEY_CAP] = {0};
	memset(buffer, 0, buffer_cap);
	memcpy(key, zero, KEY_CAP);

	char * const hush_prompt = "hush % ";
	write(1, hush_prompt, strlen(hush_prompt));

	for(ever){
		buffer_loop_start:
		read(file_des, key, KEY_CAP);
		switch(key[0]){
			case '\003': // Control-C
				write(1, "\n", 1);
				buffer[0] = '\0';
				return true;
			case '\004': // Control-D
				write(1, "\n", 1);
				return false;
			case '\012': // New line
				write(1, "\n", 1);
				buffer[cursor] = '\0';
				return true;
			case '\033': // Escape sequence
				if(key[1] == '['){
					switch(key[2]){
						case 'A': // Up arrow
							// TODO
							printf("UP ARROW\n");
							break;
						case 'B': // Down arrow
							// TODO
							printf("DOWN ARROW\n");
							break;
						case 'C': // Right arrow
							// TODO
							printf("RIGHT ARROW\n");
							break;
						case 'D': // Left arrow
							// TODO
							printf("LEFT ARROW\n");
							break;
						default:
							goto buffer_loop_start;
					}
				}else if(key[1] == '\0' && key[2] == '\0' &&
						 key[3] == '\0' && key[4] == '\0'){
					// TODO
					printf("ESC\n");
					cursor = 0;
					buffer[0] = '\0';
				}else{
					break;
				}
				return true;
			case '\177': // Backspace
				// TODO
				if(cursor > 0){
					--cursor;
				}
				break;
			default: // Printable char
				write(1, key, 1);
				if(cursor < buffer_cap - 1){
					buffer[cursor++] = key[0];
				}else{
					buffer[buffer_cap] = '\0';
					return true;
				}
		}
	}
}
