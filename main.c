#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "lim_colorize/lim_colorize.h"
#include "lim_functions/lim_functions.h"
#include "lim_state/lim_state.h"

void load_css();

void close_window() {
  gtk_main_quit();
}

gboolean on_key_press(GtkWidget *textView, GdkEventKey *event, LimState *state) {
  // printf("keyval: %d, %d\n", event->keyval, event->hardware_keycode);
  
  if ((event->type == GDK_KEY_PRESS) &&
      (event->state == 5)) {
    printf("state: %i, keycode: %i\n", event->state, event->keyval);
    switch (event->keyval) {
      case GDK_KEY_S:
        printf("hello\n");
        lim_text_buffer_save_as(state);
        break;
      case 214:  // ö
        lim_move_cursor_by(state, 5, 0);
        break;
      case GDK_KEY_J:
        lim_move_cursor_by(state, -5, 0);
        break;
      case GDK_KEY_K:
        lim_move_cursor_by(state, 0, 5);
        break;
      case GDK_KEY_L:
        lim_move_cursor_by(state, 0, -5);
        break;
      default:
        return FALSE;
    }
    return FALSE;
  }
  
  if ((event->type == GDK_KEY_PRESS) &&
      (event->state & GDK_CONTROL_MASK)) {
    switch (event->keyval) {
      case 246:  // ö
        lim_move_cursor_by(state, 1, 0);
        break;
      case GDK_KEY_j:
        lim_move_cursor_by(state, -1, 0);
        break;
      case GDK_KEY_k:
        lim_move_cursor_by(state, 0, 1);
        break;
      case GDK_KEY_l:
        lim_move_cursor_by(state, 0, -1);
        break;
      case GDK_KEY_s:
        lim_text_buffer_save(state);
        break;
      default:
        return FALSE;
    }
    return FALSE;
  }

  // printf("huhu: %i\n", event->state);

  
}

void make_window(LimState *state) {
  /** CREATE WINDOW */
  state->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_position(GTK_WINDOW(state->window), GTK_WIN_POS_CENTER);
  gtk_window_set_default_size(GTK_WINDOW(state->window), 400, 400);
  gtk_window_set_title(GTK_WINDOW(state->window), "lim");

  /** CREATE TEXTVIEW */
  state->textView = gtk_text_view_new();
  gtk_widget_set_name(state->textView, "myTV");
  gtk_text_view_set_left_margin(GTK_TEXT_VIEW(state->textView), 5);

  /** CREATE AND FILL TEXTBUFFER */
  GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(state->textView));
  lim_text_buffer_load_from_file(buffer, state->fileName, state);
  lim_move_cursor_to(state->textView, 0, 0);

  /** DISABLE CURSOR BLINK */
  GtkSettings *settings = gtk_widget_get_settings(state->textView);
  g_object_set(gtk_settings_get_default(), "gtk-cursor-blink", FALSE, NULL);

  /** CONNECT SIGNALS */
  g_signal_connect(state->window, "destroy", G_CALLBACK(close_window), NULL);
  g_signal_connect(state->textView, "key_press_event", G_CALLBACK(on_key_press), state);

  /** CREATE LINE NUMBERS TEXTVIEW */
  // todo

  /** CREATE SCROLLED WINDOW AND ADD TEXTVIEW */
  GtkWidget *scrolledWindow = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_min_content_width(scrolledWindow, 1000);
  gtk_scrolled_window_set_min_content_height(scrolledWindow, 800);
  gtk_container_add(GTK_CONTAINER(scrolledWindow), state->textView);

  /** CREATE BOX AND ADD SCROLLED WINDOW */
  GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_container_add(GTK_CONTAINER(state->window), hbox);
  gtk_box_pack_start(GTK_BOX(hbox), scrolledWindow, TRUE, TRUE, 0);

  /** CODE COLORING */
  lim_colorize_c_file(state->textView);

  gtk_widget_show_all(state->window);
}

int main(int argc, char *argv[]) {
  LimState *state = malloc(sizeof(LimState));

  gtk_init(&argc, &argv);

  for (int i = 0; i < argc; i++) {
    printf("%i. Parameter: %s\n", i, argv[i]);
  }

  if (argc > 2) {
    printf("TOO MUUCH!\n");
    exit(1);
  }

  load_css();

  state->fileName = argv[1];
  make_window(state);
  gtk_window_resize(GTK_WINDOW(state->window), 400, 400);

  gtk_main();

  return 0;
}

void load_css() {
  GtkCssProvider *provider;
  GdkDisplay *display;
  GdkScreen *screen;

  const gchar *css_style_file = "main.css";
  GFile *css_fp = g_file_new_for_path(css_style_file);
  GError *error = NULL;

  provider = gtk_css_provider_new();
  display = gdk_display_get_default();
  screen = gdk_display_get_default_screen(display);

  gtk_style_context_add_provider_for_screen(screen, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  gboolean css_successfull = gtk_css_provider_load_from_file(provider, css_fp, &error);

  if (error != NULL) {
    fprintf(stderr, "[ERROR]: %s\n", error->message);
    g_error_free(error);
  }

  g_object_unref(provider);
}