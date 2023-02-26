#include "common.h"

void sv_print_content(String_View sv)
{
	for(size_t i = 0; i < sv.size; ++i){
		fputc((int) sv.content[i], stdout);
	}
	printf("\n");
}

void sv_print(String_View sv)
{
	printf("Size = %lu\nContent: [", sv.size);
	for(size_t current = 1; current <= sv.size; ++current){
		printf("%c", *(sv.content + current - 1));
	}
	printf("]\n");
}

String_View sv_from_str(const char *str)
{
	String_View sv = {
		.size = strlen(str),
		.content = str
	};
	return sv;
}

char *sv_to_str(String_View sv){
	char *result = (char *) malloc(sv.size * sizeof (char));
	memcpy(result, sv.content, sv.size);
	return result;
}

String_View sv_trim_left(String_View sv)
{
	size_t current = 0;
	while(current < sv.size && isspace((int) *(sv.content + current))){
		++current;
	}
	String_View result = {
		.size = sv.size - current,
		.content = sv.content + current
	};
	return result;
}

String_View sv_trim_right(String_View sv)
{
	size_t current = sv.size;
	while(current > 0 && isspace((int) *(sv.content + current - 1))){
		--current;
	}
	String_View result = {
		.size = current,
		.content = sv.content
	};
	return result;
}

String_View sv_pop_left_delim(String_View *sv, const char delim)
{
	size_t current = 0;
	while(current < sv->size && *(sv->content + current) != delim){
		++current;
	}
	String_View result = {
		.size = current,
		.content = sv->content
	};
	sv->size -= current;
	sv->content += current;
	return result;
}

String_View sv_pop_right_delim(String_View *sv, const char delim)
{
	size_t current = 0;
	while(current < sv->size && *(sv->content + current) != delim){
		++current;
	}
	++current;
	String_View result = {
		.size = current,
		.content = sv->content
	};
	sv->size -= current;
	sv->content += current;
	return result;
}

String_View sv_pop_char(String_View *sv)
{
	String_View result = {
		.size = 1,
		.content = sv->content
	};
	--sv->size;
	++sv->content;
	return result;
}

String_View sv_pop_num_chars(String_View *sv, size_t num)
{
	if(num > sv->size){
		num = sv->size;
	}
	String_View result = {
		.size = num,
		.content = sv->content
	};
	sv->size -= num;
	sv->content += num;
	return result;
}

String_View sv_pop_until_true(String_View *sv, bool (*pred)(char))
{
	size_t current = 0;
	while(current < sv->size && !(*pred)(*(sv->content + current))){
		++current;
	}
	String_View result = {
		.size = current,
		.content = sv->content
	};
	sv->size -= current;
	sv->content += current;
	return result;
}

String_View sv_pop_until_false(String_View *sv, bool (*pred)(char))
{
	size_t current = 0;
	while(current < sv->size && (*pred)(*(sv->content + current))){
		++current;
	}
	String_View result = {
		.size = current,
		.content = sv->content
	};
	sv->size -= current;
	sv->content += current;
	return result;
}
