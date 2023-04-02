#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUFF_CAP 4096

typedef struct {
	char text[BUFF_CAP];
	size_t cursor;
	size_t end;
} Buffer;

