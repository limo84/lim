#include "functions.h"

#include <gtk/gtk.h>
#include <stdio.h>

void lim_text_buffer_load_from_file(GtkTextBuffer *buffer, char *fileName) {
  char *source = NULL;
  FILE *fp = fopen(fileName, "r");
  if (fp != NULL) {
    /* Go to the end of the file. */
    if (fseek(fp, 0L, SEEK_END) == 0) {
      /* Get the size of the file. */
      long bufsize = ftell(fp);
      if (bufsize == -1) { /* Error */
      }

      /* Allocate our buffer to that size. */
      source = malloc(sizeof(char) * (bufsize + 1));

      /* Go back to the start of the file. */
      if (fseek(fp, 0L, SEEK_SET) != 0) { /* Error */
      }

      /* Read the entire file into memory. */
      size_t newLen = fread(source, sizeof(char), bufsize, fp);
      if (ferror(fp) != 0) {
        fputs("Error reading file", stderr);
      } else {
        source[newLen++] = '\0'; /* Just to be safe. */
      }
    }
  }

  gtk_text_buffer_set_text(buffer, source, -1);
  fclose(fp);
  free(source);
}

void lim_text_buffer_save_to_file(GtkWidget *textView, char *fileName) {

  GtkWidget *dialog;
  dialog = gtk_file_chooser_dialog_new("Abspeichern...",
                                       NULL,
                                       GTK_FILE_CHOOSER_ACTION_SAVE,
                                       GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                       GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
                                       NULL);

  if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
    char *filename;
    char *text;
    GtkTextBuffer* buffer;
    GtkTextIter start;
    GtkTextIter end;
    gboolean result;
    GError *err;
    filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
    gtk_widget_set_sensitive (textView, FALSE);
    buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (textView));
    gtk_text_buffer_get_start_iter(buffer, &start);
    gtk_text_buffer_get_end_iter(buffer, &end);
    text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
    gtk_text_buffer_set_modified(buffer, FALSE);
    gtk_widget_set_sensitive (textView, TRUE);

    /* set the contents of the file to the text from the buffer */
    if (filename != NULL) {
      result = g_file_set_contents(filename, text, -1, &err);
    } else {
      result = g_file_set_contents(filename, text, -1, &err);
    }

    if (result == FALSE) {
      /* error saving file, show message to user */
    }

    g_free(text);
  }
  gtk_widget_destroy(dialog);
}

void lim_move_cursor_to(GtkWidget *textView, int x, int y) {
  GtkTextIter iter;
  GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textView));
  gtk_text_buffer_get_iter_at_offset(buffer, &iter, x);
  // gtk_text_buffer_get_iter_at_mark(buffer, &iter, gtk_text_buffer_get_insert(buffer));
  gtk_text_buffer_place_cursor(buffer, &iter);
}

void lim_move_cursor_by(GtkWidget *textView, int x, int y) {
  GtkTextIter iter;
  GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textView));
  gtk_text_buffer_get_iter_at_mark(buffer, &iter, gtk_text_buffer_get_insert(buffer));

  if (x > 0) {
    gtk_text_iter_forward_cursor_positions(&iter, 1);
  } else if (x < 0) {
    gtk_text_iter_backward_cursor_positions(&iter, 1);
  }

  if (y > 0) {
    gint offset = gtk_text_iter_get_line_offset(&iter);
    gtk_text_iter_forward_line(&iter);
    gtk_text_iter_set_line_offset(&iter, offset);
    // gtk_text_buffer_place_cursor (buffer, &iter);
  } else if (y < 0) {
    gint offset = gtk_text_iter_get_line_offset(&iter);
    gtk_text_iter_backward_line(&iter);
    gtk_text_iter_set_line_offset(&iter, offset);
    // gtk_text_buffer_place_cursor (buffer, &iter);
  }
  gtk_text_buffer_place_cursor(buffer, &iter);
}

void buffer_find_words(GtkWidget *textView) {
  GtkTextIter start, end, startFile, endFile;
  GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textView));

  GtkTextTag *tag = gtk_text_buffer_create_tag(buffer, "keyword", "foreground", "#8bcada", NULL);
  GtkTextTag *tag2 = gtk_text_buffer_create_tag(buffer, "preprocessor", "foreground", "#D4DA8B", NULL);

  const char *keywords1[] = {"int", "char", "void", "short", "return"};
#define n_array (sizeof(keywords1) / sizeof(const char *))

  const char *keywords2[] = {"#include", "#define"};
#define k_array (sizeof(keywords2) / sizeof(const char *))

  gtk_text_buffer_get_start_iter(buffer, &startFile);
  gtk_text_buffer_get_end_iter(buffer, &endFile);

  // GtkTextTag *defaultTag = gtk_text_buffer_create_tag(buffer, "default", "foreground", "#ebdbb2", NULL);
  // gtk_text_tag_set_priority(defaultTag, 0);
  // gtk_text_buffer_apply_tag_by_name(buffer, "default", &startFile, &endFile);

  start = startFile;

  while (!gtk_text_iter_starts_word(&start))
    gtk_text_iter_forward_char(&start);

  end = start;

  while (gtk_text_iter_forward_word_end(&end)) {
    const gchar *word = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);

    printf("%s", word);

    for (int i = 0; i < n_array; i++) {
      if (strcmp(keywords1[i], word) == 0) {
        gtk_text_buffer_apply_tag_by_name(buffer, "keyword", &start, &end);

        // gtk_text_buffer_apply_tag(buffer, tag, &start, &end);
        printf(" <- highlight");
      }
    }

    for (int i = 0; i < k_array; i++) {
      if (strcmp(keywords2[i], word) == 0) {
        gtk_text_buffer_apply_tag_by_name(buffer, "preprocessor", &start, &end);
      }
    }

    printf("\n");

    start = end;
    while (!gtk_text_iter_starts_word(&start)) {
      gtk_text_iter_forward_char(&start);
      if (gtk_text_iter_equal(&start, &endFile)) {
        return;
      }
    }
  }
}