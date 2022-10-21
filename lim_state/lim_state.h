#ifndef LIM_STATE_H
#define LIM_STATE_H

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

typedef struct State {
  GtkWidget* window;
  GtkWidget* textView;
  char* fileName;
} State;


#endif