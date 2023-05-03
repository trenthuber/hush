#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <pwd.h>

#include "buffer.h"

#define KEY_CAP 5
#define HIST_CAP 100

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

void init_terminal(void)
{
	tcgetattr(STDIN_FILENO, &original);

	Termios raw = original;
	raw.c_iflag |= ICRNL;
	raw.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &raw);
}

void release_terminal(void)
{
	tcsetattr(STDIN_FILENO, TCSANOW, &original);
}

static void clear_terminal_prompt(size_t input_len)
{
	write(STDOUT_FILENO, "\r", 1);
	for (size_t i = 0; i < input_len + hush_prompt_len; ++i) {
		write(STDOUT_FILENO, " ", 1);
	}
	write(STDOUT_FILENO, "\r", 1);
	write(STDOUT_FILENO, hush_prompt, hush_prompt_len);
}

void init_history(void)
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
	if (hist_file == NULL) {
		hist.curr = 0;
		return;
	}
	fseek(hist_file, 0, SEEK_SET);

	// Get curr line number
	char num[BUFF_CAP] = {0};
	if (fgets(num, BUFF_CAP, hist_file) == NULL) {
		hist.curr = 0;
		return;
	}
	if ((hist.curr = (size_t) strtol(num, (char **) NULL, 10)) == 0 && strcmp(num, "0\n") != 0) {
		fprintf(stderr, "hush: history file not formatted properly\n");
		exit(1);
	}

	// Read history into memory
	size_t i;
	for (i = 0; fgets(hist.buff[i], BUFF_CAP, hist_file) != NULL && i < HIST_CAP; ++i) {

		// Get rid of the newline char
		size_t nl_loc = strnlen(hist.buff[i], BUFF_CAP) - 1;
		hist.buff[i][nl_loc] = '\0';
	}

	// TODO: Changing history capacity without clearing history file
	// Idea: Perhaps you leave the previous history cap in the file as well, and then compare against it and do your thing
	if (i != HIST_CAP) {
		assert(i == hist.curr);
	}
	assert(hist.curr < HIST_CAP);
	fclose(hist_file);
}

void release_history(void)
{
	FILE *hist_file = fopen(hist.path, "w+");
	fprintf(hist_file, "%zu\n", hist.curr);
	for (size_t i = 0; i < HIST_CAP && hist.buff[i][0] != '\0'; ++i) {
		fputs(hist.buff[i], hist_file);
		fputc('\n', hist_file);
	}
	fclose(hist_file);
}

static bool key_buffer_is_empty(const char *key, size_t key_cap)
{
	size_t sum = 0;
	for (size_t i = 0; i < key_cap; ++i) {
		sum += key[i];
	}
	return sum == 0 ? true : false;
}

