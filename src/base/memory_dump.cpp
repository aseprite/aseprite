// ASEPRITE base library
// Copyright (C) 2001-2013  David Capello
//
// This source file is distributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include "base/memory_dump.h"

#ifdef WIN32
  #include "base/memory_dump_win32.h"
#else
  #include "base/memory_dump_none.h"
#endif

namespace base {

MemoryDump::MemoryDump()
  : m_impl(new MemoryDumpImpl)
{
}

MemoryDump::~MemoryDump()
{
  delete m_impl;
}

void MemoryDump::setFileName(const std::string& fileName)
{
  m_impl->setFileName(fileName);
}

} // namespace base
