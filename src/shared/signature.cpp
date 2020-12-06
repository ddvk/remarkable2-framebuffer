// ./a.out < xochitl
// ./a.out < remarkable-shutdown

#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace swtfb {
// from https://stackoverflow.com/a/14002993/442652
char *read_file(const char *filename, int *out_size) {
  FILE *f = fopen(filename, "rb");
  fseek(f, 0, SEEK_END);
  long fsize = ftell(f);
  fseek(f, 0, SEEK_SET); /* same as rewind(f); */

  char *string = (char *)malloc(fsize + 1);
  fread(string, 1, fsize, f);
  fclose(f);

  string[fsize] = 0;
  *out_size = fsize;

  return string;
}

char *locate_signature(const char *path, const char *find, int N) {
  int size;
  char *data = read_file(path, &size);

  int offset = 0x10000;
  char *ret = NULL;

  bool found;
  for (int i = 0; i < size - N; i++) {
    found = true;
    for (int j = 0; j < N; j++) {
      if (find[j] != data[i + j]) {
        found = false;
        break;
      }
    }

    if (found) {
      ret = (char *)i + offset - 4;
      break;
    }
  }

  free(data);
  return ret;
}
} // namespace swtfb
