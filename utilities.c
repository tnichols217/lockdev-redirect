/*
    lockdev-redirect  Helper to redirect /var/lock to a user writable path
    Copyright (C) 2020  Manuel Reimer <manuel.reimer@gmx.de>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <linux/limits.h>
#include <sys/stat.h>
#include "utilities.h"

/*
 Known device lock file paths:
 /var/lock          is used by both, java rxtx and lockdev by default
 /var/lock/lockdev  is used by java rxtx on Arch and Fedora (patched)
 /run/lock/lockdev  is used by lockdev on Arch and Fedora (patched)
*/

// List of known device lock file paths (prefix to match for)
char* LOCK_PATHS[] = {
  "/var/lock",
  "/run/lock",
  NULL
};

// First stage of path rewrite
// Detects if the given path is below our known lock paths.
// Prameters:
//   path: The path to check
// Return value: Lock path prefix on match. NULL otherwise.
char* _find_lockpath_prefix(const char* path) {
  size_t pathlen = strlen(path);
  unsigned char index = 0;

  while(true) {
    char* prefix = LOCK_PATHS[index];
    if (!prefix)
      return NULL;
    index++;

    size_t prefixlen = strlen(prefix);
    if (prefixlen > pathlen)
      continue;

    if (memcmp(path, prefix, prefixlen) != 0)
      continue;

    if (path[prefixlen] != '\0' && path[prefixlen] != '/')
      continue;

    return prefix;
  }

  return NULL;
}


// Second stage of path rewrite
// This function attempts to actually rewrite the given path. It also creates
// missing path levels.
// Parameters:
//   destination: Destination string buffer. Expected to have size of PATH_MAX
//   path: The path to rewrite
//   prefix: The already determined lock path prefix in the given path
// Return value: true if rewrite succeeded. false otherwise.
bool _rewrite_path(char* destination, const char* path, const char* prefix) {
  // We need XDG_RUNTIME_DIR to be set as this is where we move lock files to
  char* runtime_dir = getenv("XDG_RUNTIME_DIR");
  if (!runtime_dir) {
    fprintf(stderr, "lockdev-redirect: XDG_RUNTIME_DIR not set!\n");
    return false;
  }

  // Append a "/lock" directory to XDG_RUNTIME_DIR for our files to go
  char lock_dir[PATH_MAX];
  int n = snprintf(lock_dir, PATH_MAX, "%s/lock", runtime_dir);
  if (n < 0 || n >= PATH_MAX)
    return false;

  // Attempt to create this directory. Ignore the EEXIST error.
  if (mkdir(lock_dir, 0700) && errno != EEXIST) {
    fprintf(stderr, "lockdev-redirect: Failed to create directory %s, %s\n", lock_dir, strerror(errno));
    return false;
  }

  // Append a "/lockdev" directory
  char lockdev_dir[PATH_MAX];
  n = snprintf(lockdev_dir, PATH_MAX, "%s/lockdev", lock_dir);
  if (n < 0 || n >= PATH_MAX)
    return false;

  // Attempt to create this directory. Ignore the EEXIST error.
  if (mkdir(lockdev_dir, 0700) && errno != EEXIST) {
    fprintf(stderr, "lockdev-redirect: Failed to create directory %s, %s\n", lockdev_dir, strerror(errno));
    return false;
  }

  // Finally replace the found prefix in path with our created lock directory
  const char* suffix = path + strlen(prefix);
  n = snprintf(destination, PATH_MAX, "%s%s", lock_dir, suffix);
  if (n < 0 || n >= PATH_MAX)
    return false;

  // Report success
  return true;
}
