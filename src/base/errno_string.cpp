// Aseprite Base Library
// Copyright (c) 2001-2013, 2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string>

namespace base {

// Like 'strerror' but thread-safe.
std::string get_errno_string(int errnum)
{
  static const char *errors[] = {
    "No error",                                 /* errno = 0 */
    "Operation not permitted",                  /* errno = 1 (EPERM) */
    "No such file or directory",                /* errno = 2 (ENOFILE) */
    "No such process",                          /* errno = 3 (ESRCH) */
    "Interrupted function call",                /* errno = 4 (EINTR) */
    "Input/output error",                       /* errno = 5 (EIO) */
    "No such device or address",                /* errno = 6 (ENXIO) */
    "Arg list too long",                        /* errno = 7 (E2BIG) */
    "Exec format error",                        /* errno = 8 (ENOEXEC) */
    "Bad file descriptor",                      /* errno = 9 (EBADF) */
    "No child processes",                       /* errno = 10 (ECHILD) */
    "Resource temporarily unavailable",         /* errno = 11 (EAGAIN) */
    "Not enough space",                         /* errno = 12 (ENOMEM) */
    "Permission denied",                        /* errno = 13 (EACCES) */
    "Bad address",                              /* errno = 14 (EFAULT) */
    NULL,
    "Resource device",                          /* errno = 16 (EBUSY) */
    "File exists",                              /* errno = 17 (EEXIST) */
    "Improper link",                            /* errno = 18 (EXDEV) */
    "No such device",                           /* errno = 19 (ENODEV) */
    "Not a directory",                          /* errno = 20 (ENOTDIR) */
    "Is a directory",                           /* errno = 21 (EISDIR) */
    "Invalid argument",                         /* errno = 22 (EINVAL) */
    "Too many open files in system",            /* errno = 23 (ENFILE) */
    "Too many open files",                      /* errno = 24 (EMFILE) */
    "Inappropriate I/O control operation",      /* errno = 25 (ENOTTY) */
    NULL,
    "File too large",                           /* errno = 27 (EFBIG) */
    "No space left on device",                  /* errno = 28 (ENOSPC) */
    "Invalid seek",                             /* errno = 29 (ESPIPE) */
    "Read-only file system",                    /* errno = 30 (EROFS) */
    "Too many links",                           /* errno = 31 (EMLINK) */
    "Broken pipe",                              /* errno = 32 (EPIPE) */
    "Domain error",                             /* errno = 33 (EDOM) */
    "Result too large",                         /* errno = 34 (ERANGE) */
    NULL,
    "Resource deadlock avoided",                /* errno = 36 (EDEADLOCK) */
    NULL,
    "Filename too long",                        /* errno = 38 (ENAMETOOLONG) */
    "No locks available",                       /* errno = 39 (ENOLCK) */
    "Function not implemented",                 /* errno = 40 (ENOSYS) */
    "Directory not empty",                      /* errno = 41 (ENOTEMPTY) */
    "Illegal byte sequence",                    /* errno = 42 (EILSEQ) */
  };

  if (errnum >= 0
      && errnum < (int)(sizeof(errors)/sizeof(char *))
      && errors[errnum] != NULL) {
    return errors[errnum];
  }
  else {
    return "Unknown error";
  }
}

} // namespace base
