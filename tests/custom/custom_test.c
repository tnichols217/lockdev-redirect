// The code here is meant to test functions that are not used by any
// open source libraries and so need selfmade test code

#include <linux/limits.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>

#define LOCKDIR "/var/lock"


int main (int argc, char *argv[]) {
  char lockfilename[PATH_MAX];
  int n = snprintf(lockfilename, PATH_MAX, "lockdev-redirect-custom-%d.tmp", getpid());
  if (n < 0 || n >= PATH_MAX)
    return 1;
  char lockfilepath[PATH_MAX];
  n = snprintf(lockfilepath, PATH_MAX, "%s/%s", LOCKDIR, lockfilename);
  if (n < 0 || n >= PATH_MAX)
    return 1;

  // Test fopen64 first. This also creates a lock file to work with
  printf("Testing fopen64: ");
  FILE* fp = fopen64(lockfilepath, "w");
  if (!fp) {
    printf("FAIL\n");
    return 1;
  }
  else
    printf("PASS\n");
  fclose(fp);

  // Test scandir
  printf("Testing scandir: ");
  struct dirent **namelist;
  n = scandir(LOCKDIR, &namelist, NULL, alphasort);
  if (n == -1) {
    printf("FAIL\n");
    return 1;
  }

  bool found = false;
  while (n--) {
    if (!strcmp(namelist[n]->d_name, lockfilename))
      found = true;
    free(namelist[n]);
  }
  free(namelist);

  if (!found) {
    printf("FAIL\n");
    return 1;
  }
  else
    printf("PASS\n");

  // Test chmod
  printf("Testing chmod: ");
  n = chmod(lockfilepath, S_IRWXU);
  if (n) {
    printf("FAIL\n");
    return 1;
  }
  else
    printf("PASS\n");

  // Finally test remove
  printf("Testing remove: ");
  n = remove(lockfilepath);
  if (n) {
    printf("FAIL\n");
    return 1;
  }
  else
    printf("PASS\n");

  return 0;
}