bool fill_buffer(Buffer *buffer)
{
	buffer->cursor = buffer->text;

	// TODO: refactor input_cursor and whatnot to be a (char *)
	size_t input_cursor = 0, input_end = 0, hist_input_cursor = hist.curr;
	char key[KEY_CAP] = {0};
	bool at_beginning = false;
	char temp_buffer[BUFF_CAP] = {0};

	write(STDOUT_FILENO, hush_prompt, hush_prompt_len);
	while (true) {
		read(STDIN_FILENO, key, KEY_CAP);
		switch (key[0]) {
			// TODO: Handle control-C with signal.h?
			case '\004': { // Control-D
				write(STDOUT_FILENO, "\n", 1);
				return false;
			}
			case '\012': { // New line
				write(STDOUT_FILENO, "\n", 1);
				buffer->text[input_end] = '\0';
				buffer->end = buffer->text + input_end;
// printf("*buffer->cursor = %c (%d), *buffer->end = (%d)\n", *buffer->cursor, *buffer->cursor, *buffer->end);
				if (input_end != 0) {
					strncpy(hist.buff[hist.curr++], buffer->text, BUFF_CAP);
					hist.curr %= HIST_CAP;
				}
				return true;
			}
			case '\033': {
				if (key[1] == '[') {
					switch (key[2]) {
						case 'A': { // Up arrow
							if (at_beginning) {
								break;
							}

							// Save current buffer for later use
							if (hist_input_cursor == hist.curr) {
								memcpy(temp_buffer, buffer->text, input_end);
								temp_buffer[input_end] = '\0';
							}

							// Decrement history input_cursor
							if (hist_input_cursor == 0) {
								if (hist.buff[HIST_CAP - 1][0] == '\0') {
									break;
								}
								hist_input_cursor = HIST_CAP;
							}
							if (--hist_input_cursor == hist.curr) {
								at_beginning = true;
							}

							clear_terminal_prompt(input_end);

							char *prev_line = hist.buff[hist_input_cursor];
							size_t prev_line_len = strlen(prev_line);
							write(STDOUT_FILENO, prev_line, prev_line_len);
							strncpy(buffer->text, prev_line, prev_line_len);
							input_end = input_cursor = prev_line_len;
							buffer->text[input_end] = '\0';
							break;
						}
						case 'B': { // Down arrow

							// Increment history input_cursor
							if (hist_input_cursor == hist.curr && !at_beginning) {
								break;
							}
							at_beginning = false;

							if (hist_input_cursor == HIST_CAP - 1) {
								hist_input_cursor = 0;
							} else {
								++hist_input_cursor;
							}

							clear_terminal_prompt(input_end);

							char *next_line = hist.buff[hist_input_cursor];
							size_t next_line_len = strlen(next_line);

							// Pull previous buffer for current use
							if (hist_input_cursor == hist.curr) {
								next_line = temp_buffer;
								next_line_len = strlen(temp_buffer);
							}

							write(STDOUT_FILENO, next_line, next_line_len);
							strncpy(buffer->text, next_line, next_line_len + 1);
							input_end = input_cursor = next_line_len;
							buffer->text[input_end] = '\0';
							break;
						}
						case 'C': { // Right arrow
							if (input_cursor < input_end) {
								write(STDOUT_FILENO, &buffer->text[input_cursor], 1);
								++input_cursor;
							}
							break;
						}
						case 'D': { // Left arrow
							if (input_cursor > 0) {
								write(STDOUT_FILENO, "\b", 1);
								--input_cursor;
							}
						}
					}
				} else if (key_buffer_is_empty(&key[1], KEY_CAP - 1)) { // Escape key (clear line)
					clear_terminal_prompt(input_end);
					input_cursor = 0;
					input_end = 0;
					Buffer zero_reinit = {0};
					*buffer = zero_reinit;
				}
				break;
			}
			case '\177': { // Backspace
				if (input_cursor > 0) {
					write(STDOUT_FILENO, "\b", 1);
					write(STDOUT_FILENO, &buffer->text[input_cursor], input_end - input_cursor + 1);
					write(STDOUT_FILENO, " ", 1);
					for (size_t i = 0; i < input_end - input_cursor + 1; ++i) {
						write(STDOUT_FILENO, "\b", 1);
					}
					memmove(&buffer->text[input_cursor - 1], &buffer->text[input_cursor], input_end - input_cursor + 1);
					--input_cursor;
					--input_end;
				}
				break;
			}

			// TODO: Handle ctrl-v "verbatim insert" thing????
			default: { // Printable character
				if (input_end < BUFF_CAP - 1) { // Need space for the null terminator
					write(STDOUT_FILENO, key, 1);
					if (input_cursor != input_end) {
						write(STDOUT_FILENO, &buffer->text[input_cursor], input_end - input_cursor + 1);
						for (size_t i = 0; i < input_end - input_cursor; ++i) {
							write(STDOUT_FILENO, "\b", 1);
						}
						memmove(&buffer->text[input_cursor + 1], &buffer->text[input_cursor], input_end - input_cursor + 1);
					}
					buffer->text[input_cursor] = key[0];
					++input_cursor;
					++input_end;
				}
			}
		}
	}
}
