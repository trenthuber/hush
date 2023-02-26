#ifndef HUSH_INTERFACE_H_
#define HUSH_INTERFACE_H_

#include <stdbool.h>

void hi_init_terminal(int file_des);
void hi_release_terminal(int file_des);
bool hi_fill_buffer(int file_des, char *buffer, size_t buffer_cap);

#endif
