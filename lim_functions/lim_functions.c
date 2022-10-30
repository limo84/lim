#include "lim_functions.h"

#include <gtk/gtk.h>
#include <stdio.h>

// private functions
void priv_write_text_buffer_to_file(LimState *state);

void lim_text_buffer_load_from_file(GtkTextBuffer *buffer, char *fileName, LimState *state) {
  if (!fileName) {
    gtk_text_buffer_set_text(buffer, "", -1);
    return;
  }

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
  char window_name[255];
  strcpy(window_name, "lim - ");
  strcat(window_name, state->fileName);
  gtk_window_set_title(GTK_WINDOW(state->window), window_name);

  fclose(fp);
  free(source);
}

void lim_text_buffer_save(LimState *state) {
  printf("FILENAME: %s\n", state->fileName);
  if (state->fileName != NULL) {
    priv_write_text_buffer_to_file(state);
  }
}

void lim_text_buffer_save_as(LimState *state) {
  printf("FILENAME: %s\n", state->fileName);
  char *filename;

  GtkWidget *dialog;
  dialog = gtk_file_chooser_dialog_new("Abspeichern...",
                                       NULL,
                                       GTK_FILE_CHOOSER_ACTION_SAVE,
                                       GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                       GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
                                       NULL);
  if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
    filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
  }
  state->fileName = filename;

  priv_write_text_buffer_to_file(state);
  gtk_widget_destroy(dialog);
}

void priv_write_text_buffer_to_file(LimState *state) {
  char *text;
  gboolean result;
  GError *err = NULL;
  GtkTextBuffer *buffer;

  GtkTextIter start;
  GtkTextIter end;

  gtk_widget_set_sensitive(state->textView, FALSE);
  buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(state->textView));
  gtk_text_buffer_get_start_iter(buffer, &start);
  gtk_text_buffer_get_end_iter(buffer, &end);
  text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
  // gtk_text_buffer_set_modified(buffer, FALSE);
  gtk_widget_set_sensitive(state->textView, TRUE);

  /* set the contents of the file to the text from the buffer */
  if (state->fileName != NULL)
    result = g_file_set_contents(state->fileName, text, -1, &err);
  else
    result = g_file_set_contents(state->fileName, text, -1, &err);

  if (result == FALSE) {
    /* error saving file, show message to user */
    printf("[SAVE] %s\n", err->message);
  }

  g_free(text);
  free(err);
}

void lim_move_cursor_to(GtkWidget *textView, int x, int y) {
  GtkTextIter iter;
  GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textView));
  gtk_text_buffer_get_iter_at_offset(buffer, &iter, x);
  // gtk_text_buffer_get_iter_at_mark(buffer, &iter, gtk_text_buffer_get_insert(buffer));
  gtk_text_buffer_place_cursor(buffer, &iter);
}

void lim_move_cursor_by(LimState *state, int x, int y) {
  GtkTextIter iter;
  GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(state->textView));
  gtk_text_buffer_get_iter_at_mark(buffer, &iter, gtk_text_buffer_get_insert(buffer));

  uint8_t maxLine = gtk_text_buffer_get_line_count(buffer) - 1;

  if (x > 0) {
    gtk_text_iter_forward_cursor_positions(&iter, x);
    state->offset = gtk_text_iter_get_line_offset(&iter);
    printf("offset: %i\n", state->offset);
  }

  else if (x < 0) {
    gtk_text_iter_backward_cursor_positions(&iter, -x);
    state->offset = gtk_text_iter_get_line_offset(&iter);
    printf("offset: %i\n", state->offset);
  }

  if (y > 0) {
    uint8_t currentLine = gtk_text_iter_get_line(&iter);
    y = y < (maxLine - currentLine) ? y : (maxLine - currentLine);
    gtk_text_iter_forward_lines(&iter, y);
    uint8_t newLine = gtk_text_iter_get_line(&iter);
    if (newLine < maxLine) {
      uint8_t maxOffset = gtk_text_iter_get_chars_in_line(&iter) - 1;
      uint8_t newOffset = state->offset > maxOffset ? maxOffset : state->offset;
      gtk_text_iter_set_line_offset(&iter, newOffset);
    }
    // printf("maxOffset: %i\n", maxOffset);
    //   printf("line: %i\n", currentLine);
    //   printf("max lines: %i\n", maxLine);
  }

  else if (y < 0) {
    gtk_text_iter_forward_lines(&iter, y);
    uint8_t maxOffset = gtk_text_iter_get_chars_in_line(&iter) - 1;
    printf("maxOffset: %i\n", maxOffset);
    uint8_t newOffset = state->offset > maxOffset ? maxOffset : state->offset;
    gtk_text_iter_set_line_offset(&iter, newOffset);
  }

  gtk_text_buffer_place_cursor(buffer, &iter);
}
