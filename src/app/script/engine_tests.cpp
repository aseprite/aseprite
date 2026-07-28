// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#include "tests/utils.h"
#include "ver/info.h"

#include <gtest/gtest.h>

const char* g_exeName = nullptr;

#ifdef ENABLE_SCRIPTING
  #include "app/app.h"
  #include "app/cli/app_options.h"
  #include "app/commands/commands.h"
  #include "app/context.h"
  #include "app/doc.h"
  #include "app/ini_file.h"
  #include "app/pref/preferences.h"
  #include "app/script/engine.h"
  #include "app/script/luacpp.h"
  #include "app/script/permission_storage.h"
  #include "base/fs.h"
  #include "base/replace_string.h"
  #include "fmt/args.h"
  #include "os/os.h"

  #include <fstream>

using namespace app;
using namespace script;

  #define INIT_ENGINE_TEST(...)                                                                    \
    const os::SystemRef system = os::System::make();                                               \
    const char* argv[] = { g_exeName, "--batch", __VA_ARGS__ };                                    \
    const AppOptions options(std::size(argv), argv);                                               \
    TestTempFile tempIni("", "ini");                                                               \
    ConfigModule::setConfigFilename(tempIni.filename);                                             \
    App app;                                                                                       \
    app.initialize(options);

TEST(Engine, Init)
{
  INIT_ENGINE_TEST()
  auto engine = std::make_shared<Engine>();
  EXPECT_TRUE(engine->luaState() != nullptr);
  EXPECT_TRUE(engine->evalCode("return 0 == ColorMode.RGB"));
}

TEST(Engine, Print)
{
  INIT_ENGINE_TEST()
  auto engine = std::make_shared<Engine>();
  std::vector<std::string> print;
  std::vector<std::string> error;

  engine->ConsolePrint.connect([&print](const std::string& string) { print.push_back(string); });
  engine->ConsoleError.connect([&error](const std::string& string) { error.push_back(string); });

  EXPECT_TRUE(engine->evalCode(R"(print("hello"))"));
  EXPECT_EQ(print.size(), 1);
  EXPECT_EQ(print[0], "hello");
  EXPECT_FALSE(engine->evalCode(R"(!invalid)"));
  EXPECT_EQ(error.size(), 1);
}

TEST(Engine, AppParams)
{
  INIT_ENGINE_TEST()
  auto engine = std::make_shared<Engine>();
  std::vector<std::string> print;
  std::vector<std::string> error;

  engine->ConsolePrint.connect([&print](const std::string& string) { print.push_back(string); });
  engine->ConsoleError.connect([&error](const std::string& string) { error.push_back(string); });

  const TestTempFile tmp(R"(
if app.params.hello ~= "world" then
  error("failed")
end
if app.params.world ~= "hello" then
  error("failed")
end

print("success")
)",
                         "lua");

  EXPECT_TRUE(engine->evalFile(tmp.filename,
                               {
                                 { "hello", "world" },
                                 { "world", "hello" }
  }));
  EXPECT_EQ(error.size(), 0);
  EXPECT_EQ(print.size(), 1);
  EXPECT_EQ(print[0], "success");

  const TestTempFile tmp2(R"(
if app.params.hello ~= nil or app.params.world ~= nil or #app.params ~= 0 then
  error("failed")
end

print("success2")
)",
                          "lua");

  EXPECT_TRUE(engine->evalFile(tmp2.filename));
  EXPECT_EQ(error.size(), 0);
  EXPECT_EQ(print.size(), 2);
  EXPECT_EQ(print[1], "success2");
}

TEST(Engine, EvalFile)
{
  INIT_ENGINE_TEST()
  auto engine = std::make_shared<Engine>();

  std::vector<std::string> print;
  engine->ConsolePrint.connect([&print](const std::string& string) { print.push_back(string); });
  engine->ConsoleError.connect([](const std::string&) { FAIL(); });

  const TestTempFile tmp(R"(
local p = "test"
print(p)
)",
                         "lua");
  engine->evalFile(tmp.filename);

  EXPECT_EQ(print.size(), 1);
  EXPECT_EQ(print[0], "test");
}

