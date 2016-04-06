// Aseprite Scripting Library
// Copyright (c) 2015-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SCRIPT_ENGINE_H_INCLUDED
#define SCRIPT_ENGINE_H_INCLUDED
#pragma once

#include <string>

struct duk_hthread;

namespace script {
  class Context;
  class EngineDelegate;

  typedef int result_t;
  typedef int index_t;
  typedef struct duk_hthread* ContextHandle;
  typedef result_t (*Function)(ContextHandle ctx);

  struct FunctionEntry {
    const char* id;
    Function value;
    index_t nargs;
  };

  struct PropertyEntry {
    const char* id;
    Function getter;
    Function setter;
  };

  class Module {
  public:
    virtual ~Module() { }
    virtual const char* id() const = 0;
    virtual int registerModule(Context& ctx) = 0;
  };

  class Context {
  public:
    Context(ContextHandle handle) : m_handle(handle) { }

    ContextHandle handle() { return m_handle; }

    void dump();
    void pop();
    void remove(index_t idx);
    void duplicateTop();

    bool isConstructorCall();

    bool isUndefined(index_t i);
    bool isNull(index_t i);
    bool isNullOrUndefined(index_t i);
    bool isBool(index_t i);
    bool isNumber(index_t i);
    bool isNaN(index_t i);
    bool isString(index_t i);

    bool getBool(index_t i);
    bool toBool(index_t i);
    double getNumber(index_t i);
    int getInt(index_t i);
    unsigned int getUInt(index_t i);
    const char* getString(index_t i);
    const char* toString(index_t i);
    void* getThis();

    bool hasProp(index_t i, const char* propName);
    void getProp(index_t i, const char* propName);
    void setProp(index_t i, const char* propName);

    bool requireBool(index_t i);
    double requireNumber(index_t i);
    int requireInt(index_t i);
    unsigned int requireUInt(index_t i);
    const char* requireString(index_t i);
    void* requireObject(index_t i, const char* className);

    void pushUndefined();
    void pushNull();
    void pushBool(bool val);
    void pushNumber(double val);
    void pushNaN();
    void pushInt(int val);
    void pushUInt(unsigned int val);
    void pushString(const char* str);
    void pushThis();
    void pushThis(void* ptr);
    index_t pushObject();
    index_t pushObject(void* ptr, const char* className);
    void pushGlobalObject();

    void registerProp(index_t idx,
                      const char* id,
                      Function getter,
                      Function setter);
    void registerProps(index_t idx,
                       const PropertyEntry* props);
    void registerFunc(index_t idx,
                      const char* id,
                      const Function func,
                      index_t nargs);
    void registerFuncs(index_t idx,
                       const FunctionEntry* methods);
    void registerObject(index_t idx,
                        const char* id,
                        const FunctionEntry* methods,
                        const PropertyEntry* props);
    void registerClass(index_t idx,
                       const char* id,
                       Function ctorFunc, int ctorNargs,
                       const FunctionEntry* methods,
                       const PropertyEntry* props);

  private:
    ContextHandle m_handle;
  };

  class Engine {
  public:
    Engine(EngineDelegate* delegate);
    ~Engine();

    void printLastResult();
    void eval(const std::string& jsCode);
    void evalFile(const std::string& file);

    Context& context() { return m_ctx; }

    void registerModule(Module* module);

  private:
    Context m_ctx;
    EngineDelegate* m_delegate;
    bool m_printLastResult;
  };

}

#endif
