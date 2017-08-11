// Aseprite Scripting Library
// Copyright (c) 2015-2017 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "script/engine.h"

#include "base/convert_to.h"
#include "base/exception.h"
#include "base/file_handle.h"
#include "base/fstream_path.h"
#include "base/memory.h"
#include "script/engine_delegate.h"

#include <fstream>
#include <iostream>
#include <map>
#include <sstream>

extern "C" {
#include <mujs/mujs.h>
}

namespace script {

static const char* stacktrace_js =
  "Error.prototype.toString = function() {\n"
  "  if (this.stackTrace)\n"
  "    return this.name + ': ' + this.message + this.stackTrace;\n"
  "  return this.name + ': ' + this.message;\n"
  "}";

void Context::setContextUserData(void* userData)
{
  js_setcontext(m_handle, userData);
}

void* Context::getContextUserData()
{
  return js_getcontext(m_handle);
}

void Context::error(const char* err)
{
  js_error(m_handle, "%s", err);
}

void Context::pop()
{
  js_pop(m_handle, 1);
}

void Context::pop(index_t count)
{
  js_pop(m_handle, count);
}

void Context::remove(index_t idx)
{
  js_remove(m_handle, idx);
}

void Context::duplicateTop()
{
  js_dup(m_handle);
}

index_t Context::top()
{
  return js_gettop(m_handle);
}

bool Context::isUndefined(index_t i)
{
  return (js_isundefined(m_handle, i) ? true: false);
}

bool Context::isNull(index_t i)
{
  return (js_isnull(m_handle, i) ? true: false);
}

bool Context::isNullOrUndefined(index_t i)
{
  return (js_isnull(m_handle, i) ||
          js_isundefined(m_handle, i)? true: false);
}

bool Context::isBool(index_t i)
{
  return (js_isboolean(m_handle, i) ? true: false);
}

bool Context::isNumber(index_t i)
{
  return (js_isnumber(m_handle, i) ? true: false);
}

bool Context::isString(index_t i)
{
  return (js_isstring(m_handle, i) ? true: false);
}

bool Context::isObject(index_t i)
{
  return (js_isobject(m_handle, i) ? true: false);
}

bool Context::isArray(index_t i)
{
  return (js_isarray(m_handle, i) ? true: false);
}

bool Context::isUserData(index_t i, const char* tag)
{
  return (js_isuserdata(m_handle, i, tag) ? true: false);
}

bool Context::toBool(index_t i)
{
  return (js_toboolean(m_handle, i) ? true: false);
}

double Context::toNumber(index_t i)
{
  return js_tonumber(m_handle, i);
}

int Context::toInt(index_t i)
{
  return js_toint32(m_handle, i);
}

unsigned int Context::toUInt(index_t i)
{
  return js_touint32(m_handle, i);
}

const char* Context::toString(index_t i)
{
  return js_tostring(m_handle, i);
}

void* Context::toUserData(index_t i, const char* tag)
{
  return js_touserdata(m_handle, i, tag);
}

bool Context::requireBool(index_t i)
{
  if (js_isboolean(m_handle, i))
    return true;
  else {
    js_typeerror(m_handle, "not a boolean (index %d)\n", i);
    return false;
  }
}

double Context::requireNumber(index_t i)
{
  if (js_isnumber(m_handle, i))
    return js_tonumber(m_handle, i);
  else {
    js_typeerror(m_handle, "not a number (index %d)\n", i);
    return 0.0;
  }
}

int Context::requireInt(index_t i)
{
  return (int)requireNumber(i);
}

unsigned int Context::requireUInt(index_t i)
{
  return (unsigned int)requireNumber(i);
}

const char* Context::requireString(index_t i)
{
  if (js_isstring(m_handle, i))
    return js_tostring(m_handle, i);
  else {
    js_typeerror(m_handle, "not a string (index %d)\n", i);
    return nullptr;
  }
}

void* Context::requireUserData(index_t i, const char* tag)
{
  if (js_isuserdata(m_handle, i, tag))
    return js_touserdata(m_handle, i, tag);
  else {
    js_typeerror(m_handle, "not a user data (index %d, tag %s)\n", i, tag);
    return nullptr;
  }
}

void Context::pushUndefined()
{
  js_pushundefined(m_handle);
}

void Context::pushNull()
{
  js_pushnull(m_handle);
}

void Context::pushBool(bool val)
{
  js_pushboolean(m_handle, val);
}

void Context::pushNumber(double val)
{
  js_pushnumber(m_handle, val);
}

void Context::pushInt(int val)
{
  js_pushnumber(m_handle, double(val));
}

void Context::pushUInt(unsigned int val)
{
  js_pushnumber(m_handle, double(val));
}

void Context::pushString(const char* str)
{
  js_pushstring(m_handle, str);
}

void Context::pushGlobalObject()
{
  js_pushglobal(m_handle);
}

void Context::newObject()
{
  js_newobject(m_handle);
}

void Context::newObject(const char* className,
                        void* userData,
                        FinalizeFunction finalize)
{
  js_getglobal(m_handle, className);         // class
  js_getproperty(m_handle, -1, "prototype"); // class prototype
  js_newuserdata(m_handle, className, userData, finalize); // class userdata
  js_rot2(m_handle);   // userdata class
  js_pop(m_handle, 1); // userdata
}

void Context::newUserData(const char* tag,
                          void* userData,
                          FinalizeFunction finalize)
{
  js_newuserdata(m_handle, tag, userData, finalize);
}

void Context::registerConstants(index_t idx,
                                const ConstantEntry* consts)
{
  if (idx < 0)
    --idx;
  for (; consts->id; ++consts) {
    js_pushnumber(m_handle, consts->value);
    js_defproperty(m_handle, idx, consts->id, JS_DONTENUM);
  }
}

void Context::registerProp(index_t idx,
                           const char* id,
                           Function getter,
                           Function setter)
{
  if (idx < 0)
    idx -= 2;

  if (getter)
    js_newcfunction(m_handle, getter,
                    (std::string(id) + ".getter").c_str(), 0);
  else
    js_pushnull(m_handle);

  if (setter)
    js_newcfunction(m_handle, setter,
                    (std::string(id) + ".setter").c_str(), 1);
  else
    js_pushnull(m_handle);

  js_defaccessor(m_handle, idx, id, JS_DONTENUM);
}

void Context::registerProps(index_t idx, const PropertyEntry* props)
{
  for (int i=0; props[i].id; ++i) {
    registerProp(idx,
                 props[i].id,
                 props[i].getter,
                 props[i].setter);
  }
}

void Context::registerFunc(index_t idx,
                           const char* id,
                           const Function func,
                           index_t nargs)
{
  js_newcfunction(m_handle, func, id, nargs);
  js_defproperty(m_handle, idx, id, JS_DONTENUM);
}

void Context::registerFuncs(index_t idx, const FunctionEntry* methods)
{
  if (idx < 0)
    --idx;
  for (; methods->id; ++methods) {
    registerFunc(idx,
                 methods->id,
                 methods->value,
                 methods->nargs);
  }
}

void Context::registerObject(index_t idx,
                             const char* id,
                             const FunctionEntry* methods,
                             const PropertyEntry* props)
{
  if (idx < 0)
    --idx;

  newObject();
  if (methods) registerFuncs(-1, methods);
  if (props) registerProps(-1, props);
  js_defproperty(m_handle, idx, id, JS_DONTENUM);
}

void Context::registerClass(index_t idx,
                            const char* id,
                            Function ctorFunc, int ctorNargs,
                            const FunctionEntry* methods,
                            const PropertyEntry* props)
{
  if (idx < 0)
    idx -= 2;

  js_getglobal(m_handle, "Object");
  js_getproperty(m_handle, -1, "prototype");
  js_newuserdata(m_handle, id, nullptr, nullptr);
  if (methods) registerFuncs(-1, methods);
  if (props) registerProps(-1, props);
  js_newcconstructor(m_handle, ctorFunc, ctorFunc, id, ctorNargs);
  js_defproperty(m_handle, idx, id, JS_DONTENUM);
  js_pop(m_handle, 1);          // pop Object
}

bool Context::hasProp(index_t i, const char* propName)
{
  return (js_hasproperty(m_handle, i, propName) ? true: false);
}

void Context::getProp(index_t i, const char* propName)
{
  js_getproperty(m_handle, i, propName);
}

void Context::setProp(index_t i, const char* propName)
{
  js_defproperty(m_handle, i, propName, JS_DONTENUM);
}

Engine::Engine(EngineDelegate* delegate)
  : m_ctx(js_newstate(NULL, NULL, JS_STRICT))
  , m_delegate(delegate)
  , m_printLastResult(false)
{
  // Pre-scripts
  js_dostring(m_ctx.handle(), stacktrace_js);
}

Engine::~Engine()
{
  js_freestate(m_ctx.handle());
}

void Engine::printLastResult()
{
  m_printLastResult = true;
}

void Engine::eval(const std::string& jsCode,
                  const std::string& filename)
{
  bool errFlag = true;
  onBeforeEval();

  ContextHandle handle = m_ctx.handle();

  if (js_ploadstring(handle, filename.c_str(), jsCode.c_str()) == 0) {
    js_pushundefined(handle);
    if (js_pcall(handle, 0) == 0) {
      if (m_printLastResult) {
        if (!js_isundefined(handle, -1)) {
          const char* result = js_tostring(handle, -1);
          if (result)
            m_delegate->onConsolePrint(result);
        }
      }
      errFlag = false;
    }
  }

  // Print error message
  if (errFlag) {
    std::string err;
    const char* s = js_trystring(handle, -1, "Error");
    if (s)
      m_delegate->onConsolePrint(s);
  }

  js_pop(handle, 1);

  onAfterEval(errFlag);
}

void Engine::evalFile(const std::string& filename)
{
  std::stringstream buf;
  {
    std::ifstream s(FSTREAM_PATH(filename));
    buf << s.rdbuf();
  }
  eval(buf.str(), filename);
}

} // namespace script
