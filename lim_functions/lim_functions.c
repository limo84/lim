#include <gtk/gtk.h>
#include <stdio.h>

#include "../lim_colorize/lim_colorize.h"
#include "lim_functions.h"

// private functions
void priv_write_text_buffer_to_file(LimState *state);

gboolean on_key_press_test(GtkWidget *modal, GdkEventKey *event, LimState *state)
{
  // printf("keyval: %d, %d\n", event->keyval, event->hardware_keycode);

  if ((event->type == GDK_KEY_PRESS) &&
      (event->state & GDK_CONTROL_MASK))
  {
    switch (event->keyval)
    {
    case GDK_KEY_q:
      gtk_main_quit();
      break;

    case GDK_KEY_t:
      printf("key pressed in Modal: t\n");
      break;

    case GDK_KEY_j:
      gtk_widget_destroy(GTK_WIDGET(modal));
      break;

    case GDK_KEY_l:
      lim_text_buffer_load_from_file(gtk_text_view_get_buffer(GTK_TEXT_VIEW(state->textView)), "testfile.txt", state);
      gtk_widget_destroy(GTK_WIDGET(modal));
      break;

    default:
      return FALSE;
    }
    return FALSE;
  }

  if (event->type == GDK_KEY_PRESS)
  {
    switch (event->keyval)
    {
    case GDK_KEY_Return:
      printf("isBin: %d\n", GTK_IS_BIN(modal));
      printf("Modal name: %s\n", gtk_widget_get_name(modal));
      GtkWidget *child = gtk_bin_get_child(GTK_BIN(modal));
      printf("Child name: %s\n", gtk_widget_get_name(child));

      printf("Child: isBin: %d\n", GTK_IS_BIN(child));
      printf("Child: isBin: %d\n", GTK_IS_ENTRY(child));
      printf("Child: isBin: %d\n", GTK_IS_ENTRY_BUFFER(child));
      printf("Child: isBin: %d\n", GTK_IS_BOX(child));
      printf("Child: isBin: %d\n", GTK_IS_CONTAINER(child));

      GList *children = gtk_container_get_children(GTK_CONTAINER(child));
      printf("Child2: %d\n", GTK_IS_BIN(children->data));
      printf("Child2: %s\n", gtk_widget_get_name(children->data));

      GtkWidget *input = children->data;
      GtkEntryBuffer *buffer = gtk_entry_get_buffer(input);
      char* test = gtk_entry_buffer_get_text(buffer);

      printf("buffer: %s\n", test);

      state->fileName = test;

      // printf("Child2: %s\n", gtk_widget_get_name(g_list_next(children)));


      // GtkWidget *entry = 


      lim_text_buffer_load_from_file(gtk_text_view_get_buffer(GTK_TEXT_VIEW(state->textView)), test, state);
      lim_colorize_c_file(state->textView);
      gtk_widget_destroy(GTK_WIDGET(modal));
      break;

    case GDK_KEY_Escape:
      gtk_widget_destroy(GTK_WIDGET(modal));
      break;

    default:
      return FALSE;
    }
    return FALSE;
  }

  // printf("huhu: %i\n", event->state);
}

void lim_open_dialog(LimState *state)
{
  GtkWidget *modal = gtk_window_new(GTK_WINDOW_POPUP);
  gtk_window_set_default_size(GTK_WINDOW(modal), 400, 50);
  gtk_window_set_position(GTK_WINDOW(modal), GTK_WIN_POS_CENTER);
  gtk_window_set_modal(GTK_WINDOW(modal), 1);

  GtkEntryBuffer *buffer = gtk_entry_buffer_new(state->fileName, -1);
  GtkWidget *input = gtk_entry_new_with_buffer(buffer);
  gtk_widget_set_name(input, "input");

  GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start(GTK_BOX(hbox), input, TRUE, TRUE, 0);
  gtk_container_add(GTK_CONTAINER(modal), hbox);

  // GtkTextIter iter;
  // gtk_text_buffer_get_iter_at_offset(buffer, &iter, 0);
  // // gtk_text_buffer_get_iter_at_mark(buffer, &iter, gtk_text_buffer_get_insert(buffer));
  // gtk_text_buffer_place_cursor(buffer, &iter);

  // gtk_window_set_attached_to(test, state->window);
  g_signal_connect(modal, "key_press_event", G_CALLBACK(on_key_press_test), state);

  // printf("isModal: %d\n", gtk_window_get_modal(GTK_WINDOW(modal)));
  // printf("isContainer: %d\n", GTK_IS_CONTAINER(modal));
  // printf("isBin: %d\n", GTK_IS_BIN(modal));

  gtk_widget_show_all(GTK_WIDGET(modal));
}

