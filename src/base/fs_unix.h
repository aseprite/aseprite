// Aseprite Base Library
// Copyright (c) 2001-2013 David Capello
//
// This source file is distributed under MIT license,
// please read LICENSE.txt for more information.

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdexcept>
#include <vector>

#if __APPLE__
#include <mach-o/dyld.h>
#endif

#include "base/path.h"

#define MAXPATHLEN 1024

namespace base {

bool file_exists(const string& path)
{
  struct stat sts;
  return (stat(path.c_str(), &sts) == 0 && S_ISREG(sts.st_mode)) ? true: false;
}

bool directory_exists(const string& path)
{
  struct stat sts;
  return (stat(path.c_str(), &sts) == 0 && S_ISDIR(sts.st_mode)) ? true: false;
}

void make_directory(const string& path)
{
  int result = mkdir(path.c_str(), 0777);
  if (result < 0) {
    // TODO add errno into the exception
    throw std::runtime_error("Error creating directory");
  }
}

void remove_directory(const string& path)
{
  int result = rmdir(path.c_str());
  if (result != 0) {
    // TODO add errno into the exception
    throw std::runtime_error("Error removing directory");
  }
}

string get_app_path()
{
  std::vector<char> path(MAXPATHLEN);

#if __APPLE__

  uint32_t size = path.size();
  while (_NSGetExecutablePath(&path[0], &size) == -1)
    path.resize(size);

#else

  if (readlink("/proc/self/exe", &path[0], path.size()) > 0)
    return std::string(&path[0]);

#endif

  return std::string();
}

string get_temp_path()
{
  char* tmpdir = getenv("TMPDIR");
  if (tmpdir)
    return tmpdir;
  else
    return "/tmp";
}

}
