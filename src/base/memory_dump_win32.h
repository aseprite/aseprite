// Aseprite Base Library
// Copyright (c) 2001-2013, 2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef BASE_MEMORY_DUMP_WIN32_H_INCLUDED
#define BASE_MEMORY_DUMP_WIN32_H_INCLUDED
#pragma once

#ifdef _WIN32
  #include <windows.h>
  #include <dbghelp.h>
#endif

#include "base/string.h"

static std::wstring memoryDumpFile;

class base::MemoryDump::MemoryDumpImpl
{
public:
  MemoryDumpImpl() {
    memoryDumpFile = L"memory.dmp";
    ::SetUnhandledExceptionFilter(MemoryDumpImpl::unhandledException);
  }

  ~MemoryDumpImpl() {
    ::SetUnhandledExceptionFilter(NULL);
  }

  void setFileName(const std::string& fileName) {
    memoryDumpFile = base::from_utf8(fileName);
  }

  static LONG WINAPI unhandledException(_EXCEPTION_POINTERS* exceptionPointers) {
    MemoryDumpImpl::createMemoryDump(exceptionPointers);
    return EXCEPTION_EXECUTE_HANDLER;
  }

private:
  class MemoryDumpFile {
  public:
    MemoryDumpFile() {
      m_handle = ::CreateFile(memoryDumpFile.c_str(),
                              GENERIC_WRITE, 0, NULL,
                              CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    }
    ~MemoryDumpFile() {
      ::CloseHandle(m_handle);
    }
    HANDLE handle() { return m_handle; }
  private:
    HANDLE m_handle;
  };

  static void createMemoryDump(_EXCEPTION_POINTERS* exceptionPointers) {
    MemoryDumpFile file;

    MINIDUMP_EXCEPTION_INFORMATION ei;
    ei.ThreadId           = GetCurrentThreadId();
    ei.ExceptionPointers  = exceptionPointers;
    ei.ClientPointers     = FALSE;

    ::MiniDumpWriteDump(::GetCurrentProcess(),
                        ::GetCurrentProcessId(),
                        file.handle(),
                        MiniDumpNormal,
                        (exceptionPointers ? &ei: NULL),
                        NULL,
                        NULL);

  }
};

#endif // BASE_MEMORY_DUMP_WIN32_H_INCLUDED
