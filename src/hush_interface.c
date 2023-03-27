#include <termios.h>
#include <pwd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/param.h>

#include "common.h"

#define KEY_CAP 5
#define HIST_CAP 4

typedef struct termios Termios;
static Termios original;

typedef struct {
	char *path;
	char buff[HIST_CAP][BUFF_CAP];
	size_t curr;
} Hist;
static Hist hist;

char * const hush_prompt = "hush % ";
const size_t hush_prompt_len = strlen(hush_prompt);

void hi_init_terminal(void)
{
	tcgetattr(STDIN_FILENO, &original);

	Termios raw = original;
	raw.c_iflag |= ICRNL;
	raw.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &raw);
	return;
}

void hi_release_terminal(void)
{
	tcsetattr(STDIN_FILENO, TCSANOW, &original);
	return;
}

static void hi_clear_terminal_prompt(size_t curr_buff_len)
{
	write(STDOUT_FILENO, "\r", 1);
	for(size_t i = 0; i < curr_buff_len + hush_prompt_len; ++i){
		write(STDOUT_FILENO, " ", 1);
	}
	write(STDOUT_FILENO, "\r", 1);
	write(STDOUT_FILENO, hush_prompt, hush_prompt_len);
	return;
}

void hi_init_history(void)
{
	struct passwd *user_pw = getpwuid(getuid());

	// Regenerate history filepath name
	const char *home_dir = user_pw->pw_dir;
	const char *hist_name = "/.hush_history";
	size_t home_dir_len = strlen(home_dir);
	size_t hist_name_len = strlen(hist_name);
	hist.path = (char *) calloc(home_dir_len + hist_name_len + 1, sizeof (char));
	strncpy(hist.path, home_dir, home_dir_len);
	strncat(hist.path, hist_name, hist_name_len);

	FILE *hist_file = fopen(hist.path, "r+");
	fseek(hist_file, 0, SEEK_SET);

	// Get curr line number
	char num[BUFF_CAP] = {0};
	if(fgets(num, BUFF_CAP, hist_file) == NULL || strcmp(num, "0\n") == 0){
		hist.curr = 0;
		return;
	}
	if((hist.curr = (size_t) strtol(num, (char **) NULL, 10)) == 0){
		fprintf(stderr, "hush: history file not formatted properly\n");
		exit(1);
	}

	// Read history into memory
	size_t i;
	for(i = 0; fgets(hist.buff[i], BUFF_CAP, hist_file) != NULL && i < HIST_CAP; ++i);
	if(i != HIST_CAP){
		assert(i == hist.curr);
	}
	assert(hist.curr < HIST_CAP);
	fclose(hist_file);
	return;
}

void hi_release_history(void)
{
	FILE *hist_file = fopen(hist.path, "w+");
	fprintf(hist_file, "%zu\n", hist.curr);
	for(size_t i = 0; i < HIST_CAP && hist.buff[i][0] != '\0'; ++i){
		fputs(hist.buff[i], hist_file);
	}
	fclose(hist_file);
	return;
}

static void hi_write_to_hist(const char *str)
{
	size_t str_len = strlen(str);
	strcpy(hist.buff[hist.curr], str);
	hist.buff[hist.curr][str_len] = '\n';
	hist.buff[hist.curr++][str_len + 1] = '\0';
	hist.curr %= HIST_CAP;
	return;
}

static bool hi_key_buffer_is_empty(const char *key, size_t key_cap)
{
	size_t sum = 0;
	for(size_t i = 0; i < key_cap; ++i){
		sum += key[i];
	}
	return sum == 0 ? true : false;
}

