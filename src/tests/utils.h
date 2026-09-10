// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef ASEPRITE_TESTS_UTILS_H_INCLUDED
#define ASEPRITE_TESTS_UTILS_H_INCLUDED

#include <gtest/gtest.h>

#include "base/fs.h"
#include "base/time.h"

#include "fmt/args.h"

#include <fstream>
#include <string>

class TestTempFile {
public:
  explicit TestTempFile(const std::string& content = "", const std::string& ext = "")
  {
    thread_local int i = 0;
    thread_local const std::string dir = testing::TempDir();
    filename = base::join_path(
      dir,
      fmt::format("tmp_{}_{}{}", base::current_tick(), i, ext.empty() ? "" : "." + ext));
    std::ofstream out(filename);
    out << content;
    i++;
  }

  ~TestTempFile() { base::delete_file(filename); }

  std::string filename;
};

#endif // ASEPRITE_TESTS_UTILS_H_INCLUDED
