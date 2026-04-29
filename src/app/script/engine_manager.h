// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_SCRIPT_ENGINE_MANAGER_H_INCLUDED
#define APP_SCRIPT_ENGINE_MANAGER_H_INCLUDED

#include "app/script/engine.h"

namespace app::script {

using Engines = std::map<std::string, std::shared_ptr<Engine>>;
class EngineManager {
public:
  // Creates a new Engine with Console printing.
  static std::unique_ptr<Engine> create();

  // Creates a new Engine and evals the given filename, if the filename entry point was already
  // used, the engine is reused.
  // This engine's memory is handled by the manager and is released on exit.
  static void runUserScript(const std::string& filename, const Params& params);

  // Deletes all managed engine instances
  static void clear();

  static EngineManager* instance();

private:
  EngineManager() = default;
  Engines m_engines;
};

} // namespace app::script

#endif // APP_SCRIPT_ENGINE_MANAGER_H_INCLUDED
