// Aseprite Scripting Library
// Copyright (c) 2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SCRIPTING_ENGINE_H_INCLUDED
#define SCRIPTING_ENGINE_H_INCLUDED
#pragma once

#include <string>

namespace scripting {
  class EngineDelegate;

  typedef int result_t;
  typedef int index_t;

  typedef void* ContextHandle;

  class Context {
  public:
    Context(ContextHandle handle) : m_handle(handle) { }

    ContextHandle handle() { return m_handle; }

    bool isConstructorCall();

    bool isUndefined(index_t i);
    bool isNull(index_t i);
    bool isNullOrUndefined(index_t i);
    bool isBool(index_t i);
    bool isNumber(index_t i);
    bool isNaN(index_t i);
    bool isString(index_t i);

    bool getBool(index_t i);
    double getNumber(index_t i);
    int getInt(index_t i);
    unsigned int getUInt(index_t i);
    const char* getString(index_t i);
    void* getThis();

    bool requireBool(index_t i);
    double requireNumber(index_t i);
    int requireInt(index_t i);
    unsigned int requireUInt(index_t i);
    const char* requireString(index_t i);

    void pushUndefined();
    void pushNull();
    void pushBool(bool val);
    void pushNumber(double val);
    void pushNaN();
    void pushInt(int val);
    void pushUInt(unsigned int val);
    void pushString(const char* str);
    void pushThis(void* ptr);

  private:
    ContextHandle m_handle;
  };

  typedef result_t (*Function)(ContextHandle ctx);

  struct FunctionEntry {
    const char* key;
    Function value;
    index_t nargs;
  };

  struct PropertyEntry {
    const char* key;
    Function getter;
    Function setter;
  };

  class Engine {
  public:
    Engine(EngineDelegate* delegate);
    ~Engine();

    void eval(const std::string& jsCode);
    void evalFile(const std::string& file);

    void registerFunction(const char* id, Function function, int nargs);
    void registerClass(const char* id,
                       Function ctorFunc, int ctorNargs,
                       const FunctionEntry* methods,
                       const PropertyEntry* props);

  private:
    Context m_ctx;
    EngineDelegate* m_delegate;
  };

}

#endif
