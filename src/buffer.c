#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <termios.h>
#include <unistd.h>

#include "buffer.h"

#define KEY_CAP 5
#define HIST_CAP 100

typedef struct termios Termios;
static Termios original;

typedef struct {
	char *path;
	char buff[HIST_CAP][BUFF_CAP];
	size_t end;
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
		hist.end = 0;
		return;
	}
	fseek(hist_file, 0, SEEK_SET);

	// Get curr line number
	char num[BUFF_CAP] = {0};
	if (fgets(num, BUFF_CAP, hist_file) == NULL) {
		hist.end = 0;
		return;
	}
	if ((hist.end = (size_t) strtol(num, (char **) NULL, 10)) == 0 && strcmp(num, "0\n") != 0) {
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
	fclose(hist_file);

	// TODO: Changing history capacity without clearing history file
	// Idea: Perhaps you leave the previous history cap in the file as well, and then compare against it and do your thing
	if (i != HIST_CAP) {
		assert(i == hist.end);
	}
	assert(hist.end < HIST_CAP);
}

void release_history(void)
{
	FILE *hist_file = fopen(hist.path, "w+");
	fprintf(hist_file, "%zu\n", hist.end);
	for (size_t i = 0; i < HIST_CAP && hist.buff[i][0] != '\0'; ++i) {
		fputs(hist.buff[i], hist_file);
		fputc('\n', hist_file);
	}
	fclose(hist_file);
}

static void clear_key_buffer(char *key, size_t key_cap)
{
	for (size_t i = 0; i < key_cap; ++i) {
		key[i] = '\0';
	}
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
	buffer->cursor = buffer->end = buffer->text;
	size_t hist_index = hist.end, buffer_len;

	char key[KEY_CAP] = {0};
	bool at_beginning = false;
	char temp_buffer[BUFF_CAP] = {0};

	write(STDOUT_FILENO, hush_prompt, hush_prompt_len);
	while (true) {
		clear_key_buffer(key, KEY_CAP);
		read(STDIN_FILENO, key, KEY_CAP);
		buffer_len = buffer->end - buffer->text;
		switch (key[0]) {
			// TODO: Handle control-C with signal.h?
			case '\004': { // Control-D
				write(STDOUT_FILENO, "\n", 1);
				return false;
			}
			// TODO: Figure out tabs
			case '\011': { // Tab
				break;
			}
			case '\012': { // New line
				write(STDOUT_FILENO, "\n", 1);
				buffer->cursor = buffer->text;
				*buffer->end = '\0';
				if (buffer->end != buffer->text) {
					strncpy(hist.buff[hist.end++], buffer->text, BUFF_CAP);
					hist.end %= HIST_CAP;
				}
				return true;
			}
			case '\033': {
				if (key[1] == '[') {
					switch (key[2]) {
						// TODO: Refactor this code to just have buffer->text point to the array entries instead of copying them
						case 'A': { // Up arrow
							if (at_beginning) {
								break;
							}

							// Save current buffer for later use
							*buffer->end = '\0';
							if (hist_index == hist.end) {
								memcpy(temp_buffer, buffer->text, buffer_len);
								temp_buffer[buffer_len] = '\0';
							}

							// Decrement hist_index
							if (hist_index == 0) {
								if (hist.buff[HIST_CAP - 1][0] == '\0') {
									break;
								}
								hist_index = HIST_CAP;
							}
							if (--hist_index == hist.end) {
								at_beginning = true;
							}

							clear_terminal_prompt(buffer_len);

							char *prev_line = hist.buff[hist_index];
							size_t prev_line_len = strlen(prev_line);
							write(STDOUT_FILENO, prev_line, prev_line_len);
							strncpy(buffer->text, prev_line, prev_line_len);
							buffer->cursor = buffer->end = buffer->text + prev_line_len;
							break;
						}
						case 'B': { // Down arrow

							// Increment hist_index
							if (hist_index == hist.end && !at_beginning) {
								break;
							}
							at_beginning = false;
							hist_index = hist_index == HIST_CAP - 1 ? 0 : hist_index + 1;

							clear_terminal_prompt(buffer_len);

							char *next_line = hist.buff[hist_index];
							size_t next_line_len = strlen(next_line);

							// Pull previous buffer for current use
							*buffer->end = '\0';
							if (hist_index == hist.end) {
								next_line = temp_buffer;
								next_line_len = strlen(temp_buffer);
							}

							write(STDOUT_FILENO, next_line, next_line_len);
							strncpy(buffer->text, next_line, next_line_len);
							buffer->cursor = buffer->end = buffer->text + next_line_len;
							break;
						}
						case 'C': { // Right arrow
							if (buffer->cursor < buffer->end) {
								write(STDOUT_FILENO, buffer->cursor, 1);
								++buffer->cursor;
							}
							break;
						}
						case 'D': { // Left arrow
							if (buffer->cursor > buffer->text) {
								write(STDOUT_FILENO, "\b", 1);
								--buffer->cursor;
							}
						}
					}
				} else if (key_buffer_is_empty(&key[1], KEY_CAP - 1)) { // Escape key (clear line)
					clear_terminal_prompt(buffer_len);
					memset(buffer->text, 0, BUFF_CAP);
					buffer->cursor = buffer->end = buffer->text;
					hist_index = hist.end;
				}
				break;
			}
			case '\177': { // Backspace
				if (buffer->cursor > buffer->text) {
					write(STDOUT_FILENO, "\b", 1);
					printf("buffer->end = '%c`", *buffer->end);
					write(STDOUT_FILENO, buffer->cursor, buffer->end - buffer->cursor);
					write(STDOUT_FILENO, " ", 1);
					for (size_t i = 0; i < (size_t) (buffer->end - buffer->cursor + 1); ++i) {
						write(STDOUT_FILENO, "\b", 1);
					}
					memmove(buffer->cursor - 1, buffer->cursor, buffer->end - buffer->cursor + 1);
					--buffer->cursor;
					--buffer->end;
				}
				break;
			}
			default: { // Printable character
				if (buffer_len < BUFF_CAP - 1 && isprint(key[0])) { // Need space for the null terminator
					write(STDOUT_FILENO, key, 1);
					if (buffer->cursor != buffer->end) {
						write(STDOUT_FILENO, buffer->cursor, buffer->end - buffer->cursor + 1);
						for (size_t i = 0; i < (size_t) (buffer->end - buffer->cursor); ++i) {
							write(STDOUT_FILENO, "\b", 1);
						}
						memmove(buffer->cursor + 1, buffer->cursor, buffer->end - buffer->cursor + 1);
					}
					*buffer->cursor = key[0];
					++buffer->cursor;
					++buffer->end;
				}
			}
		}
	}
}
