#ifndef BUFFER_H_
#define BUFFER_H_

#define BUFF_CAP 4096

typedef struct {
	char text[BUFF_CAP];
	char *cursor;
	char *end;
} Buffer;

void init_terminal(void);
void release_terminal(void);
void init_history(void);
void release_history(void);
bool fill_buffer(Buffer *buffer);

#endif // BUFFER_H_
