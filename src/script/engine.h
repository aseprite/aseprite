// Aseprite Scripting Library
// Copyright (c) 2015-2017 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SCRIPT_ENGINE_H_INCLUDED
#define SCRIPT_ENGINE_H_INCLUDED
#pragma once

#include <string>

struct js_State;

namespace script {
  class Context;
  class EngineDelegate;

  typedef int result_t;
  typedef int index_t;
  typedef js_State* ContextHandle;
  typedef void (*Function)(ContextHandle);
  typedef void (*FinalizeFunction)(ContextHandle, void*);

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

  struct ConstantEntry {
    const char* id;
    double value;
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

    void setContextUserData(void* userData);
    void* getContextUserData();

    void error(const char* err);

    void pop();
    void pop(index_t count);
    void remove(index_t idx);
    void duplicateTop();
    index_t top();

    bool isUndefined(index_t i);
    bool isNull(index_t i);
    bool isNullOrUndefined(index_t i);
    bool isBool(index_t i);
    bool isNumber(index_t i);
    bool isString(index_t i);
    bool isObject(index_t i);
    bool isArray(index_t i);
    bool isUserData(index_t i, const char* tag);

    bool toBool(index_t i);
    double toNumber(index_t i);
    int toInt(index_t i);
    unsigned int toUInt(index_t i);
    const char* toString(index_t i);
    void* toUserData(index_t i, const char* tag);

    bool hasProp(index_t i, const char* propName);
    void getProp(index_t i, const char* propName);
    void setProp(index_t i, const char* propName);

    bool requireBool(index_t i);
    double requireNumber(index_t i);
    int requireInt(index_t i);
    unsigned int requireUInt(index_t i);
    const char* requireString(index_t i);
    void* requireUserData(index_t i, const char* tag);

    void pushUndefined();
    void pushNull();
    void pushBool(bool val);
    void pushNumber(double val);
    void pushInt(int val);
    void pushUInt(unsigned int val);
    void pushString(const char* str);
    void pushGlobalObject();
    void newObject();
    void newObject(const char* className,
                   void* userData,
                   FinalizeFunction finalize);
    void newUserData(const char* tag,
                     void* userData,
                     FinalizeFunction finalize);

    void registerConstants(index_t idx,
                           const ConstantEntry* consts);
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
    bool eval(const std::string& jsCode,
              const std::string& filename = std::string());
    bool evalFile(const std::string& filename);

    Context& context() { return m_ctx; }

  protected:
    virtual void onBeforeEval() { }
    virtual void onAfterEval(bool err) { }

  private:
    Context m_ctx;
    EngineDelegate* m_delegate;
    bool m_printLastResult;
  };

}

#endif
