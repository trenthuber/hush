#ifndef STRING_VIEW_H_
#define STRING_VIEW_H_

#include <stdbool.h>

typedef struct {
	size_t size;
	const char *content;
} String_View;

void sv_print_content(String_View sv);
void sv_print(String_View sv);
String_View sv_from_str(const char *str);
char *sv_to_str(String_View sv);
String_View sv_trim_left(String_View sv);
String_View sv_trim_right(String_View sv);
String_View sv_pop_left_delim(String_View *sv, char delim);
String_View sv_pop_right_delim(String_View *sv, const char delim);
String_View sv_pop_char(String_View *sv);
String_View sv_pop_num_chars(String_View *sv, size_t num);
String_View sv_pop_until_true(String_View *sv, bool (*pred)(char));
String_View sv_pop_until_false(String_View *sv, bool (*pred)(char));

#endif
