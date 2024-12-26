// Aseprite
// Copyright (C) 2019-2024  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_COMMANDS_NEW_PARAMS_H
#define APP_COMMANDS_NEW_PARAMS_H

#include "app/commands/command.h"
#include "app/commands/params.h"
#include "app/context.h"

#include <map>
#include <string>
#include <utility>

#ifdef ENABLE_SCRIPTING
extern "C" struct lua_State;
#endif

namespace app {

class ParamBase {
public:
  virtual ~ParamBase() {}
  virtual void resetValue() = 0;
  virtual void fromString(const std::string& value) = 0;
#ifdef ENABLE_SCRIPTING
  virtual void fromLua(lua_State* L, int index) = 0;
#endif
};

class NewParams {
public:
  void addParam(const char* id, ParamBase* param) { m_params[id] = param; }

  void resetValues()
  {
    for (auto& pair : m_params)
      pair.second->resetValue();
  }

  ParamBase* getParam(const std::string& id)
  {
    auto it = m_params.find(id);
    if (it != m_params.end())
      return it->second;
    else
      return nullptr;
  }

private:
  std::map<std::string, ParamBase*> m_params;
};

template<typename T>
class Param : public ParamBase {
public:
  Param(NewParams* params, const T& defaultValue, const char* id)
    : m_defaultValue(defaultValue)
    , m_value(defaultValue)
  {
    if (id)
      params->addParam(id, this);
  }

  Param(NewParams* params, const T& defaultValue, const std::initializer_list<const char*> ids)
    : Param(params, defaultValue, nullptr)
  {
    for (const auto& id : ids)
      params->addParam(id, this);
  }

  bool isSet() const { return m_isSet; }

  void resetValue() override
  {
    m_value = m_defaultValue;
    m_isSet = false;
  }

  void fromString(const std::string& value) override;
#ifdef ENABLE_SCRIPTING
  void fromLua(lua_State* L, int index) override;
#endif

  const T& operator()() const { return m_value; }

  T& operator()(const T& value)
  {
    m_value = value;
    m_isSet = true;
    return m_value;
  }

private:
  void setValue(const T& value) { operator()(value); }

  T m_defaultValue;
  T m_value;
  bool m_isSet = false;
};

class CommandWithNewParamsBase : public Command {
public:
  template<typename... Args>
  CommandWithNewParamsBase(Args&&... args) : Command(std::forward<Args>(args)...)
  {
  }

#ifdef ENABLE_SCRIPTING
  void loadParamsFromLuaTable(lua_State* L, int index);
#endif

protected:
  void onLoadParams(const Params& params) override;

  virtual void onResetValues() = 0;
  virtual ParamBase* onGetParam(const std::string& k) = 0;

private:
#ifdef ENABLE_SCRIPTING
  bool m_skipLoadParams = false;
#endif
};

template<typename T>
class CommandWithNewParams : public CommandWithNewParamsBase {
public:
  template<typename... Args>
  CommandWithNewParams(Args&&... args) : CommandWithNewParamsBase(std::forward<Args>(args)...)
  {
  }

  T& params() { return m_params; }
  const T& params() const { return m_params; }

protected:
  void onResetValues() override { m_params.resetValues(); }

  ParamBase* onGetParam(const std::string& k) override { return m_params.getParam(k); }

private:
  T m_params;
};

// Common logic to know if we should add a file to recent files. We
// offer two params: "ui" and "recent", if "recent" is specified, we
// do what it says. In other case "ui" is like the default value of
// "recent", i.e. if there is ui=true, we add to recent, if there is
// ui=false, we don't add it.
template<typename T>
inline bool should_add_file_to_recents(const Context* ctx, const T& params)
{
  ASSERT(ctx);
  return (ctx->isUIAvailable() &&
          ((params.recent.isSet() && params.recent()) || (!params.recent.isSet() && params.ui())));
}

} // namespace app

#endif
