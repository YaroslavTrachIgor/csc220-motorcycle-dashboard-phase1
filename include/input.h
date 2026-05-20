/**
 * Phase III: non-blocking stdin / terminal restore.
 */

#ifndef INPUT_H
#define INPUT_H

#include <stdbool.h>

bool input_terminal_setup(void);
void input_terminal_restore(void);

void *input_thread(void *arg);

#endif