TEST(Engine, EvalUserFileWithDeps)
{
  INIT_ENGINE_TEST()

  const TestTempFile tmpDep(R"(
print("dependency")
GlobalThing = "thing"
return "thing2"
)",
                            "lua");

  {
    auto engine = std::make_shared<Engine>();
    std::vector<std::string> print;
    engine->ConsolePrint.connect([&print](const std::string& msg) { print.push_back(msg); });
    engine->ConsoleError.connect([](const std::string& msg) { GTEST_FATAL_FAILURE_(msg.c_str()); });

    const TestTempFile tmp(fmt::format(R"(
local thing = require("./{}")
print(GlobalThing)
print(thing)
)",
                                       base::get_file_title(tmpDep.filename)),
                           "lua");

    engine->evalUserFile(tmp.filename);

    EXPECT_EQ(print.size(), 3);
    EXPECT_EQ(print[0], "dependency");
    EXPECT_EQ(print[1], "thing");
    EXPECT_EQ(print[2], "thing2");
  }

  {
    auto engine = std::make_shared<Engine>();
    std::vector<std::string> print;
    engine->ConsolePrint.connect([&print](const std::string& msg) { print.push_back(msg); });
    engine->ConsoleError.connect([](const std::string& msg) { GTEST_FATAL_FAILURE_(msg.c_str()); });

    const TestTempFile tmp(fmt::format(R"(
local thing = dofile("./{}")
print(GlobalThing)
print(thing)
)",
                                       base::get_file_name(tmpDep.filename)),
                           "lua");

    engine->evalUserFile(tmp.filename);

    EXPECT_EQ(print.size(), 3);
    EXPECT_EQ(print[0], "dependency");
    EXPECT_EQ(print[1], "thing");
    EXPECT_EQ(print[2], "thing2");
  }
}

TEST(Engine, AppScript)
{
  const TestTempFile tmp(R"(
Sprite(128, 128)
)",
                         "lua");

  INIT_ENGINE_TEST("--script", tmp.filename.c_str())

  EXPECT_EQ(App::instance()->context()->activeDocument()->sprite()->bounds(),
            gfx::Rect(0, 0, 128, 128));
}

TEST(Engine, Security)
{
  INIT_ENGINE_TEST()

  auto engine = std::make_shared<Engine>();
  const TestTempFile lua{ "os.execute(\"cd\")", "lua" };
  std::vector<std::string> err;
  engine->ConsoleError.connect([&err](const std::string& string) { err.push_back(string); });

  app.preferences().general.allowCliScriptsFullAccess(false);

  EXPECT_EQ(app.preferences().developer.disableIntegrityCheck(), false);

  const TestTempFile extensionsJSON("{}", "json");
  PermissionStorage storage(extensionsJSON.filename);
  set_permission_storage(&storage);

  storage.writeForMatch(lua.filename, Permission::Execute, "cd", true);

  engine->setPrintEvalResult(true);
  engine->evalUserFile(lua.filename);
  EXPECT_TRUE(err.empty());

  storage.writeForMatch(lua.filename, Permission::Execute, "cd", false);
  engine->evalUserFile(lua.filename);

  EXPECT_EQ(err.size(), 1);
  EXPECT_TRUE(err[0].find("'cd'") != std::string::npos);

  storage.writeForMatch(lua.filename, Permission::Execute, "cd", true);
  {
    std::ofstream(lua.filename, std::ios_base::app) << "\n-- Modified";
  }
  engine->evalUserFile(lua.filename);
  EXPECT_EQ(err.size(), 2);

  storage.writeForMatch(lua.filename, Permission::Execute, "cd", true);
  engine->evalUserFile(lua.filename);
  EXPECT_EQ(err.size(), 2);

  // Same file included through dofile
  auto dofilename = lua.filename;
  #ifdef LAF_WINDOWS
  base::replace_string(dofilename, "\\", "\\\\");
  #endif
  const TestTempFile lua2{ fmt::format("dofile('{}')", dofilename), "lua" };
  engine->evalUserFile(lua2.filename);
  EXPECT_EQ(err.size(), 2);
  set_permission_storage(nullptr);
}

TEST(Engine, TempFiles)
{
  INIT_ENGINE_TEST()

  const TestTempFile lua{ "local path = os.tmpname()\nprint(path)", "lua" };
  auto engine = std::make_shared<Engine>();
  std::vector<std::string> print;
  std::vector<std::string> err;
  engine->ConsolePrint.connect([&print](const std::string& string) { print.push_back(string); });
  engine->ConsoleError.connect([&err](const std::string& string) { err.push_back(string); });

  const TestTempFile extensionsJSON("{}", "json");
  PermissionStorage storage(extensionsJSON.filename);
  set_permission_storage(&storage);

  app.preferences().general.allowCliScriptsFullAccess(false);

  engine->evalUserFile(lua.filename);

  EXPECT_EQ(err.size(), 1);
  EXPECT_EQ(print.size(), 0);

  storage.write(lua.filename, Permission::TemporaryFile, true);

  engine->evalUserFile(lua.filename);

  EXPECT_EQ(err.size(), 1);
  EXPECT_EQ(print.size(), 1);

  EXPECT_TRUE(base::is_file(print[0]));

  auto arbitraryFilename = fmt::format("test_{}.txt", base::current_tick());
  engine->evalCode(fmt::format("print(os.tmpname('{}'))", arbitraryFilename));

  auto path = base::join_path(base::get_temp_path(), get_app_name());
  EXPECT_TRUE(base::is_file(base::join_path(path, arbitraryFilename)));

  engine.reset();

  EXPECT_FALSE(base::is_file(print[0]));
  EXPECT_FALSE(base::is_file(base::join_path(base::get_temp_path(), arbitraryFilename)));

  set_permission_storage(nullptr);
}

