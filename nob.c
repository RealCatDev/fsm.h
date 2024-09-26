#define NOB_IMPLEMENTATION
#include "./include/nob.h"

#include <stdbool.h>

#define CC "gcc"
#define CFLAGS "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-ggdb", "-std=c99", "-I./include"
#define LDFLAGS ""

typedef struct {
  const char *source_path;
  const char *exe_path;
} example_t;

example_t examples[] = {
  (example_t){
    .source_path = "./examples/turnstile.c",
    .exe_path = "./build/turnstile",
  },
  (example_t){
    .source_path = "./examples/regex.c",
    .exe_path = "./build/regex",
  },
  // (example_t){
  //   .source_path = ,
  //   .exe_path = ,
  // },
};

bool build(const char *src_path, const char *exe_path) {
  Nob_Cmd cmd = {0};
  nob_cmd_append(&cmd, CC, CFLAGS, "-o", exe_path, src_path, LDFLAGS);
  return nob_cmd_run_sync(cmd);
}

bool build_examples() {
  size_t size = NOB_ARRAY_LEN(examples);
  for (size_t i = 0; i < size; ++i) {
    example_t example = examples[i];
    if (!build(example.source_path, example.exe_path)) return false;
  }
  return true;
}

int main(int argc, char **argv) {
  NOB_GO_REBUILD_URSELF(argc, argv);

  if (!nob_mkdir_if_not_exists("build")) return 1;
  if (!build_examples()) return 1;

  return 0;
}