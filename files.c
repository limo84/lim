#include "files.h"

#include <stdio.h>
#include <stdlib.h>
#include <ftw.h>
#include <stdint.h>
#include <string.h>
#include <libgen.h>
#include <unistd.h>

#define MAX_FILES 1000 // Maximum number of files to store
#define PATH_MAX 4096
#define LK_TICK 39

// Global array to store file paths
char **file_paths = NULL;
int file_count = 0;
char cwd[PATH_MAX];
uint16_t cwd_len = 0;

// Callback function to add file paths to the array
int list_callback(const char *fpath, const struct stat *sb, int typeflag) {
  if (typeflag == FTW_F && file_count < MAX_FILES) {
    
    int path_len = strlen(fpath);
    char filename[PATH_MAX];
    char *ext;
    strcpy(filename, fpath + cwd_len);
    ext = strrchr(fpath, '.'); // last(?) dot
    //printf("%s\n", buffer);
    // Allocate memory for the file path and store it in the array
    file_paths[file_count] = malloc(path_len - cwd_len + 1);
    if (file_paths[file_count] == NULL) {
      perror("malloc");
      return -1;
    }
    if (ext && (strcmp(ext, ".c") == 0
        || strcmp(ext, ".h") == 0
        || strcmp(ext, ".txt") == 0)) {
      strcpy(file_paths[file_count], filename);
      file_count++;
    }
    /*if (strncmp(".git/", buffer, 5) != 0) {
      strcpy(file_paths[file_count], buffer);
      file_count++;
    }*/
    
  }
  return 0; // Continue walking the file tree
}

void free_file_paths() {
  for (int i = 0; i < file_count; i++) {
    free(file_paths[i]);
  }
}

int compare(const void* a, const void* b) {
  const char *s1 = *(char**) a;
  const char *s2 = *(char**) b;
  return strcmp(s1, s2);
}

int get_file_system(const char *path, char ***files, int *file_len) {

  file_paths = malloc(sizeof(char*) * MAX_FILES);
  //if (getcwd(cwd, PATH_MAX) == NULL) {
  //  perror("getcwd");
  //  return EXIT_FAILURE;
  //}
  cwd_len = strlen(path) + 1;

  //const char *dir = (const char*)cwd;
  
  // Walk the file tree starting from 'path'
  if (ftw(path, list_callback, 16) == -1) {
    perror("ftw");
    free_file_paths();
    return EXIT_FAILURE;
  }
  qsort(file_paths, file_count, sizeof(char*), compare);

  *files = file_paths;
  *file_len = file_count;
  // Print the file paths
  //printf("\nList of files:\n");
  //for (int i = 0; i < file_count; i++) {
    //char *filename = file_paths[i];
    //printf("%s\n", filename);
  //}

  // Free the memory allocated for the file paths
  // free_file_paths();

  return EXIT_SUCCESS;
}
