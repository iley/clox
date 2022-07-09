#include <stdio.h>
#include <stdlib.h>

#include "chunk.h"
#include "debug.h"
#include "vm.h"

static void run_repl();
static void run_script(const char* path);
static char* read_file(const char* path);

int main(int argc, char* argv[]) {
  vm_init();

  if (argc == 1) {
    run_repl();
  } else if (argc == 2) {
    const char* path = argv[1];
    run_script(path);
  } else {
    fprintf(stderr, "Usage: %s [path]\n", argv[0]);
    return 1;
  }

  vm_free();
  return 0;
}

static void run_repl() {
  char line[1024];
  for (;;) {
    printf("> ");
    if (!fgets(line, sizeof(line), stdin)) {
      printf("\n");
      break;
    }
    execute(line);
  }
}

static void run_script(const char* path) {
  const char* source = read_file(path);
  execute_result_t result = execute(source);
  if (result != EXECUTE_OK) {
    exit(1);
  }
}

static char* read_file(const char* path) {
  FILE* file = fopen(path, "rb");
  if (file == NULL) {
    fprintf(stderr, "could not open file '%s'\n", path);
    exit(1);
  }

  fseek(file, 0L, SEEK_END);
  size_t file_size = ftell(file);
  rewind(file);

  char* buffer = (char*)malloc(file_size + 1);
  if (buffer == NULL) {
    fprintf(stderr, "not enough memory to read flie '%s' (size %lu)\n", path, file_size);
    exit(1);
  }
  size_t bytes_read = fread(buffer, sizeof(char), file_size, file);
  if (bytes_read < file_size) {
    fprintf(stderr, "could not read file '%s'\n", path);
    exit(1);
  }
  buffer[bytes_read] = '\0';

  fclose(file);
  return buffer;
}
