#ifndef LIM_FUNCTIONS_H
#define LIM_FUNCTIONS_H

#include <gtk/gtk.h>

#include "../lim_state/lim_state.h"

void lim_text_buffer_load_from_file(GtkTextBuffer *buffer, char *fileName, LimState *state);

void lim_text_buffer_save_as(LimState *state);

void lim_text_buffer_save(LimState *state);

void lim_move_cursor_to(GtkWidget *textView, int x, int y);

void lim_move_cursor_by(LimState *state, int x, int y);

void lim_open_dialog(LimState *state);

#endif