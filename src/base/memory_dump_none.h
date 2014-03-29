// Aseprite Base Library
// Copyright (c) 2001-2013 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef BASE_MEMORY_DUMP_NONE_H_INCLUDED
#define BASE_MEMORY_DUMP_NONE_H_INCLUDED
#pragma once

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
