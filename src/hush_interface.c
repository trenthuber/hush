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
	raw.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(file_des, TCSANOW, &raw);
	return;
}

void hi_release_terminal(int file_des)
{
	tcsetattr(file_des, TCSANOW, &original);
	return;
}

static bool hi_key_buffer_is_empty(char *key, size_t key_cap)
{
	size_t sum = 0;
	for(size_t i = 0; i < key_cap; ++i){
		sum += key[i];
	}
	return sum == 0 ? true : false;
}

bool hi_fill_buffer(int file_des, char *buffer, size_t buffer_cap)
{
	size_t cursor = 0, buffer_end = 0;
	char key[KEY_CAP] = {0};
	memset(buffer, 0, buffer_cap);

	char * const hush_prompt = "hush % ";
	const size_t hush_prompt_len = strlen(hush_prompt);
	write(1, hush_prompt, hush_prompt_len);

	for(ever){
		read(file_des, key, KEY_CAP);
		switch(key[0]){
			// TODO: Handle control-C with signal.h
			case '\004': // Control-D
				write(1, "\n", 1);
				return false;
			case '\012': // New line
				write(1, "\n", 1);
				buffer[buffer_end] = '\0';
				return true;
			case '\033':
				if(key[1] == '['){
					switch(key[2]){
						case 'A': // Up arrow
							// TODO
							printf("UP ARROW");
							break;
						case 'B': // Down arrow
							// TODO
							printf("DOWN ARROW");
							break;
						case 'C': // Right arrow
							if(cursor < buffer_end){
								write(1, &buffer[cursor], 1);
								++cursor;
							}
							break;
						case 'D': // Left arrow
							if(cursor > 0){
								write(1, "\b", 1);
								--cursor;
							}
							break;
						default:
							break;
					}
				}else if(hi_key_buffer_is_empty(&key[1], KEY_CAP - 1)){ // Escape key (clear line)
					write(1, "\r", 1);
					for(size_t i = 0; i < buffer_end + hush_prompt_len; ++i){
						write(1, " ", 1);
					}
					write(1, "\r", 1);
					write(1, hush_prompt, hush_prompt_len);
					cursor = 0;
					buffer_end = 0;
					buffer[0] = '\0';
				}
				break;
			case '\177': // Backspace
				if(cursor > 0){
					write(1, "\b", 1);
					write(1, &buffer[cursor], buffer_end - cursor + 1);
					write(1, " ", 1);
					for(size_t i = 0; i < buffer_end - cursor + 1; ++i){
						write(1, "\b", 1);
					}
					memmove(&buffer[cursor - 1], &buffer[cursor], buffer_end - cursor + 1);
					--cursor;
					--buffer_end;
				}
				break;
			default: // Printable character
				if(buffer_end < buffer_cap - 1){
					write(1, key, 1);
					if(cursor != buffer_end){
						write(1, &buffer[cursor], buffer_end - cursor + 1);
						for(size_t i = 0; i < buffer_end - cursor; ++i){
							write(1, "\b", 1);
						}
						memmove(&buffer[cursor + 1], &buffer[cursor], buffer_end - cursor + 1);
					}
					buffer[cursor] = key[0];
					++cursor;
					++buffer_end;
				}
				break;
		}
	}
}
