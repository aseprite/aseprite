// Aseprite Scripting Library
// Copyright (c) 2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "scripting/engine.h"

#include "base/convert_to.h"
#include "base/exception.h"
#include "base/file_handle.h"
#include "base/memory.h"
#include "scripting/engine_delegate.h"

#define DUK_OPT_PANIC_HANDLER scripting_engine_panic_handler
#include <duktape.h>

class ScriptingEngineException : public base::Exception {
public:
  ScriptingEngineException(duk_errcode_t code, const char* msg)
    : Exception(std::string(msg) + " (code " + base::convert_to<std::string>(code) + ")") {
  }
};

static void scripting_engine_panic_handler(duk_errcode_t code, const char* msg)
{
  throw ScriptingEngineException(code, msg);
}

extern "C" {
  #include <duktape.c>
}

namespace scripting {

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
  duk_get_prop_string(m_handle, i, "\xFF" "\xFF" "ptr");
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

void Context::pushThis(void* ptr)
{
  duk_push_this(m_handle);
  duk_push_pointer(m_handle, ptr);
  duk_put_prop_string(m_handle, -2, "\xFF" "\xFF" "ptr");
}

void Context::pushObject(void* ptr, const char* className)
{
  duk_push_object(m_handle);
  duk_push_pointer(m_handle, ptr);
  duk_put_prop_string(m_handle, -2, "\xFF" "\xFF" "ptr");

  duk_get_global_string(m_handle, className);
  duk_get_prop_string(m_handle, -1, "prototype");
  duk_set_prototype(m_handle, -3);
  duk_pop(m_handle);
}

void* Context::getThis()
{
  duk_push_this(m_handle);
  duk_get_prop_string(m_handle, -1, "\xFF" "\xFF" "ptr");
  void* result = (void*)duk_to_pointer(m_handle, -1);
  duk_pop(m_handle);
  return result;
}

Engine::Engine(EngineDelegate* delegate)
  : m_delegate(delegate)
  , m_ctx(duk_create_heap_default())
{
}

Engine::~Engine()
{
  duk_destroy_heap(m_ctx.handle());
}

void Engine::eval(const std::string& jsCode)
{
  try {
    ContextHandle handle = m_ctx.handle();

    duk_eval_string(handle, jsCode.c_str());

    if (!duk_is_null_or_undefined(handle, -1))
      m_delegate->onConsolePrint(duk_safe_to_string(handle, -1));

    duk_pop(handle);
  }
  catch (const std::exception& ex) {
    std::string err = "Error: ";
    err += ex.what();
    m_delegate->onConsolePrint(err.c_str());
  }
}

void Engine::evalFile(const std::string& file)
{
  try {
    ContextHandle handle = m_ctx.handle();

    FILE* f = base::open_file_raw(file, "rb");
    if (!f)
      goto fail;

    if (fseek(f, 0, SEEK_END) < 0)
      goto fail;
    std::size_t sz = ftell(f);
    if (sz < 0)
      goto fail;
    if (fseek(f, 0, SEEK_SET) < 0)
      goto fail;

    char* buf = (char*)duk_push_fixed_buffer(handle, sz);
    ASSERT(buf != nullptr);
    if (fread(buf, 1, sz, f) != sz)
      goto fail;

    fclose(f);
    f = nullptr;

    duk_push_string(handle, duk_to_string(handle, -1));
    duk_eval_raw(handle, nullptr, 0, DUK_COMPILE_EVAL);

    if (!duk_is_null_or_undefined(handle, -1))
      m_delegate->onConsolePrint(duk_safe_to_string(handle, -1));

    duk_pop(handle);

  fail:
    if (f)
      fclose(f);
  }
  catch (const std::exception& ex) {
    std::string err = "Error: ";
    err += ex.what();
    m_delegate->onConsolePrint(err.c_str());
  }
}

void Engine::registerFunction(const char* id, Function function, int nargs)
{
  ContextHandle handle = m_ctx.handle();

  duk_push_global_object(handle);
  duk_push_c_function(handle, function, nargs);
  duk_put_prop_string(handle, -2, id);
}

void Engine::registerClass(const char* id,
                           Function ctorFunc, int ctorNargs,
                           const FunctionEntry* methods,
                           const PropertyEntry* props)
{
  ContextHandle handle = m_ctx.handle();
  duk_push_c_function(handle, ctorFunc, ctorNargs);

  duk_push_object(handle);

  if (methods)
    duk_put_function_list(handle, -1, (const duk_function_list_entry*)methods);

  if (props) {
    for (int i=0; props[i].key; ++i) {
      duk_push_string(handle, props[i].key);

      duk_idx_t objidx = -2;
      duk_uint_t flags = 0;

      if (props[i].getter) {
        flags |= DUK_DEFPROP_HAVE_GETTER;
        duk_push_c_function(handle, props[i].getter, 0);
        --objidx;
      }

      if (props[i].setter) {
        flags |= DUK_DEFPROP_HAVE_SETTER;
        duk_push_c_function(handle, props[i].setter, 1);
        --objidx;
      }

      duk_def_prop(handle, objidx, flags);
    }
  }

  duk_put_prop_string(handle, -2, "prototype");

  duk_put_global_string(handle, id);
}

void Engine::registerGlobal(const char* id,
                            Function getter, Function setter)
{
  ContextHandle handle = m_ctx.handle();

  duk_push_global_object(handle);
  duk_push_string(handle, id);

  duk_idx_t objidx = -2;
  duk_uint_t flags = 0;

  if (getter) {
    flags |= DUK_DEFPROP_HAVE_GETTER;
    duk_push_c_function(handle, getter, 0);
    --objidx;
  }

  if (setter) {
    flags |= DUK_DEFPROP_HAVE_SETTER;
    duk_push_c_function(handle, setter, 1);
    --objidx;
  }

  duk_def_prop(handle, objidx, flags);
}

} // namespace scripting
