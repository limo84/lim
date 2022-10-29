#ifndef lim_functions_h
#define lim_functions_h

#include <gtk/gtk.h>

#include "../lim_state/lim_state.h"

void lim_text_buffer_load_from_file(GtkTextBuffer *buffer, char *fileName, LimState* state);

void lim_text_buffer_save_to_file(GtkWidget *textView, LimState *state);

void lim_move_cursor_to(GtkWidget *textView, int x, int y);

void lim_move_cursor_by(GtkWidget *textView, int x, int y);

#endif