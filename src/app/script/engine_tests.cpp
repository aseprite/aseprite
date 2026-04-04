// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#include <gtest/gtest.h>

const char* g_exeName = nullptr;

#ifdef ENABLE_SCRIPTING
  #include "app/app.h"
  #include "app/cli/app_options.h"
  #include "app/commands/commands.h"
  #include "app/context.h"
  #include "app/doc.h"
  #include "app/script/luacpp.h"
  #include "base/fs.h"
  #include "base/time.h"
  #include "engine.h"
  #include "fmt/args.h"
  #include "os/os.h"

  #include <fstream>

using namespace app;
using namespace script;

  #define INIT_ENGINE_TEST(...)                                                                    \
    const os::SystemRef system = os::System::make();                                               \
    const char* argv[] = { g_exeName, "--batch", __VA_ARGS__ };                                    \
    const AppOptions options(std::size(argv), argv);                                               \
    App app;                                                                                       \
    app.initialize(options);

// TODO: Move to a general file
class TestTempFile {
public:
  explicit TestTempFile(const std::string& content = "", const std::string& ext = "")
  {
    static int i = 0;
    static const std::string dir = testing::TempDir();
    filename = base::join_path(
      dir,
      fmt::format("tmp_{}_{}{}", base::current_tick(), i, ext.empty() ? "" : "." + ext));
    std::ofstream out(filename);
    out << content;
    i++;
  }

  ~TestTempFile() { base::delete_file(filename); }

  std::string filename;
};

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
  const TestTempFile tmp{};
  std::vector<std::string> print;
  engine->ConsolePrint.connect([&print](const std::string& string) { print.push_back(string); });

  engine->setPrintEvalResult(true);
  engine->evalCode(fmt::format(R"(
    io.open("{}")
  )",
                               tmp.filename));

  EXPECT_TRUE(print.empty());
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
  function()
    print('event.beforecommand')
  end)
AfterCommandID = app.events:on('aftercommand',
  function()
    print('event.aftercommand')
  end)
print(AfterCommandID)
)"));

  EXPECT_EQ(print.size(), 1);

  app.context()->executeCommand(Commands::instance()->byId(CommandId::Refresh()));

  EXPECT_EQ(print.size(), 3);
  EXPECT_EQ(print[1], "event.beforecommand");
  EXPECT_EQ(print[2], "event.aftercommand");

  // Ensure we are not sharing events.
  auto engine2 = std::make_shared<Engine>();
  engine2->ConsoleError.connect([](const std::string& msg) { GTEST_FATAL_FAILURE_(msg.c_str()); });
  EXPECT_TRUE(engine2->evalCode(fmt::format("app.events:off({})", print[0])));

  app.context()->executeCommand(Commands::instance()->byId(CommandId::Refresh()));
  EXPECT_EQ(print[3], "event.beforecommand");
  EXPECT_EQ(print[4], "event.aftercommand");

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
