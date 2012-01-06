// ASEPRITE base library
// Copyright (C) 2001-2012  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef BASE_MEMORY_DUMP_H_INCLUDED
#define BASE_MEMORY_DUMP_H_INCLUDED

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
