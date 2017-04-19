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
#include "base/memory.h"
#include "script/engine_delegate.h"

#include <map>
#include <iostream>

#include <duktape.h>

class ScriptEngineException : public base::Exception {
public:
  ScriptEngineException(duk_errcode_t code, const char* msg)
    : Exception(std::string(msg) + " (code " + base::convert_to<std::string>(code) + ")") {
  }
};

namespace script {

const char* kPtrId = "\xFF" "\xFF" "ptr";

namespace {

// TODO classes in modules isn't supported yet
std::map<std::string, Module*> g_modules;

void* on_alloc_function(void* udata, duk_size_t size)
{
  if (size)
    return base_malloc(size);
  else
    return nullptr;
}

void* on_realloc_function(void* udata, void* ptr, duk_size_t size)
{
  if (!ptr) {
    if (size)
      return base_malloc(size);
    else
      return nullptr;
  }
  else if (!size) {
    base_free(ptr);
    return nullptr;
  }
  else
    return base_realloc(ptr, size);
}

void on_free_function(void* udata, void* ptr)
{
  if (ptr)
    base_free(ptr);
}

duk_ret_t on_search_module(duk_context* ctx)
{
  const char* id = duk_get_string(ctx, 0);
  if (!id)
    return 0;

  auto it = g_modules.find(id);
  if (it == g_modules.end()) {
    // TODO error module not found
    return 0;
  }

  Module* module = it->second;
  Context ctxWrapper(ctx);
  if (module->registerModule(ctxWrapper) > 0) {
    // Overwrite the 'exports' property of the module (arg 3)
    // with the object returned by registerModule()
    duk_put_prop_string(ctx, 3, "exports");
  }

  // 0 means no source code
  return 0;
}

void on_fatal_handler(duk_context* ctx, duk_errcode_t code, const char* msg)
{
  throw ScriptEngineException(code, msg);
}

}

void Context::dump()
{
  duk_push_context_dump(m_handle);
  std::cout << duk_to_string(m_handle, -1) << std::endl;
  duk_pop(m_handle);
}

void Context::pop()
{
  duk_pop(m_handle);
}

void Context::pop(index_t count)
{
  duk_pop_n(m_handle, count);
}

void Context::remove(index_t idx)
{
  duk_remove(m_handle, idx);
}

void Context::duplicateTop()
{
  duk_dup_top(m_handle);
}

bool Context::isConstructorCall()
{
  return (duk_is_constructor_call(m_handle) ? true: false);
}

bool Context::isUndefined(index_t i)
{
  return (duk_is_undefined(m_handle, i) ? true: false);
}

bool Context::isNull(index_t i)
{
  return (duk_is_null(m_handle, i) ? true: false);
}

bool Context::isNullOrUndefined(index_t i)
{
  return (duk_is_null_or_undefined(m_handle, i) ? true: false);
}

bool Context::isBool(index_t i)
{
  return (duk_is_boolean(m_handle, i) ? true: false);
}

bool Context::isNumber(index_t i)
{
  return (duk_is_number(m_handle, i) ? true: false);
}

bool Context::isNaN(index_t i)
{
  return (duk_is_nan(m_handle, i) ? true: false);
}

bool Context::isString(index_t i)
{
  return (duk_is_string(m_handle, i) ? true: false);
}

bool Context::getBool(index_t i)
{
  return (duk_get_boolean(m_handle, i) ? true: false);
}

bool Context::toBool(index_t i)
{
  return (duk_to_boolean(m_handle, i) ? true: false);
}

double Context::getNumber(index_t i)
{
  return duk_get_number(m_handle, i);
}

int Context::getInt(index_t i)
{
  return duk_get_int(m_handle, i);
}

unsigned int Context::getUInt(index_t i)
{
  return duk_get_uint(m_handle, i);
}

const char* Context::getString(index_t i)
{
  return duk_get_string(m_handle, i);
}

const char* Context::toString(index_t i)
{
  return duk_to_string(m_handle, i);
}

void* Context::getPointer(index_t i)
{
  return duk_get_pointer(m_handle, i);
}

bool Context::requireBool(index_t i)
{
  return (duk_require_boolean(m_handle, i) ? true: false);
}

double Context::requireNumber(index_t i)
{
  return duk_require_number(m_handle, i);
}

int Context::requireInt(index_t i)
{
  return duk_require_int(m_handle, i);
}

unsigned int Context::requireUInt(index_t i)
{
  return duk_require_uint(m_handle, i);
}

const char* Context::requireString(index_t i)
{
  return duk_require_string(m_handle, i);
}

void* Context::requireObject(index_t i, const char* className)
{
  duk_get_prop_string(m_handle, i, kPtrId);
  void* result = (void*)duk_to_pointer(m_handle, -1);
  // TODO check pointer type
  duk_pop(m_handle);
  return result;
}

void Context::pushUndefined()
{
  duk_push_undefined(m_handle);
}

void Context::pushNull()
{
  duk_push_null(m_handle);
}

void Context::pushBool(bool val)
{
  duk_push_boolean(m_handle, val);
}

void Context::pushNumber(double val)
{
  duk_push_number(m_handle, val);
}

void Context::pushNaN()
{
  duk_push_nan(m_handle);
}

void Context::pushInt(int val)
{
  duk_push_int(m_handle, val);
}

void Context::pushUInt(unsigned int val)
{
  duk_push_uint(m_handle, val);
}

void Context::pushString(const char* str)
{
  duk_push_string(m_handle, str);
}

void Context::pushThis()
{
  duk_push_this(m_handle);
}

void Context::pushThis(void* ptr, const char* className)
{
  duk_push_this(m_handle);
  duk_push_pointer(m_handle, ptr);
  duk_put_prop_string(m_handle, -2, kPtrId);

  // TODO classes in modules isn't supported yet
  duk_get_global_string(m_handle, className);
  duk_get_prototype(m_handle, -1);
  duk_set_prototype(m_handle, -3);
  duk_pop_2(m_handle);
}

void Context::pushPointer(void* ptr)
{
  duk_push_pointer(m_handle, ptr);
}

index_t Context::pushObject()
{
  return duk_push_object(m_handle);
}

index_t Context::pushObject(void* ptr, const char* className)
{
  index_t obj = duk_push_object(m_handle);
  duk_push_pointer(m_handle, ptr);
  duk_put_prop_string(m_handle, obj, kPtrId);

  // TODO classes in modules isn't supported yet
  duk_get_global_string(m_handle, className);
  duk_get_prototype(m_handle, -1);
  duk_set_prototype(m_handle, obj);
  duk_pop(m_handle);

  return obj;
}

void Context::pushGlobalObject()
{
  duk_push_global_object(m_handle);
}

void Context::registerConstants(index_t idx,
                                const ConstantEntry* consts)
{
  duk_put_number_list(m_handle, idx, (const duk_number_list_entry*)consts);
}

void Context::registerProp(index_t idx,
                           const char* id,
                           Function getter,
                           Function setter)
{
  duk_push_string(m_handle, id);
  if (idx < 0)
    --idx;

  duk_uint_t flags = 0;

  if (getter) {
    flags |= DUK_DEFPROP_HAVE_GETTER;
    duk_push_c_function(m_handle, getter, 0);
    if (idx < 0)
      --idx;
  }

  if (setter) {
    flags |= DUK_DEFPROP_HAVE_SETTER;
    duk_push_c_function(m_handle, setter, 1);
    if (idx < 0)
      --idx;
  }

  duk_def_prop(m_handle, idx, flags);
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
  duk_push_c_function(m_handle, func, nargs);
  duk_put_prop_string(m_handle, idx, id);
}

void Context::registerFuncs(index_t idx, const FunctionEntry* methods)
{
  duk_put_function_list(m_handle, idx, (const duk_function_list_entry*)methods);
}

void Context::registerObject(index_t idx,
                             const char* id,
                             const FunctionEntry* methods,
                             const PropertyEntry* props)
{
  duk_push_object(m_handle);
  if (idx < 0)
    --idx;

  if (methods)
    registerFuncs(-1, methods);
  if (props)
    registerProps(-1, props);

  duk_put_prop_string(m_handle, idx, id);
}

void Context::registerClass(index_t idx,
                            const char* id,
                            Function ctorFunc, int ctorNargs,
                            const FunctionEntry* methods,
                            const PropertyEntry* props)
{
  ASSERT(ctorFunc);
  duk_push_c_function(m_handle, ctorFunc, ctorNargs);

  duk_push_object(m_handle); // Prototype object
 if (methods)
    duk_put_function_list(m_handle, -1, (const duk_function_list_entry*)methods);
  if (props)
    registerProps(-1, props);

  duk_set_prototype(m_handle, -2);
  duk_put_prop_string(m_handle, idx-1, id);
}

void* Context::getThis()
{
  duk_push_this(m_handle);
  duk_get_prop_string(m_handle, -1, kPtrId);
  void* result = (void*)duk_to_pointer(m_handle, -1);
  duk_pop_2(m_handle);
  return result;
}

bool Context::hasProp(index_t i, const char* propName)
{
  return (duk_has_prop_string(m_handle, i, propName) ? true: false);
}

void Context::getProp(index_t i, const char* propName)
{
  duk_get_prop_string(m_handle, i, propName);
}

void Context::setProp(index_t i, const char* propName)
{
  duk_put_prop_string(m_handle, i, propName);
}

Engine::Engine(EngineDelegate* delegate)
  : m_ctx(duk_create_heap(&on_alloc_function,
                          &on_realloc_function,
                          &on_free_function,
                          (void*)this,
                          &on_fatal_handler))
  , m_delegate(delegate)
  , m_printLastResult(false)
{
  // Set 'on_search_module' as the function to search modules with
  // require('modulename') on JavaScript.
  duk_get_global_string(m_ctx.handle(), "Duktape");
  duk_push_c_function(m_ctx.handle(), &on_search_module, 4);
  duk_put_prop_string(m_ctx.handle(), -2, "modSearch");
  duk_pop(m_ctx.handle());
}

Engine::~Engine()
{
  duk_destroy_heap(m_ctx.handle());
}

void Engine::printLastResult()
{
  m_printLastResult = true;
}

void Engine::eval(const std::string& jsCode)
{
  bool errFlag = true;
  onBeforeEval();
  try {
    ContextHandle handle = m_ctx.handle();

    duk_eval_string(handle, jsCode.c_str());

    if (m_printLastResult &&
        !duk_is_null_or_undefined(handle, -1)) {
      m_delegate->onConsolePrint(duk_safe_to_string(handle, -1));
    }

    duk_pop(handle);
    errFlag = false;
  }
  catch (const std::exception& ex) {
    std::string err = "Error: ";
    err += ex.what();
    m_delegate->onConsolePrint(err.c_str());
  }
  onAfterEval(errFlag);
}

void Engine::evalFile(const std::string& file)
{
  bool errFlag = true;
  onBeforeEval();
  try {
    ContextHandle handle = m_ctx.handle();

    base::FileHandle fhandle(base::open_file(file, "rb"));
    FILE* f = fhandle.get();
    if (!f)
      return;

    if (fseek(f, 0, SEEK_END) < 0)
      return;

    long sz = ftell(f);
    if (sz < 0)
      return;

    if (fseek(f, 0, SEEK_SET) < 0)
      return;

    char* buf = (char*)duk_push_fixed_buffer(handle, sz);
    ASSERT(buf != nullptr);
    if (long(fread(buf, 1, sz, f)) != sz)
      return;

    fclose(f);
    f = nullptr;

    duk_push_string(handle, duk_to_string(handle, -1));
    duk_eval_raw(handle, nullptr, 0, DUK_COMPILE_EVAL);

    if (m_printLastResult &&
        !duk_is_null_or_undefined(handle, -1)) {
      m_delegate->onConsolePrint(duk_safe_to_string(handle, -1));
    }

    duk_pop(handle);
    errFlag = false;
  }
  catch (const std::exception& ex) {
    std::string err = "Error: ";
    err += ex.what();
    m_delegate->onConsolePrint(err.c_str());
  }
  onAfterEval(errFlag);
}

void Engine::registerModule(Module* module)
{
  g_modules[module->id()] = module;
}

} // namespace script
