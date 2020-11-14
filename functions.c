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


#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdarg.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>
#include <linux/limits.h>
#include <sys/stat.h>
#include <dirent.h>
#include "utilities.h"


typedef int (*orig_open_func_type)(const char* file, int oflag, ...);
typedef char* (*orig_mktemp_func_type)(char* template);
typedef FILE* (*orig_fopen_func_type)(const char* filename, const char* modes);
typedef int (*orig_unlink_func_type)(const char *name);
typedef int (*orig_xstat_func_type)(int ver, const char* filename, struct stat* stat_buf);
typedef int (*orig_creat_func_type)(const char *file, mode_t mode);
typedef int (*orig_link_func_type)(const char *from, const char *to);
typedef int (*orig_rename_func_type)(const char *old, const char *new);
typedef int (*orig_chmod_func_type)(const char *file, __mode_t mode);
typedef int (*orig_scandir_func_type)(const char *__restrict dir, struct dirent ***__restrict __namelist, int (*selector) (const struct dirent *), int (*cmp) (const struct dirent **, const struct dirent **));
typedef FILE* (*orig_fopen64_func_type)(const char* filename, const char* modes);typedef int (*orig_remove_func_type)(const char *filename);


//
// glibc functions overrides start here
//

int open(const char *file, int oflag, ...) {
  orig_open_func_type orig_func;
  orig_func = (orig_open_func_type)dlsym(RTLD_NEXT, "open");
  if (orig_func == NULL) {
    fprintf(stderr, "lockdev-redirect: CRITICAL ERROR: can't call open\n");
    return -1;
  }

  const char* lockpath_prefix = _find_lockpath_prefix(file);
  if (!lockpath_prefix) {
    if (__OPEN_NEEDS_MODE(oflag)) {
      va_list args;
      va_start(args, oflag);
      int mode = va_arg(args, int);
      va_end(args);
      return orig_func(file, oflag, mode);
    } else {
      return orig_func(file, oflag);
    }
  }

  char buffer[PATH_MAX];
  const char* new_path = file;
  if (_rewrite_path(buffer, file, lockpath_prefix))
    new_path = buffer;

  if (__OPEN_NEEDS_MODE(O_CREAT)) {
    va_list args;
    va_start(args, oflag);
    int mode = va_arg(args, int);
    va_end(args);
    return orig_func(new_path, oflag, mode);
  } else {
    return orig_func(new_path, oflag);
  }
}


FILE *fopen (const char *filename, const char *modes) {
  orig_fopen_func_type orig_func;
  orig_func = (orig_fopen_func_type)dlsym(RTLD_NEXT, "fopen");
  if (orig_func == NULL) {
    fprintf(stderr, "lockdev-redirect: CRITICAL ERROR: can't call fopen\n");
    return NULL;
  }

  const char* lockpath_prefix = _find_lockpath_prefix(filename);
  if (!lockpath_prefix)
    return orig_func(filename, modes);

  char new_path[PATH_MAX];
  if (!_rewrite_path(new_path, filename, lockpath_prefix))
    return orig_func(filename, modes);

  return orig_func(new_path, modes);
}


int unlink(const char *name) {
  orig_unlink_func_type orig_func;
  orig_func = (orig_unlink_func_type)dlsym(RTLD_NEXT, "unlink");
  if (orig_func == NULL) {
    fprintf(stderr, "lockdev-redirect: CRITICAL ERROR: can't call unlink\n");
    return -1;
  }

  const char* lockpath_prefix = _find_lockpath_prefix(name);
  if (!lockpath_prefix)
    return orig_func(name);

  char new_path[PATH_MAX];
  if (!_rewrite_path(new_path, name, lockpath_prefix))
    return orig_func(name);

  return orig_func(new_path);
}


char* mktemp(char *template) {
  orig_mktemp_func_type orig_func;
  orig_func = (orig_mktemp_func_type)dlsym(RTLD_NEXT, "mktemp");
  if (orig_func == NULL) {
    fprintf(stderr, "lockdev-redirect: CRITICAL ERROR: can't call mktemp\n");
    template[0] = '\0';
    return template;
  }

  const char* lockpath_prefix = _find_lockpath_prefix(template);
  if (!lockpath_prefix)
    return orig_func(template);

  char new_template[PATH_MAX];
  if (!_rewrite_path(new_template, template, lockpath_prefix))
    return orig_func(template);

  orig_func(new_template);
  if (new_template[0] == '\0') {
    template[0] = '\0';
    return template;
  }

  // If we successfully made a unique temporary filename for your own target
  // path, then integrate its last six characters into the original template
  char* last_six = new_template + strlen(new_template) - 6;
  strcpy(template + strlen(template) - 6, last_six);
  return template;
}


int __xstat (int ver, const char *filename, struct stat *stat_buf) {
  orig_xstat_func_type orig_func;
  orig_func = (orig_xstat_func_type)dlsym(RTLD_NEXT, "__xstat");
  if (orig_func == NULL) {
    fprintf(stderr, "lockdev-redirect: CRITICAL ERROR: can't call __xstat\n");
    return -1;
  }

  const char* lockpath_prefix = _find_lockpath_prefix(filename);
  if (!lockpath_prefix)
    return orig_func(ver, filename, stat_buf);

  char new_path[PATH_MAX];
  if (!_rewrite_path(new_path, filename, lockpath_prefix))
    return orig_func(ver, filename, stat_buf);

  return orig_func(ver, new_path, stat_buf);
}

//
// Implementations up to this line make rxtx work properly
//

