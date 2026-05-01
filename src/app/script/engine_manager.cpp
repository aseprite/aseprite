// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#include "app/script/engine_manager.h"

#include "app/console.h"

namespace app::script {

std::unique_ptr<Engine> EngineManager::create()
{
  auto engine = std::make_unique<Engine>();
  engine->ConsolePrint.connect(&Console::println);
  engine->ConsoleError.connect(&Console::println);
  return engine;
}

void EngineManager::runUserScript(const std::string& filename, const Params& params)
{
  auto* manager = instance();
  std::shared_ptr<Engine> engine;

  auto it = manager->m_engines.find(filename);
  if (it == manager->m_engines.end()) {
    engine = create();
    it = manager->m_engines.emplace(filename, engine).first;
  }
  else {
    engine = it->second;
  }

  engine->evalUserFile(filename, params);
  if (!engine->hasLingeringObjects())
    manager->m_engines.erase(it);
}

void EngineManager::clear()
{
  instance()->m_engines.clear();
}

EngineManager* EngineManager::instance()
{
  static EngineManager instance;
  return &instance;
}
} // namespace app::script
