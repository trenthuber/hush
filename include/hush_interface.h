#ifndef HUSH_INTERFACE_H_
#define HUSH_INTERFACE_H_

#include <stdbool.h>

void hi_init_terminal(void);
void hi_release_terminal(void);
void hi_init_history(void);
void hi_release_history(void);
bool hi_fill_buffer(char *buffer);

#endif