void lim_text_buffer_load_from_file(GtkTextBuffer *buffer, char *fileName, LimState *state)
{
  if (!fileName)
  {
    gtk_text_buffer_set_text(buffer, "", -1);
    return;
  }

  char *source = NULL;
  FILE *fp = fopen(fileName, "r");
  if (fp != NULL)
  {
    /* Go to the end of the file. */
    if (fseek(fp, 0L, SEEK_END) == 0)
    {
      /* Get the size of the file. */
      long bufsize = ftell(fp);
      if (bufsize == -1)
      { /* Error */
      }

      /* Allocate our buffer to that size. */
      source = malloc(sizeof(char) * (bufsize + 1));

      /* Go back to the start of the file. */
      if (fseek(fp, 0L, SEEK_SET) != 0)
      { /* Error */
      }

      /* Read the entire file into memory. */
      size_t newLen = fread(source, sizeof(char), bufsize, fp);
      if (ferror(fp) != 0)
      {
        fputs("Error reading file", stderr);
      }
      else
      {
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

void lim_text_buffer_save(LimState *state)
{
  printf("FILENAME: %s\n", state->fileName);
  if (state->fileName != NULL)
  {
    priv_write_text_buffer_to_file(state);
  }
}

void lim_text_buffer_save_as(LimState *state)
{
  printf("DEPRECATED... Filename: %s\n", state->fileName);
  // char *filename;

  // GtkWidget *dialog;
  // dialog = gtk_file_chooser_dialog_new("Abspeichern...",
  //                                      NULL,
  //                                      GTK_FILE_CHOOSER_ACTION_SAVE,
  //                                      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
  //                                      GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
  //                                      NULL);
  // if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
  // {
  //   filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
  // }
  // state->fileName = filename;

  // priv_write_text_buffer_to_file(state);
  // gtk_widget_destroy(dialog);
}

void priv_write_text_buffer_to_file(LimState *state)
{
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

  if (result == FALSE)
  {
    /* error saving file, show message to user */
    printf("[SAVE] %s\n", err->message);
  }

  g_free(text);
  free(err);
}

void lim_move_cursor_to(GtkWidget *textView, int x, int y)
{
  GtkTextIter iter;
  GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textView));
  gtk_text_buffer_get_iter_at_offset(buffer, &iter, x);
  // gtk_text_buffer_get_iter_at_mark(buffer, &iter, gtk_text_buffer_get_insert(buffer));
  gtk_text_buffer_place_cursor(buffer, &iter);
}

void lim_move_cursor_by(LimState *state, int x, int y)
{
  GtkTextIter iter;
  GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(state->textView));
  gtk_text_buffer_get_iter_at_mark(buffer, &iter, gtk_text_buffer_get_insert(buffer));

  uint8_t maxLine = gtk_text_buffer_get_line_count(buffer) - 1;

  if (x > 0)
  {
    gtk_text_iter_forward_cursor_positions(&iter, x);
    state->offset = gtk_text_iter_get_line_offset(&iter);
    printf("offset: %i\n", state->offset);
  }

  else if (x < 0)
  {
    gtk_text_iter_backward_cursor_positions(&iter, -x);
    state->offset = gtk_text_iter_get_line_offset(&iter);
    printf("offset: %i\n", state->offset);
  }

  if (y > 0)
  {
    uint8_t currentLine = gtk_text_iter_get_line(&iter);
    y = y < (maxLine - currentLine) ? y : (maxLine - currentLine);
    gtk_text_iter_forward_lines(&iter, y);
    uint8_t newLine = gtk_text_iter_get_line(&iter);
    if (newLine < maxLine)
    {
      uint8_t maxOffset = gtk_text_iter_get_chars_in_line(&iter) - 1;
      uint8_t newOffset = state->offset > maxOffset ? maxOffset : state->offset;
      gtk_text_iter_set_line_offset(&iter, newOffset);
    }
    // printf("maxOffset: %i\n", maxOffset);
    //   printf("line: %i\n", currentLine);
    //   printf("max lines: %i\n", maxLine);
  }

  else if (y < 0)
  {
    gtk_text_iter_forward_lines(&iter, y);
    uint8_t maxOffset = gtk_text_iter_get_chars_in_line(&iter) - 1;
    printf("maxOffset: %i\n", maxOffset);
    uint8_t newOffset = state->offset > maxOffset ? maxOffset : state->offset;
    gtk_text_iter_set_line_offset(&iter, newOffset);
  }

  gtk_text_buffer_place_cursor(buffer, &iter);
}
