// ASEPRITE base library
// Copyright (C) 2001-2012  David Capello
//
// This source file is distributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef BASE_MEMORY_DUMP_NONE_H_INCLUDED
#define BASE_MEMORY_DUMP_NONE_H_INCLUDED

class base::MemoryDump::MemoryDumpImpl
{
public:
  MemoryDumpImpl() {
    // Do nothing
  }

  ~MemoryDumpImpl() {
    // Do nothing
  }

  void setFileName(const std::string& fileName) {
    // Do nothing
  }
};

#endif // BASE_MEMORY_DUMP_NONE_H_INCLUDED
