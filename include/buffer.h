#ifndef BUFFER_H_
#define BUFFER_H_

#define BUFF_CAP 100
#define HIST_CAP 1000

typedef struct {
	char text[BUFF_CAP + 1];
	char *cursor;
	char *end;
} Buffer;

void init_terminal(void);
void release_terminal(void);
void init_history(void);
void release_history(void);
Buffer *get_next_buffer(void);

#endif // BUFFER_H_
