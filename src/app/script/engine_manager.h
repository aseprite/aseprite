#ifndef ASEPRITE_ENGINEMANAGER_H
#define ASEPRITE_ENGINEMANAGER_H

#include "app/console.h"
#include "engine.h"

namespace app { namespace script {

using Engines = std::map<std::string, std::shared_ptr<Engine>>;
class EngineManager {
public:
  // Creates a new Engine with Console printing.
  static std::unique_ptr<Engine> create()
  {
    auto engine = std::make_unique<Engine>();
    engine->ConsolePrint.connect(&Console::println);
    engine->ConsoleError.connect(&Console::println);
    return engine;
  }

  // Creates a new Engine and evals the given filename, if the filename entry point was already
  // used, the engine is reused.
  // This engine's memory is handled by the manager and is released on exit.
  static void createForScript(const std::string& filename, const Params& params)
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

  static void clear() { instance()->m_engines.clear(); }

  static EngineManager* instance()
  {
    static EngineManager instance;
    return &instance;
  }

private:
  EngineManager() = default;
  Engines m_engines;
};

}} // namespace app::script

#endif // ASEPRITE_ENGINEMANAGER_H
