#include <termios.h>
#include <pwd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/param.h>

#include "common.h"

#define KEY_CAP 5
#define HIST_CAP 25

typedef struct termios Termios;
static Termios original;

typedef struct {
	char *path;
	char buff[HIST_CAP][BUFF_CAP];
	size_t curr;
} Hist;
static Hist hist;

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
	size_t cursor = 0, buffer_end = 0;
	char key[KEY_CAP] = {0};
	memset(buffer, 0, BUFF_CAP);

	char * const hush_prompt = "hush % ";
	const size_t hush_prompt_len = strlen(hush_prompt);
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
				hi_write_to_hist(buffer);
				return true;
			case '\033':
				if(key[1] == '['){
					switch(key[2]){
						case 'A': // Up arrow
							// TODO: Something weird going on with these keys?
							break;
						case 'B': // Down arrow
							// TODO
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
					write(STDOUT_FILENO, "\r", 1);
					for(size_t i = 0; i < buffer_end + hush_prompt_len; ++i){
						write(STDOUT_FILENO, " ", 1);
					}
					write(STDOUT_FILENO, "\r", 1);
					write(STDOUT_FILENO, hush_prompt, hush_prompt_len);
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
