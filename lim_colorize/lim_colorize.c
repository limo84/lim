#include <gtk/gtk.h>
#include <stdio.h>

#include "lim_colorize.h"

void lim_colorize_c_file(GtkWidget *textView) {
  GtkTextIter start, end, startFile, endFile;
  GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textView));

  GtkTextTag *tag = gtk_text_buffer_create_tag(buffer, "keyword", "foreground", "#8bcada", NULL);
  GtkTextTag *tag2 = gtk_text_buffer_create_tag(buffer, "preprocessor", "foreground", "#D4DA8B", NULL);

  const char *keywords1[] = {"int", "char", "void", "short", "return", "switch", "case", "default", "break"};
  #define n_array (sizeof(keywords1) / sizeof(const char *))

  const char *keywords2[] = {"#include", "#define"};
  #define k_array (sizeof(keywords2) / sizeof(const char *))

  gtk_text_buffer_get_start_iter(buffer, &startFile);
  gtk_text_buffer_get_end_iter(buffer, &endFile);

  start = startFile;

  while (!gtk_text_iter_starts_word(&start))
    gtk_text_iter_forward_char(&start);

  end = start;

  while (gtk_text_iter_forward_word_end(&end)) {
    const gchar *word = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);

    // printf("%s", word);

    for (int i = 0; i < n_array; i++) {
      if (strcmp(keywords1[i], word) == 0) {
        gtk_text_buffer_apply_tag_by_name(buffer, "keyword", &start, &end);

        // gtk_text_buffer_apply_tag(buffer, tag, &start, &end);
        // printf(" <- highlight");
      }
    }

    for (int i = 0; i < k_array; i++) {
      if (strcmp(keywords2[i], word) == 0) {
        gtk_text_buffer_apply_tag_by_name(buffer, "preprocessor", &start, &end);
      }
    }

    // printf("\n");

    start = end;
    while (!gtk_text_iter_starts_word(&start)) {
      gtk_text_iter_forward_char(&start);
      if (gtk_text_iter_equal(&start, &endFile)) {
        return;
      }
    }
  }
}