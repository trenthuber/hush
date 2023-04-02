#include <ctype.h>

#include "common.h"

static bool ht_isspace(char chr)
{
	return isspace(chr) || chr == '\0';
}

static bool ht_char_is_term(char chr)
{
	return ht_isspace(chr) || chr == '|' || chr == ';' || chr == '>' || chr == '<';
}

char *ht_get_token(Buffer *buff)
{
	if (buff->cursor == buff->end) {
		return NULL;
	}
	char *result;
	switch (buff->text[buff->cursor]) {
		case '|':
		case ';': 
		case '<': {
			if (buff->cursor + 1 == buff->end) {
				return &buff->text[buff->cursor++];
			}
			if (ht_isspace(buff->text[buff->cursor + 1])) {
				buff->text[buff->cursor + 1] = '\0';
				result = &buff->text[buff->cursor];
				buff->cursor += 2;
			} else {
				result = (char *) malloc(2 * sizeof (char));
				if (result == NULL) {
					fprintf(stderr, "hush: unable to allocate memory\n");
					exit(1);
				}
				result[0] = buff->text[buff->cursor];
				result[1] = '\0';
				++buff->cursor;
			}
			break;
		}
		case '>': {
			if (buff->cursor + 1 == buff->end) {
				return &buff->text[buff->cursor++];
			}
			switch (buff->text[buff->cursor + 1]) {
				case '>':
				case '!': {
					if (buff->cursor + 2 == buff->end) {
						buff->cursor += 2;
						return &buff->text[buff->cursor - 2];
					}
					if (ht_isspace(buff->text[buff->cursor + 2])) {
						buff->text[buff->cursor + 2] = '\0';
						result = &buff->text[buff->cursor];
						buff->cursor += 3;
					} else {
						result = (char *) malloc(3 * sizeof (char));
						if (result == NULL) {
							fprintf(stderr, "hush: unable to allocate memory\n");
							exit(1);
						}
						result[0] = '>';
						result[1] = buff->text[buff->cursor + 1];
						result[2] = '\0';
						buff->cursor += 2;
					}
					break;
				}
				default: {
					if (ht_isspace(buff->text[buff->cursor + 1])) {
						buff->text[buff->cursor + 1] = '\0';
						buff->cursor += 2;
						result = &buff->text[buff->cursor - 2];
					} else {
						result = (char *) malloc(2 * sizeof (char));
						if (result == NULL) {
							fprintf(stderr, "hush: unable to allocate memory\n");
							exit(1);
						}
						result[0] = '>';
						result[1] = '\0';
						++buff->cursor;
					}
				}
			}
			break;
		}
		default: {
			size_t begin = buff->cursor;
			for (; buff->cursor < buff->end && !ht_char_is_term(buff->text[buff->cursor]); ++buff->cursor);
// printf("begin = %zu, buff->cursor = %zu, buff->end = %zu\n", begin, buff->cursor, buff->end);
			if (buff->cursor == buff->end) {
				return &buff->text[begin];
			}
			if (ht_isspace(buff->text[buff->cursor])) {
				buff->text[buff->cursor++] = '\0';
				result = &buff->text[begin];
			} else {
				size_t token_len = buff->cursor - begin;
// printf("token length = %zu\n", token_len);
				result = (char *) malloc((token_len + 1) * sizeof (char));
				if (result == NULL) {
					fprintf(stderr, "hush: unable to allocate memory\n");
					exit(1);
				}
				strncpy(result, &buff->text[begin], token_len);
				result[token_len] = '\0';
			}
		}
	}
	return result;
}
