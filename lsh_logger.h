#ifndef lsh_logger_h
#define lsh_logger_h

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

FILE *file = NULL;

void logger_set_filename(const char *file_name);
void logger_log(const char *format, ...);

#ifdef LSH

void logger_open_logfile(const char *file_name) {
  file = fopen(file_name, "w");
  if (!file)
    exit(1);
  printf("%s\n", file_name);
}

void logger_log(const char *format, ...) {
  va_list args;
  va_start(args, format);
    vfprintf(file, format, args);
  va_end(args);
}

#endif
#endif
