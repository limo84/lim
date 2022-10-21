#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "functions.h"

typedef struct State {
  GtkWidget* window;
  GtkWidget* textView;
  char* fileName;
} State;

GtkWidget *textView;

void load_css();

void close_window() {
  gtk_main_quit();
}


gboolean on_key_press(GtkWidget *textView, GdkEventKey *event, gpointer user_data) {
  printf("keyval: %d, %d\n", event->keyval, event->hardware_keycode);
  if ((event->type == GDK_KEY_PRESS) &&
      (event->state & GDK_CONTROL_MASK)) {
    switch (event->keyval) {
      case 246:  // ö
        printf("key pressed: %s\n", "ctrl + ö");
        lim_move_cursor_by(textView, 1, 0);
        break;
      case GDK_KEY_j:
        printf("key pressed: %s\n", "ctrl + j");
        lim_move_cursor_by(textView, -1, 0);
        break;
      case GDK_KEY_k:
        lim_move_cursor_by(textView, 0, 1);
        break;
      case GDK_KEY_l:
        lim_move_cursor_by(textView, 0, -1);
        break;
      case GDK_KEY_s:
        printf("key pressed: %s\n", "ctrl + s");
        lim_text_buffer_save_to_file(textView, "testfile.txt");
        break;
      default:
        return FALSE;
    }
    return FALSE;
  }
}

void make_window(char* fileName, State* state) {
  GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
  gtk_window_set_default_size(GTK_WINDOW(window), 400, 400);
  gtk_window_set_title(GTK_WINDOW(window), "lim");

  g_signal_connect(window, "destroy", G_CALLBACK(close_window), NULL);
  // g_signal_connect(window, "key_press_event", G_CALLBACK(on_key_press), NULL);

  GtkWidget *textView = gtk_text_view_new();
  gtk_widget_set_name(textView, "myTV");
  GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textView));
  // gtk_style_context_add_class(gtk_widget_get_style_context(textView), "text");

  GtkSettings *settings = gtk_widget_get_settings(textView);
  g_object_set (gtk_settings_get_default (), "gtk-cursor-blink", FALSE, NULL);

  g_signal_connect(textView, "key_press_event", G_CALLBACK(on_key_press), NULL);
  lim_text_buffer_load_from_file(buffer, fileName);
  lim_move_cursor_to(textView, 0, 0);



  GtkTextIter start, end;
  PangoFontDescription *font_desc;
  GdkRGBA rgba;
  GtkTextTag *tag;

  /* Change left margin throughout the widget */
  gtk_text_view_set_left_margin(GTK_TEXT_VIEW(textView), 30);

  /* Change left margin throughout the widget */
  gtk_text_view_set_left_margin(GTK_TEXT_VIEW(textView), 5);


  GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add(GTK_CONTAINER(window), vbox);
  gtk_box_pack_start(GTK_BOX(vbox), textView, TRUE, TRUE, 0);

  
  // buffer_find_words(textView);

  gtk_widget_show_all(window);
}

int main(int argc, char *argv[]) {
  State *state = malloc(sizeof(State));

  gtk_init(&argc, &argv);

  for (int i = 0; i < argc; i++) {
    printf("%i. Parameter: %s\n", i, argv[i]);
  }

  if (argc < 2) {
    printf("GIVE ME FILE\n");
    exit(1);
  }

  if (argc > 2) {
    printf("TOO MUUCH!\n");
    exit(1);
  }

  load_css();

  make_window(argv[1], state);

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
    fprintf (stderr, "[ERROR]: %s\n", error->message);
    g_error_free (error);
  }

  g_object_unref(provider);
}