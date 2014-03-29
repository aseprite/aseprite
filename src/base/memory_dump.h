// Aseprite Base Library
// Copyright (c) 2001-2013 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef BASE_MEMORY_DUMP_H_INCLUDED
#define BASE_MEMORY_DUMP_H_INCLUDED
#pragma once

#include <string>

namespace base {

class MemoryDump {
public:
  MemoryDump();
  ~MemoryDump();

  void setFileName(const std::string& fileName);

private:
  class MemoryDumpImpl;
  MemoryDumpImpl* m_impl;
};

} // namespace base

#endif // BASE_MEMORY_DUMP_H_INCLUDED
