// Aseprite Base Library
// Copyright (c) 2001-2013 David Capello
//
// This source file is distributed under MIT license,
// please read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

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
