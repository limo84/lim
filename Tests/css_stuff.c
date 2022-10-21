#include <gtk/gtk.h>

void load_css();

int main() {
  GtkWidget *window;
  GtkWidget *textView;
  
  gtk_init(NULL, NULL);

  load_css();
  
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  g_signal_connect(window, "delete_event", gtk_main_quit, NULL);

  textView = gtk_text_view_new();
  gtk_widget_set_name(textView, "myTV");
  
  gtk_container_add(GTK_CONTAINER(window), textView);
  // g_signal_connect(textView, "clicked", gtk_main_quit, NULL);

  gtk_widget_show_all(window);

  gtk_main();
}

void load_css() {
  GtkCssProvider *provider;
  GdkDisplay *display;
  GdkScreen *screen;

  const gchar *css_style_file = "style.css";
  GFile *css_fp = g_file_new_for_path(css_style_file);
  GError *error;

  provider = gtk_css_provider_new();
  display = gdk_display_get_default();
  screen = gdk_display_get_default_screen(display);

  gtk_style_context_add_provider_for_screen(screen, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  gtk_css_provider_load_from_file(provider, css_fp, &error);

  g_object_unref(provider);
}