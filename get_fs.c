#include "files.h"

#include <stdio.h>

int main() {
  char **files = NULL;
  int files_len;

  get_file_system(&files, &files_len);
  for (int i = 0; i < files_len; i++) {
    printf("%s\n", files[i]);
  }
  return 0;
}