TEST(Engine, LingeringObjects)
{
  INIT_ENGINE_TEST()

  auto engine = std::make_shared<Engine>();

  EXPECT_FALSE(engine->hasLingeringObjects());

  engine->evalCode(R"(
  GlobalListener1 = app.events:on('sitechange', function() end)
  GlobalListener2 = app.events:on('fgcolorchange', function() end)

  local sprite = Sprite(128, 128)
  GlobalListener3 = sprite.events:on('filenamechange', function() end))");

  EXPECT_TRUE(engine->hasLingeringObjects());

  engine->evalCode("app.events:off(GlobalListener1)");

  EXPECT_TRUE(engine->hasLingeringObjects());

  engine->evalCode("app.sprite.events:off(GlobalListener3)");

  EXPECT_TRUE(engine->hasLingeringObjects());

  engine->evalCode("app.events:off(GlobalListener2)");

  EXPECT_FALSE(engine->hasLingeringObjects());

  // TODO: We can't test Timers or Dialogs here because we're running in batch mode without a UI.
  engine->trackObject();
  EXPECT_TRUE(engine->hasLingeringObjects());
  engine->untrackObject();
  EXPECT_FALSE(engine->hasLingeringObjects());
}

TEST(Engine, LingeringSpriteEvents)
{
  INIT_ENGINE_TEST()

  auto engine = std::make_shared<Engine>();

  EXPECT_FALSE(engine->hasLingeringObjects());

  engine->evalCode(R"(
  sprite = Sprite(128, 128)
  listener = sprite.events:on('filenamechange', function() end))");

  EXPECT_TRUE(engine->hasLingeringObjects());

  engine->evalCode("sprite:close()");

  EXPECT_FALSE(engine->hasLingeringObjects());
}

TEST(Engine, MemoryLimit)
{
  INIT_ENGINE_TEST()

  auto engine = std::make_shared<Engine>();
  engine->setMemoryLimit(160'000);
  engine->ConsoleError.connect(
    [](const std::string& string) { EXPECT_EQ(string, "not enough memory"); });

  bool result = engine->evalCode(R"(
Something = "was allocated"
)");
  EXPECT_TRUE(result);
  EXPECT_GT(engine->memoryTracker().usage, engine->memoryTracker().initialUsage);

  // Need to disable the infinite loop detector to be able to allocate enough memory to pop.
  lua_sethook(engine->luaState(), NULL, 0, 0);

  result = engine->evalCode(R"(
Hog = {}
while true do
  table.insert(Hog, {"some kind of", 1, "table", false})
end
)");
  EXPECT_FALSE(result);
  EXPECT_LT(engine->memoryTracker().usage, engine->memoryTracker().limit);
}

TEST(Engine, Events)
{
  INIT_ENGINE_TEST()

  auto engine = std::make_shared<Engine>();
  std::vector<std::string> print;
  engine->ConsolePrint.connect([&print](const std::string& msg) { print.push_back(msg); });
  engine->ConsoleError.connect([](const std::string& msg) { GTEST_FATAL_FAILURE_(msg.c_str()); });

  EXPECT_TRUE(engine->evalCode(R"(
BeforeCommandID = app.events:on('beforecommand',
  function(ev)
    print('event.beforecommand.' .. ev.name)
  end)
AfterCommandID = app.events:on('aftercommand',
  function(ev)
    print('event.aftercommand.' .. ev.name)
  end)
print(AfterCommandID)
)"));

  EXPECT_EQ(print.size(), 1);

  app.context()->executeCommand(Commands::instance()->byId(CommandId::Refresh()));

  EXPECT_EQ(print.size(), 3);
  EXPECT_EQ(print[1], "event.beforecommand.Refresh");
  EXPECT_EQ(print[2], "event.aftercommand.Refresh");

  // Ensure we are not sharing events.
  auto engine2 = std::make_shared<Engine>();
  engine2->ConsoleError.connect([](const std::string& msg) { GTEST_FATAL_FAILURE_(msg.c_str()); });
  EXPECT_TRUE(engine2->evalCode(fmt::format("app.events:off({})", print[0])));

  app.context()->executeCommand(Commands::instance()->byId(CommandId::Refresh()));
  EXPECT_EQ(print[3], "event.beforecommand.Refresh");
  EXPECT_EQ(print[4], "event.aftercommand.Refresh");

  EXPECT_TRUE(engine->evalCode(R"(
app.events:off(BeforeCommandID)
app.events:off(AfterCommandID)
)"));

  app.context()->executeCommand(Commands::instance()->byId(CommandId::Refresh()));
  EXPECT_EQ(print.size(), 5);
}
#endif

int app_main(int argc, char* argv[])
{
  g_exeName = argv[0];
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
