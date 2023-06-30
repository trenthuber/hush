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

char * const hush_prompt = "hush % ";
const size_t hush_prompt_len = strlen(hush_prompt);

typedef struct termios Termios;
static Termios original;

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

static void clear_buffer(Buffer *buff)
{
	memset(buff->text, 0, BUFF_CAP + 1);
	buff->cursor = buff->end = buff->text;
}

typedef struct {
	char *path;
	Buffer buffs[HIST_CAP + 1];
	Buffer *zero;
	Buffer *cap;
	Buffer *start;
	Buffer *current;
	Buffer *end;
} History;
static History hist;

void init_history(void)
{
	// Initialize history buffer structs
	hist.zero = &hist.buffs[0];
	hist.cap = &hist.buffs[HIST_CAP];

	for (hist.current = hist.zero; hist.current <= hist.cap; ++hist.current) {
		clear_buffer(hist.current);
	}

	// Regenerate history filepath name
	struct passwd *user_pw = getpwuid(getuid());
	const char *home_dir = user_pw->pw_dir;
	const char *hist_name = "/.hush_history";
	size_t home_dir_len = strlen(home_dir);
	size_t hist_name_len = strlen(hist_name);
	hist.path = (char *) calloc(home_dir_len + hist_name_len + 1, sizeof (char));
	strncpy(hist.path, home_dir, home_dir_len);
	strncat(hist.path, hist_name, hist_name_len);

	FILE *hist_file = fopen(hist.path, "r");
	if (hist_file == NULL) {
		hist.start = hist.current = hist.end = hist.zero;
		return;
	}
	fseek(hist_file, 0, SEEK_SET);

	// Read history file into hist struct
	size_t i;
	for (i = HIST_CAP; i > 0 && fgets(hist.buffs[i].text, BUFF_CAP + 1, hist_file) != NULL; --i) {

		// Get rid of the newline char
		size_t nl_loc = strnlen(hist.buffs[i].text, BUFF_CAP + 1) - 1;
		hist.buffs[i].text[nl_loc] = '\0';
		hist.buffs[i].cursor = hist.buffs[i].end = &hist.buffs[i].text[nl_loc];
	}
	fclose(hist_file);

	hist.start = &hist.buffs[(i + 1) % (HIST_CAP + 1)];
	hist.current = hist.end = hist.zero;
}

void release_history(void)
{
	if (hist.start == hist.end && hist.end->text == hist.end->end) {
		return;
	}

	FILE *hist_file = fopen(hist.path, "w");
	if (hist.start <= hist.end) {
		for (hist.current = hist.end; hist.current >= hist.start; --hist.current) {
			fprintf(hist_file, "%s\n", hist.current->text);
		}
	} else {
		for (hist.current = hist.end; hist.current >= hist.zero; --hist.current) {
			fprintf(hist_file, "%s\n", hist.current->text);
		}
		for (hist.current = hist.cap; hist.current >= hist.start; --hist.current) {
			fprintf(hist_file, "%s\n", hist.current->text);
		}
	}
	fclose(hist_file);
}

static void clear_prompt(void)
{
	write(STDOUT_FILENO, "\r", 1);
	for (size_t i = 0; i < (hist.current->end - hist.current->text) + hush_prompt_len; ++i) {
		write(STDOUT_FILENO, " ", 1);
	}
	write(STDOUT_FILENO, "\r", 1);
	write(STDOUT_FILENO, hush_prompt, hush_prompt_len);
}

static Buffer result;