int creat(const char *file, mode_t mode) {
  orig_creat_func_type orig_func;
  orig_func = (orig_creat_func_type)dlsym(RTLD_NEXT, "creat");
  if (orig_func == NULL) {
    fprintf(stderr, "lockdev-redirect: CRITICAL ERROR: can't call creat\n");
    return -1;
  }

  const char* lockpath_prefix = _find_lockpath_prefix(file);
  if (!lockpath_prefix)
    return orig_func(file, mode);

  char new_path[PATH_MAX];
  if (!_rewrite_path(new_path, file, lockpath_prefix))
    return orig_func(file, mode);

  return orig_func(new_path, mode);
}


int link(const char *from, const char *to) {
  orig_link_func_type orig_func;
  orig_func = (orig_link_func_type)dlsym(RTLD_NEXT, "link");
  if (orig_func == NULL) {
    fprintf(stderr, "lockdev-redirect: CRITICAL ERROR: can't call link\n");
    return -1;
  }

  const char* lockpath_prefix_from = _find_lockpath_prefix(from);
  const char* lockpath_prefix_to = _find_lockpath_prefix(to);
  if (!lockpath_prefix_from && !lockpath_prefix_to)
    return orig_func(from, to);

  char from_buffer[PATH_MAX];
  const char* new_from = from;
  if (lockpath_prefix_from) {
    if (_rewrite_path(from_buffer, from, lockpath_prefix_from))
      new_from = from_buffer;
  }

  char to_buffer[PATH_MAX];
  const char* new_to = to;
  if (lockpath_prefix_to) {
    if (_rewrite_path(to_buffer, to, lockpath_prefix_to))
      new_to = to_buffer;
  }

  return orig_func(new_from, new_to);
}


int rename (const char* old, const char* new) {
  orig_rename_func_type orig_func;
  orig_func = (orig_rename_func_type)dlsym(RTLD_NEXT, "rename");
  if (orig_func == NULL) {
    fprintf(stderr, "lockdev-redirect: CRITICAL ERROR: can't call rename\n");
    return -1;
  }

  const char* lockpath_prefix_old = _find_lockpath_prefix(old);
  const char* lockpath_prefix_new = _find_lockpath_prefix(new);
  if (!lockpath_prefix_old && !lockpath_prefix_new)
    return orig_func(old, new);

  char old_buffer[PATH_MAX];
  const char* new_old = old;
  if (lockpath_prefix_old) {
    if (_rewrite_path(old_buffer, old, lockpath_prefix_old))
      new_old = old_buffer;
  }

  char new_buffer[PATH_MAX];
  const char* new_new = new;
  if (lockpath_prefix_new) {
    if (_rewrite_path(new_buffer, new, lockpath_prefix_new))
      new_new = new_buffer;
  }

  return orig_func(new_old, new_new);
}

//
// Implementations up to this line make lockdev work properly
//

int chmod(const char *file, __mode_t mode) {
  orig_chmod_func_type orig_func;
  orig_func = (orig_chmod_func_type)dlsym(RTLD_NEXT, "chmod");
  if (orig_func == NULL) {
    fprintf(stderr, "lockdev-redirect: CRITICAL ERROR: can't call chmod\n");
    return -1;
  }

  const char* lockpath_prefix = _find_lockpath_prefix(file);
  if (!lockpath_prefix)
    return orig_func(file, mode);

  char new_path[PATH_MAX];
  if (!_rewrite_path(new_path, file, lockpath_prefix))
    return orig_func(file, mode);

  return orig_func(new_path, mode);
}


extern int scandir (const char *__restrict dir, struct dirent ***__restrict namelist, int (*selector) (const struct dirent *), int (*cmp) (const struct dirent **, const struct dirent **)) {
  orig_scandir_func_type orig_func;
  orig_func = (orig_scandir_func_type)dlsym(RTLD_NEXT, "scandir");
  if (orig_func == NULL) {
    fprintf(stderr, "lockdev-redirect: CRITICAL ERROR: can't call scandir\n");
    return -1;
  }

  const char* lockpath_prefix = _find_lockpath_prefix(dir);
  if (!lockpath_prefix)
    return orig_func(dir, namelist, selector, cmp);

  char new_path[PATH_MAX];
  if (!_rewrite_path(new_path, dir, lockpath_prefix))
    return orig_func(dir, namelist, selector, cmp);

  return orig_func(new_path, namelist, selector, cmp);
}


FILE *fopen64 (const char *filename, const char *modes) {
  orig_fopen64_func_type orig_func;
  orig_func = (orig_fopen64_func_type)dlsym(RTLD_NEXT, "fopen64");
  if (orig_func == NULL) {
    fprintf(stderr, "lockdev-redirect: CRITICAL ERROR: can't call fopen64\n");
    return NULL;
  }

  const char* lockpath_prefix = _find_lockpath_prefix(filename);
  if (!lockpath_prefix)
    return orig_func(filename, modes);

  char new_path[PATH_MAX];
  if (!_rewrite_path(new_path, filename, lockpath_prefix))
    return orig_func(filename, modes);

  return orig_func(new_path, modes);
}


int remove(const char *filename) {
  orig_remove_func_type orig_func;
  orig_func = (orig_remove_func_type)dlsym(RTLD_NEXT, "remove");
  if (orig_func == NULL) {
    fprintf(stderr, "lockdev-redirect: CRITICAL ERROR: can't call remove\n");
    return -1;
  }

  const char* lockpath_prefix = _find_lockpath_prefix(filename);
  if (!lockpath_prefix)
    return orig_func(filename);

  char new_path[PATH_MAX];
  if (!_rewrite_path(new_path, filename, lockpath_prefix))
    return orig_func(filename);

  return orig_func(new_path);
}

//
// Implementations up to this line make MATLAB libmwserialsupport.so work
//