bool hi_fill_buffer(char *buffer)
{
	size_t cursor = 0, buffer_end = 0, hist_cursor = hist.curr;
	char key[KEY_CAP] = {0};
	bool at_beginning = false;
	memset(buffer, 0, BUFF_CAP);
	char temp_buffer[BUFF_CAP] = {0};

	write(STDOUT_FILENO, hush_prompt, hush_prompt_len);
	for(ever){
		read(STDIN_FILENO, key, KEY_CAP);
		switch(key[0]){
			// TODO: Handle control-C with signal.h?
			case '\004': // Control-D
				write(STDOUT_FILENO, "\n", 1);
				return false;
			case '\012': // New line
				write(STDOUT_FILENO, "\n", 1);
				buffer[buffer_end] = '\0';
				if(buffer_end != 0){
					hi_write_to_hist(buffer);
					hist_cursor = hist.curr;
				}
				return true;
			case '\033':
				if(key[1] == '['){
					switch(key[2]){
						case 'A': // Up arrow
							if(at_beginning){
								break;
							}

							// Save current buffer for later use
							if(hist_cursor == hist.curr){
								memcpy(temp_buffer, buffer, buffer_end);
								temp_buffer[buffer_end + 1] = '\0';
							}

							// Decrement history cursor
							if(hist_cursor == 0){
								if(hist.buff[HIST_CAP - 1][0] == '\0'){
									break;
								}
								hist_cursor = HIST_CAP;
							}
							if(--hist_cursor == hist.curr){
								at_beginning = true;
							}

							hi_clear_terminal_prompt(buffer_end);

							char *prev_line = hist.buff[hist_cursor];
							size_t prev_line_len = strlen(prev_line) - 1; // Minus 1 to ignore newline
							write(STDOUT_FILENO, prev_line, prev_line_len);
							strncpy(buffer, prev_line, prev_line_len);
							buffer_end = cursor = prev_line_len;
							buffer[buffer_end] = '\0';
							break;
						case 'B': // Down arrow

							// Increment history cursor
							if(hist_cursor == hist.curr && !at_beginning){
								break;
							}
							at_beginning = false;

							if(hist_cursor == HIST_CAP - 1){
								hist_cursor = 0;
							}else{
								++hist_cursor;
							}

							hi_clear_terminal_prompt(buffer_end);

							char *next_line = hist.buff[hist_cursor];
							size_t next_line_len = strlen(next_line) - 1; // Minus 1 to ignore newline

							// Pull previous buffer for current use
							if(hist_cursor == hist.curr){
								next_line = temp_buffer;
								next_line_len = strlen(temp_buffer);
							}

							write(STDOUT_FILENO, next_line, next_line_len);
							strncpy(buffer, next_line, next_line_len + 1);
							buffer_end = cursor = next_line_len;
							buffer[buffer_end] = '\0';
							break;
						case 'C': // Right arrow
							if(cursor < buffer_end){
								write(STDOUT_FILENO, &buffer[cursor], 1);
								++cursor;
							}
							break;
						case 'D': // Left arrow
							if(cursor > 0){
								write(STDOUT_FILENO, "\b", 1);
								--cursor;
							}
							break;
						default:
							break;
					}
				}else if(hi_key_buffer_is_empty(&key[1], KEY_CAP - 1)){ // Escape key (clear line)
					hi_clear_terminal_prompt(buffer_end);
					cursor = 0;
					buffer_end = 0;
					buffer[0] = '\0';
				}
				break;
			case '\177': // Backspace
				if(cursor > 0){
					write(STDOUT_FILENO, "\b", 1);
					write(STDOUT_FILENO, &buffer[cursor], buffer_end - cursor + 1);
					write(STDOUT_FILENO, " ", 1);
					for(size_t i = 0; i < buffer_end - cursor + 1; ++i){
						write(STDOUT_FILENO, "\b", 1);
					}
					memmove(&buffer[cursor - 1], &buffer[cursor], buffer_end - cursor + 1);
					--cursor;
					--buffer_end;
				}
				break;
			default: // Printable character
				if(buffer_end < BUFF_CAP - 2){ // Need space for the newline and the null terminator
					write(STDOUT_FILENO, key, 1);
					if(cursor != buffer_end){
						write(STDOUT_FILENO, &buffer[cursor], buffer_end - cursor + 1);
						for(size_t i = 0; i < buffer_end - cursor; ++i){
							write(STDOUT_FILENO, "\b", 1);
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
