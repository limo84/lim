#include <stdio.h>
#include <stdlib.h>
#include <ftw.h>
#include <string.h>
#include <libgen.h>
#include <unistd.h>

#define MAX_FILES 1000 // Maximum number of files to store
#define PATH_MAX 4096

// Global array to store file paths
char *file_paths[MAX_FILES];
int file_count = 0;

// Callback function to add file paths to the array
int list_callback(const char *fpath, const struct stat *sb, int typeflag) {
  if (typeflag == FTW_F && file_count < MAX_FILES) {
        // Allocate memory for the file path and store it in the array
        file_paths[file_count] = malloc(strlen(fpath) + 1);
        if (file_paths[file_count] == NULL) {
            perror("malloc");
            return -1;
        }
        strcpy(file_paths[file_count], fpath);
        file_count++;
    }
    return 0; // Continue walking the file tree
}

void free_file_paths() {
    for (int i = 0; i < file_count; i++) {
        free(file_paths[i]);
    }
}

int main(int argc, char *argv[]) {

  char cwd[PATH_MAX];

  if (getcwd(cwd, PATH_MAX) == NULL) {
    perror("getcwd");
    return EXIT_FAILURE;
  }

  printf("%s\n", cwd);

  const char *dir = (const char*)cwd;
    // Walk the file tree starting from 'dir'
    if (ftw(dir, list_callback, 16) == -1) {
        perror("ftw");
        free_file_paths();
        return EXIT_FAILURE;
    }

    // Print the file paths
    printf("List of files:\n");
    for (int i = 0; i < file_count; i++) {
      char *filename = file_paths[i];
      printf("%s\n", filename);
    }

    // Free the memory allocated for the file paths
    free_file_paths();

    return EXIT_SUCCESS;
}
