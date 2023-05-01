#include "lim_colorize.h"

#include <gtk/gtk.h>
#include <stdio.h>

void lim_colorize_c_file(GtkWidget *textView) {
  gboolean printTokens = FALSE;
  gboolean isWord = FALSE;
  gboolean isString = FALSE;
  gboolean isComment = FALSE;

  GtkTextIter start, end, startFile, endFile;
  GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textView));

  GtkTextTag *keywordTag = gtk_text_buffer_create_tag(buffer, "keyword", "foreground", "#8bcada", NULL);
  GtkTextTag *preProcTag = gtk_text_buffer_create_tag(buffer, "preprocessor", "foreground", "#d98555", NULL);
  GtkTextTag *stringTag = gtk_text_buffer_create_tag(buffer, "string", "foreground", "#D4DA8B", NULL);

  const char *keywords[] = {"int", "char", "void", "short", "return", "switch", "case", "default", "break", "for", "if"};
  unsigned int keywords_size = sizeof(keywords) / sizeof(const char *);
  // #define n_array (sizeof(keywords) / sizeof(const char *))

  // const char *keywords2[] = {"#include", "#define"};
  // #define k_array (sizeof(keywords2) / sizeof(const char *))

  gtk_text_buffer_get_start_iter(buffer, &startFile);
  gtk_text_buffer_get_end_iter(buffer, &endFile);

  start = startFile;
  // end = start;

  while (!gtk_text_iter_equal(&start, &endFile)) {
    end = start;
    gtk_text_iter_forward_char(&end);

    // gchar* s = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
    char s = gtk_text_iter_get_char(&start);

    if (s == ' ' || s == 10) {
      gtk_text_iter_forward_char(&start);

      if (printTokens) {
        printf("%c", s);
      }

      continue;
    }

    else if (s == '#') {
      while (gtk_text_iter_get_char(&end) != ' ' && !gtk_text_iter_equal(&end, &endFile)) {
        gtk_text_iter_forward_char(&end);
      }
      const gchar *word = gtk_text_iter_get_slice(&start, &end);

      if (printTokens) {
        printf("[%s]", word);
      }

      gtk_text_buffer_apply_tag_by_name(buffer, "preprocessor", &start, &end);
      start = end;
    }

    else if (s == '"') {
      while (gtk_text_iter_get_char(&end) != '"' && !gtk_text_iter_equal(&end, &endFile)) {
        gtk_text_iter_forward_char(&end);
      }
      gtk_text_iter_forward_char(&end);
      const gchar *word = gtk_text_iter_get_slice(&start, &end);

      if (printTokens) {
        printf("[%s]", word);
      }

      gtk_text_buffer_apply_tag_by_name(buffer, "string", &start, &end);
      start = end;
    }

    else {
      while (!gtk_text_iter_equal(&end, &endFile)) {
        // gtk_text_iter_forward_char(&end);

        switch (gtk_text_iter_get_char(&end)) {
          case ' ':
          case '(':
          case ')':
          case ',':
          case '=':
          case ';':
          case 10:  // newline
            break;

          default:
            gtk_text_iter_forward_char(&end);
            continue;
        }
        break;
      }
      gchar *word = gtk_text_iter_get_slice(&start, &end);
      for (int i = 0; i < keywords_size; i++) {
        if (!strcmp(keywords[i], word)) {
          gtk_text_buffer_apply_tag_by_name(buffer, "keyword", &start, &end);
        }
      }

      if (printTokens) {
        printf("[%s]", word);
      }

      start = end;
    }

    // if (s == '<') {
    //   while (gtk_text_iter_get_char(&end) != '>' && !gtk_text_iter_equal(&end, &endFile)) {
    //     gtk_text_iter_forward_char(&end);
    //   }
    //   gtk_text_iter_forward_char(&end);
    //   const gchar *word = gtk_text_iter_get_slice(&start, &end);
    //   gtk_text_buffer_apply_tag_by_name(buffer, "string", &start, &end);
    //   start = end;
    // }

    // printf("%c ", s);

    gtk_text_iter_forward_char(&start);
  }

  // while (!gtk_text_iter_starts_word(&start))
  //   gtk_text_iter_forward_char(&start);

  // end = start;

  // while (gtk_text_iter_forward_word_end(&end)) {
  //   const gchar *word = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);

  //   // printf("%s", word);

  //   for (int i = 0; i < n_array; i++) {
  //     if (strcmp(keywords1[i], word) == 0) {
  //       gtk_text_buffer_apply_tag_by_name(buffer, "keyword", &start, &end);

  //       // gtk_text_buffer_apply_tag(buffer, tag, &start, &end);
  //       // printf(" <- highlight");
  //     }
  //   }

  //   for (int i = 0; i < k_array; i++) {
  //     if (strcmp(keywords2[i], word) == 0) {
  //       gtk_text_buffer_apply_tag_by_name(buffer, "preprocessor", &start, &end);
  //     }
  //   }

  //   // printf("\n");

  //   start = end;
  //   while (!gtk_text_iter_starts_word(&start)) {
  //     gtk_text_iter_forward_char(&start);
  //     if (gtk_text_iter_equal(&start, &endFile)) {
  //       return;
  //     }
  //   }
  // }
}