Buffer *get_next_buffer(void)
{
	// Set current buffer to the next buffer if current buffer has text
	if (hist.end->text != hist.end->end) {
		hist.end = (hist.end == hist.cap) ? hist.zero : hist.end + 1;
		hist.current = hist.end;
		if (hist.end == hist.start) {
			hist.start = (hist.start == hist.cap) ? hist.zero : hist.start + 1;
		}
	}
	clear_buffer(hist.current);

	write(STDOUT_FILENO, hush_prompt, hush_prompt_len);
	while (true) {
		char key[KEY_CAP] = {0};
		read(STDIN_FILENO, key, KEY_CAP);
		switch (key[0]) {
			// TODO: Handle control-C with signal.h?
			case '\004': { // Control-D
				write(STDOUT_FILENO, "\n", 1);
				if (hist.start != hist.end && hist.end->text == hist.end->end) {
					hist.end = (hist.end == hist.zero) ? hist.cap : hist.end - 1;
				}
				return NULL;
			}
			// TODO: Figure out tabs
			case '\011': { // Tab
				break;
			}
			case '\012': { // New line
				write(STDOUT_FILENO, "\n", 1);

				// Copy current buffer to end of history if you're going to run it
				if (hist.current != hist.end) {
					*hist.end = *hist.current;
					hist.end->cursor = hist.end->text + (hist.current->cursor - hist.current->text);
					hist.end->end = hist.end->text + (hist.current->end - hist.current->text);
					hist.current = hist.end;
				}

				hist.end->cursor = hist.end->end;
				*hist.end->end = '\0';
				result = *hist.end; // Result gets clobbered in lexing
				result.cursor = result.text;
				result.end = result.text + (hist.end->end - hist.end->text);
				return &result;
			}
			case '\033': {
				if (key[1] == '[') {
					switch (key[2]) {
						case 'A': { // Up arrow
							if (hist.current == hist.start) {
								break;
							}
							clear_prompt();

							hist.current->cursor = hist.current->end;
							*hist.current->end = '\0';

							hist.current = (hist.current == hist.zero) ? hist.cap : hist.current - 1;
							
							write(STDOUT_FILENO, hist.current->text, strlen(hist.current->text));
							hist.current->cursor = hist.current->end = hist.current->text + strlen(hist.current->text);
							break;
						}
						case 'B': { // Down arrow
							if (hist.current == hist.end) {
								break;
							}

							clear_prompt();

							hist.current->cursor = hist.current->end;
							*hist.current->end = '\0';

							hist.current = (hist.current == hist.cap) ? hist.zero : hist.current + 1;

							write(STDOUT_FILENO, hist.current->text, strlen(hist.current->text));
							hist.current->cursor = hist.current->end = hist.current->text + strlen(hist.current->text);
							break;
						}
						case 'C': { // Right arrow
							if (hist.current->cursor < hist.current->end) {
								write(STDOUT_FILENO, hist.current->cursor, 1);
								++hist.current->cursor;
							}
							break;
						}
						case 'D': { // Left arrow
							if (hist.current->cursor > hist.current->text) {
								write(STDOUT_FILENO, "\b", 1);
								--hist.current->cursor;
							}
						}
					}
				} else {
					size_t is_not_zero = 0;
					for (size_t i = 1; i < KEY_CAP; ++i) {
						is_not_zero += key[i];
					}
					if (!is_not_zero) { // Escape key
						clear_prompt();
						hist.current = hist.end;
						clear_buffer(hist.end);
					}
				}
				break;
			}
			case '\177': { // Backspace
				if (hist.current->cursor > hist.current->text) {
					write(STDOUT_FILENO, "\b", 1);
					write(STDOUT_FILENO, hist.current->cursor, hist.current->end - hist.current->cursor);
					write(STDOUT_FILENO, " ", 1);
					for (size_t i = 0; i < (size_t) (hist.current->end - hist.current->cursor + 1); ++i) {
						write(STDOUT_FILENO, "\b", 1);
					}

					if (hist.current != hist.end) {
						*hist.end = *hist.current;
						hist.end->cursor = hist.end->text + (hist.current->cursor - hist.current->text);
						hist.end->end = hist.end->text + (hist.current->end - hist.current->text);
						hist.current = hist.end;
					}
					memmove(hist.end->cursor - 1, hist.end->cursor, hist.end->end - hist.end->cursor + 1);
					--hist.end->cursor;
					--hist.end->end;
				}
				break;
			}
			default: { // Printable character
				if (hist.current->end - hist.current->text < BUFF_CAP && isprint(key[0])) {
					write(STDOUT_FILENO, key, 1);

					if (hist.current != hist.end) {
						*hist.end = *hist.current;
						hist.end->cursor = hist.end->text + (hist.current->cursor - hist.current->text);
						hist.end->end = hist.end->text + (hist.current->end - hist.current->text);
						hist.current = hist.end;
					}
					if (hist.end->cursor != hist.end->end) {
						write(STDOUT_FILENO, hist.end->cursor, hist.end->end - hist.end->cursor + 1);
						for (size_t i = 0; i < (size_t) (hist.end->end - hist.end->cursor); ++i) {
							write(STDOUT_FILENO, "\b", 1);
						}
						memmove(hist.end->cursor + 1, hist.end->cursor, hist.end->end - hist.end->cursor + 1);
					}
					*hist.end->cursor = key[0];
					++hist.end->cursor;
					++hist.end->end;
				}
			}
		}
	}
}
