// Aseprite
// Copyright (C) 2025  Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#include "doc_undo.h"

#include <gtest/gtest.h>

const char* g_exeName = nullptr;

#ifdef ENABLE_SCRIPTING
  #include "app.h"
  #include "app/commands/commands.h"
  #include "app/extensions.h"
  #include "app/ini_file.h"
  #include "base/fs.h"
  #include "commands/command.h"
  #include "context.h"
  #include "pref/preferences.h"

  #include "archive.h"
  #include "archive_entry.h"
  #include "base/convert_to.h"
  #include "cli/app_options.h"
  #include "doc.h"
  #include "doc/layer.h"
  #include "i18n/strings.h"
  #include "os/system.h"

  #include <fstream>

using namespace app;

namespace {
  #define EXPECT_EQ_PATH(path1, path2)                                                             \
    EXPECT_EQ(base::fix_path_separators(path1), base::fix_path_separators(path2))

// Thanks https://evanhahn.com/worlds-smallest-png/
constexpr unsigned char kValidPNG[] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00,
                                        0x00, 0x0D, 0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x01,
                                        0x00, 0x00, 0x00, 0x01, 0x08, 0x06, 0x00, 0x00, 0x00, 0x1F,
                                        0x15, 0xC4, 0x89, 0x00, 0x00, 0x00, 0x0A, 0x49, 0x44, 0x41,
                                        0x54, 0x78, 0x9C, 0x63, 0x00, 0x01, 0x00, 0x00, 0x05, 0x00,
                                        0x01, 0x0D, 0x0A, 0x2D, 0xB4, 0x00, 0x00, 0x00, 0x00, 0x49,
                                        0x45, 0x4E, 0x44, 0xAE, 0x42, 0x60, 0x82 };

using ExtFiles = std::map<std::string, std::string, std::less<>>;

class ExtTestHelper final {
public:
  explicit ExtTestHelper(const std::string& name, const ExtFiles& files) : m_name(name)
  {
    m_path = base::join_path(base::get_file_path(g_exeName), "data/extensions/" + name);

    if (base::is_directory(m_path))
      deleteFolder();

    base::make_all_directories(m_path);
    ASSERT(base::is_directory(m_path));

    for (const auto& [filename, data] : files) {
      auto file = base::join_path(m_path, filename);
      std::ofstream(file, std::ios::binary) << data;
      ASSERT(base::is_file(base::get_absolute_path(file)));
    }
  }

  ~ExtTestHelper()
  {
    if (base::is_directory(m_path))
      deleteFolder();

    if (!m_zipPath.empty() && base::is_file(m_zipPath))
      base::delete_file(m_zipPath);
  }

  void deleteFolder() const
  {
    for (const auto& item : base::list_files(m_path)) {
      const std::string& file = base::join_path(m_path, item);
      if (base::is_file(file))
        base::delete_file(file);
    }
    base::remove_directory(m_path);
  }

  Extension* get(Extensions& extensions) const
  {
    const auto it = std::find_if(extensions.begin(),
                                 extensions.end(),
                                 [this](const Extension* ext) { return ext->name() == m_name; });
    ASSERT(it != extensions.end());
    return *it;
  }

  void zip()
  {
    m_zipPath = m_name + ".aseprite-extension";
    const auto a =
      std::unique_ptr<archive, void (*)(archive*)>(archive_write_new(), [](struct archive* a) {
        EXPECT_EQ(archive_write_close(a), ARCHIVE_OK);
        EXPECT_EQ(archive_write_free(a), ARCHIVE_OK);
      });

    if (archive_write_set_format_zip(a.get()) != ARCHIVE_OK ||
        archive_write_open_filename(a.get(), m_zipPath.c_str()) != ARCHIVE_OK) {
      FAIL() << "Could create the zip file";
    }

    std::vector<char> buffer(8192);
    for (const auto& item : base::list_files(m_path, base::ItemType::Files)) {
      auto fn = base::join_path(m_path, item);

      const auto e = std::unique_ptr<struct archive_entry, void (*)(struct archive_entry*)>(
        archive_entry_new(),
        &archive_entry_free);

      archive_entry_set_pathname(e.get(), base::get_relative_path(fn, m_path).c_str());
      archive_entry_set_perm(e.get(), 0644);
      archive_entry_set_filetype(e.get(), AE_IFREG);
      archive_entry_set_size(e.get(), base::file_size(fn));
      archive_write_header(a.get(), e.get());

      std::ifstream ifs(fn, std::ios::binary);
      while (ifs.read(buffer.data(), buffer.size()) || ifs.gcount()) {
        if (archive_write_data(a.get(), buffer.data(), ifs.gcount()) < 0)
          FAIL() << "Failed to write to the zip file";
      }
    }
  }

  std::string path() const { return m_path; }
  std::string zipPath() const { return m_zipPath; }

private:
  std::string m_name;
  std::string m_path;
  std::string m_zipPath;

  DISABLE_COPYING(ExtTestHelper);
};

} // namespace

  #define INIT_EXTENSION_TEST()                                                                    \
    const os::SystemRef system = os::System::make();                                               \
    const char* argv[] = { g_exeName, "--batch" };                                                 \
    const AppOptions options(std::size(argv), argv);                                               \
    App app;                                                                                       \
    app.initialize(options);                                                                       \
    if (base::is_file("_extensions.ini"))                                                          \
      base::delete_file("_extensions.ini");                                                        \
    set_config_file("_extensions.ini")

TEST(Extensions, BasicBuiltIn)
{
  const ExtFiles extensionFiles = {
    { "package.json", R"(
{
  "name": "test-basic",
  "displayName": "Basic Extension Test",
  "contributes": {
    "scripts":  "./script.lua"
  }
})" },
    { "script.lua",   R"(
function init(plugin)
  plugin:newCommand{
    id="BasicTestCommand",
    title="Test Command",
    onclick=function() end
  }
end)" },
  };
  const ExtTestHelper e("test-basic", extensionFiles);
  INIT_EXTENSION_TEST();

  Extension* testExt = e.get(app.extensions());
  if (!testExt)
    FAIL() << "Failed to create the extension!";

  EXPECT_EQ(testExt->displayName(), "Basic Extension Test");
  EXPECT_EQ(testExt->category(), Extension::Category::Scripts);
  EXPECT_EQ(testExt->isEnabled(), true);

  EXPECT_FALSE(Commands::instance()->byId("BasicTestCommand") == nullptr);

  app.extensions().enableExtension(testExt, false);
  EXPECT_EQ(Commands::instance()->byId("BasicTestCommand"), nullptr);

  EXPECT_EQ(testExt->canBeDisabled(), false);

  app.extensions().enableExtension(testExt, true);
  EXPECT_FALSE(Commands::instance()->byId("BasicTestCommand") == nullptr);
}

TEST(Extensions, Complex)
{
  const ExtFiles extensionFiles = {
    { "package.json", R"(
{
  "name": "test-complex",
  "displayName": "Complex Extension Test",
  "description": "A Test Extension",
  "version": "0.1",
  "author": { "name": "Test",
              "email": "test@igara.com",
              "url": "https://aseprite.org/" },
  "contributes": {
    "scripts": [
        { "path": "./script.lua" }
    ],
    "ditheringMatrices": [
      {
        "id": "matrix1",
        "name": "Matrix 1",
        "path": "./matrix1.png"
      },
      {
        "id": "matrix2",
        "name": "Matrix 2",
        "path": "./matrix2.png"
      }
    ],
    "palettes": [
      { "id": "palette1", "path": "./palette.gpl" }
    ],
    "themes": [
      { "id": "theme1", "path": "." }
    ],
   "languages": [
      { "id": "klin1234",
        "path": "./klin1234.ini",
        "displayName": "Klingon" }
    ],
    "keys": [
      { "id": "keys1", "path": "./keys.aseprite-keys" }
    ]
  }
})" },
    { "script.lua", R"(
function init(plugin)
  if plugin.preferences.count == nil then
    plugin.preferences.count = 0
  end

  -- Check serialization
  if plugin.preferences.string ~= "hello" then error() end
  if plugin.preferences.bone ~= true then error() end
  if plugin.preferences.btwo ~= false then error() end
  if plugin.preferences["spc-chars"] ~= "ünicode" then error() end

  if plugin.preferences.table.one ~= 1 or plugin.preferences.table.two ~= 2 then
    error()
  end

  plugin.preferences.starting_pref = plugin.preferences.starting_pref + 1

  plugin:newMenuGroup{
    id="new_group_id",
    title="Menu Item Label",
    group="parent_group_id"
  }
  plugin:newMenuGroup{
    id="new_group_id_2",
    title="Menu Item Label 2",
    group="new_group_id"
  }
  plugin:newMenuGroup{
    id="new_group_id_3",
    title="Menu Item Label 3",
    group="new_group_id"
  }
  plugin:deleteMenuGroup("new_group_id_3")

  plugin:newCommand{
    id="TestCommand",
    title="Test Command",
    group="new_group_id_2",
    onclick=function()
      plugin.preferences.count = plugin.preferences.count + 1
    end
  }

  plugin:newCommand{
    id="DeleteMeCommand",
    group="new_group_id_2",
    title="For deletion",
    onclick=function() end
  }

  plugin:deleteCommand("DeleteMeCommand")
end

function exit(plugin)
end
)" },
    { "__pref.lua",
     R"(return {starting_pref=1234,string="hello",bone=true,btwo=false,table={one=1,two=2},["spc-chars"]="ünicode"})" },
    { "matrix1.png", std::string(reinterpret_cast<const char*>(kValidPNG), sizeof(kValidPNG)) }
  };
  const ExtTestHelper e("test-complex", extensionFiles);
  INIT_EXTENSION_TEST();

  Extension* testExt = e.get(app.extensions());
  if (!testExt)
    FAIL() << "Failed to create the extension!";

  int menuItemRemoveCount = 0;
  std::vector<std::string> menuItemGroupIds;

  EXPECT_EQ_PATH(testExt->path(), base::get_absolute_path(e.path()));
  EXPECT_EQ(testExt->displayName(), "Complex Extension Test");
  EXPECT_EQ(testExt->category(), Extension::Category::Multiple);
  EXPECT_EQ(testExt->version(), "0.1");

  EXPECT_EQ(testExt->keys().size(), 1);
  EXPECT_EQ(testExt->languages().size(), 1);
  EXPECT_EQ(testExt->themes().size(), 1);
  EXPECT_EQ(testExt->palettes().size(), 1);

  EXPECT_EQ(testExt->hasScripts(), true);
  EXPECT_EQ(testExt->hasDitheringMatrices(), true);

  EXPECT_EQ(testExt->canBeUninstalled(), false);
  EXPECT_EQ(testExt->isCurrentTheme(), false);
  EXPECT_EQ(testExt->isDefaultTheme(), false);

  testExt->MenuItemRemoveWidget.connect(
    [&menuItemRemoveCount](ui::Widget*) { menuItemRemoveCount++; });
  testExt->MenuItemRemoveCommand.connect(
    [&menuItemRemoveCount](Command*) { menuItemRemoveCount++; });
  testExt->MenuGroupRemove.connect(
    [&menuItemGroupIds](const std::string& id) { menuItemGroupIds.push_back(id); });

  EXPECT_TRUE(Commands::instance()->byId("TestCommand") != nullptr);

  EXPECT_FALSE(app.extensions().ditheringMatrix("matrix0"));
  EXPECT_TRUE(app.extensions().ditheringMatrix("matrix1"));
  // Should throw because we can't find the file
  EXPECT_THROW(app.extensions().ditheringMatrix("matrix2"), std::runtime_error);

  EXPECT_EQ(app.extensions().palettePath("palette0"), "");
  EXPECT_EQ_PATH(base::get_absolute_path(app.extensions().palettePath("palette1")),
                 base::get_absolute_path(base::join_path(e.path(), "./palette.gpl")));

  EXPECT_EQ(app.extensions().themePath("theme0"), "");
  EXPECT_EQ_PATH(base::get_absolute_path(app.extensions().themePath("theme1")),
                 base::get_absolute_path(base::join_path(e.path(), ".")));

  EXPECT_EQ(app.extensions().languagePath("nolang"), "");
  EXPECT_EQ_PATH(base::get_absolute_path(app.extensions().languagePath("klin1234")),
                 base::get_absolute_path(base::join_path(e.path(), "./klin1234.ini")));

  // This should be deleted in the script.
  EXPECT_FALSE(Commands::instance()->byId("DeleteMeCommand"));

  // Run the script command, we'll later check the output was good.
  auto* command = Commands::instance()->byId("TestCommand");
  EXPECT_TRUE(command != nullptr);
  auto* ctx = App::instance()->context();
  ctx->executeCommand(command);
  EXPECT_EQ(ctx->commandResult().type(), CommandResult::kOk);

  app.extensions().enableExtension(testExt, false);

  EXPECT_FALSE(Commands::instance()->byId("TestCommand"));

  // Menu items
  EXPECT_EQ(menuItemRemoveCount, 1);
  EXPECT_EQ(menuItemGroupIds.size(), 2);
  EXPECT_EQ(menuItemGroupIds[0], "new_group_id_2");
  EXPECT_EQ(menuItemGroupIds[1], "new_group_id");

  // Make sure all our values got serialized correctly into __pref.lua
  const std::ifstream ifs(base::join_path(e.path(), "__pref.lua"));
  std::stringstream buffer;
  buffer << ifs.rdbuf();
  const auto pref = buffer.str();
  const std::vector<std::string> serializedResults = {
    "count=1",
    "starting_pref=1235",
    "string=\"hello\"",
    "bone=true",
    "btwo=false",
    R"(["spc-chars"]="ünicode")",
    // Separating table{one=1,two=2} because serialization is not deterministic in the order they
    // appear but all that matters is that they  show up.
    "table={",
    "one=1",
    "two=2",
  };
  for (const auto& expected : serializedResults) {
    if (pref.find(expected) == std::string::npos)
      FAIL() << "Could not find serialized value " << expected << " in __pref.lua: " << pref;
  }

  // Check all signals
  int extSignalCount = 0;
  auto extSignalSum = [&extSignalCount, testExt](const Extension* ext) {
    if (ext != nullptr)
      EXPECT_EQ(ext, testExt);

    extSignalCount++;
  };
  app.extensions().KeysChange.connect(extSignalSum);
  app.extensions().LanguagesChange.connect(extSignalSum);
  app.extensions().ThemesChange.connect(extSignalSum);
  app.extensions().PalettesChange.connect(extSignalSum);
  app.extensions().DitheringMatricesChange.connect(extSignalSum);
  app.extensions().ScriptsChange.connect(extSignalSum);

  const auto prevPaletteCount = app.extensions().palettes().size();
  const auto prevMatrixCount = app.extensions().ditheringMatrices().size();

  app.extensions().enableExtension(testExt, true);

  EXPECT_EQ(app.extensions().palettes().size(), prevPaletteCount + 1);
  EXPECT_EQ(app.extensions().ditheringMatrices().size(), prevMatrixCount + 2);

  EXPECT_EQ(extSignalCount, 6);
  EXPECT_TRUE(Commands::instance()->byId("TestCommand"));

  app.run(false);

  EXPECT_FALSE(Commands::instance()->byId("TestCommand"));
}

TEST(Extensions, BasicZipInstall)
{
  const ExtFiles extensionFiles = {
    { "package.json", R"(
{
  "name": "test-zipped",
  "displayName": "Zipped Extension Test",
  "contributes": {
    "scripts":  "./script.lua"
  }
})" },
    { "script.lua",   R"(
function init(plugin)
  plugin:newCommand{
    id="BasicTestCommand",
    title="Test Command",
    onclick=function() end
  }
end)" },
  };

  ExtTestHelper e("test-zipped", extensionFiles);
  e.zip();
  e.deleteFolder();
  INIT_EXTENSION_TEST();

  auto info = app.extensions().getCompressedExtensionInfo(e.zipPath());

  EXPECT_FALSE(base::is_directory(info.dstPath));

  app.extensions().installCompressedExtension(e.zipPath(), info);

  EXPECT_TRUE(base::is_directory(info.dstPath));

  Extension* testExt = e.get(app.extensions());
  EXPECT_TRUE(testExt != nullptr);
  EXPECT_EQ(testExt->displayName(), "Zipped Extension Test");
  EXPECT_EQ(testExt->category(), Extension::Category::Scripts);

  app.extensions().uninstallExtension(testExt, DeletePluginPref::Yes);

  EXPECT_FALSE(base::is_directory(info.dstPath));
}

TEST(Extensions, CustomFormatsLoad)
{
  const ExtFiles extensionFiles = {
    { "package.json", R"(
{
  "name": "test-custom-formats-load",
  "displayName": "Custom Formats (Load) Test",
  "contributes": {
    "scripts":  "./script.lua"
  }
})" },
    { "script.lua",   R"(
function init(plugin)
  plugin:newFileFormat{
    name="lua",
    supports=FormatSupport.INDEXED,
    onload=function(filename, file)
      if not filename then
        error("Filename is empty")
      end
      if not file then
        error("No file object")
      end

      local sprite = Sprite(128, 128)
      sprite.layers[1].name = "Test"
      sprite.cels[1].image:drawPixel(4, 4, Color{ r=255, g=0, b=0, a=255 })
      return sprite
    end,
    extensions={"lua"}
  }
end)" },
  };

  const ExtTestHelper e("test-custom-formats-load", extensionFiles);
  INIT_EXTENSION_TEST();

  const Extension* testExt = e.get(app.extensions());
  EXPECT_TRUE(testExt != nullptr);
  EXPECT_EQ(testExt->category(), Extension::Category::Scripts);
  EXPECT_EQ(testExt->hasFileFormats(), true);

  auto* command = Commands::instance()->byId(CommandId::OpenFile());

  // TODO: Test options.
  auto* ctx = App::instance()->context();
  EXPECT_TRUE(ctx->activeDocument() == nullptr);

  ctx->executeCommand(
    command,
    {
      { "filename", base::get_absolute_path(base::join_path(e.path(), "script.lua")) },
  });

  EXPECT_EQ(ctx->commandResult().type(), CommandResult::kOk);

  EXPECT_TRUE(ctx->activeDocument() != nullptr);

  const auto* sprite = ctx->activeDocument()->sprite();
  EXPECT_TRUE(sprite != nullptr);

  EXPECT_EQ(sprite->size(), gfx::Size(128, 128));
  EXPECT_EQ(sprite->allLayers().at(0)->name(), "Test");
  EXPECT_EQ(sprite->allLayers().at(0)->cel(0)->image()->getPixel(4, 4), gfx::rgba(255, 0, 0, 255));

  EXPECT_EQ(ctx->activeDocument()->isUndoing(), false);
  EXPECT_EQ(ctx->activeDocument()->undoHistory()->canRedo(), false);
  EXPECT_EQ(ctx->activeDocument()->undoHistory()->canUndo(), false);

  EXPECT_EQ(ctx->activeDocument()->isModified(), false);
  EXPECT_EQ(ctx->activeDocument()->isAssociatedToFile(), true);
}

TEST(Extensions, CustomFormatsSave)
{
  const ExtFiles extensionFiles = {
    { "package.json", R"(
{
  "name": "test-custom-formats-save",
  "displayName": "Custom Formats (Save) Test",
  "contributes": {
    "scripts":  "./script.lua"
  }
})" },
    { "script.lua",   R"(
function init(plugin)
  plugin:newFileFormat{
    name="lua",
    onsave=function(filename, file, sprite)
       if (sprite.layers[1].name ~= "Test") then error() end
       file:write("return true")
       file:close() -- TODO: Make sure we call this anyway for the user's benefit?
       return sprite
    end,
    extensions={"lua"}
  }
end)" },
  };

  const ExtTestHelper e("test-custom-formats-save", extensionFiles);
  INIT_EXTENSION_TEST();

  const Extension* testExt = e.get(app.extensions());
  EXPECT_TRUE(testExt != nullptr);
  EXPECT_EQ(testExt->hasFileFormats(), true);

  if (base::is_file("./save_result.lua"))
    base::delete_file("./save_result.lua"); // TODO move to constant

  // Create and manipulate the file we're gonna be saving
  auto* ctx = App::instance()->context();
  {
    ctx->executeCommand(Commands::instance()->byId(CommandId::NewFile()),
                        {
                          { "width",  "128" },
                          { "height", "128" }
    });
    EXPECT_EQ(ctx->commandResult().type(), CommandResult::kOk);
    EXPECT_TRUE(ctx->activeDocument() != nullptr);

    const auto* sprite = ctx->activeDocument()->sprite();
    EXPECT_EQ(sprite->size(), gfx::Size(128, 128));

    ctx->executeCommand(Commands::instance()->byId(CommandId::SpriteSize()),
                        {
                          { "width",  "64" },
                          { "height", "64" }
    });

    EXPECT_EQ(sprite->size(), gfx::Size(64, 64));
    sprite->allLayers().at(0)->setName("Test");
    sprite->allLayers().at(0)->cel(0)->image()->putPixel(4, 4, gfx::rgba(255, 0, 0, 255));

    EXPECT_EQ(ctx->activeDocument()->isModified(), true);
    EXPECT_EQ(ctx->activeDocument()->isAssociatedToFile(), false);
  }

  ctx->executeCommand(Commands::instance()->byId(CommandId::SaveFile()),
                      {
                        { "filename", "./save_result.lua" }
  });

  EXPECT_EQ(ctx->commandResult().type(), CommandResult::kOk);

  EXPECT_TRUE(base::is_file("./save_result.lua"));

  const std::ifstream ifs("./save_result.lua");
  std::stringstream buffer;
  buffer << ifs.rdbuf();
  const auto saveResult = buffer.str();

  EXPECT_EQ(saveResult, "return true");

  EXPECT_EQ(ctx->activeDocument()->isModified(), false);
  EXPECT_EQ(ctx->activeDocument()->isAssociatedToFile(), true);
}

TEST(Extensions, CustomFormatConflict)
{
  const ExtFiles extensionFiles = {
    { "package.json", R"(
{
  "name": "test-custom-formats-conflict1",
  "displayName": "Custom format conflict with a native format",
  "contributes": {
    "scripts":  "./script.lua"
  }
})" },
    { "script.lua",   R"(
function init(plugin)
  plugin:newFileFormat{
    name="conflict1",
    onsave=function(filename, file, sprite) end,
    onload=function(filename, file) end,
    extensions={"png"}
  }
end
)" },
  };
  const ExtTestHelper e("test-custom-formats-conflict1", extensionFiles);
  INIT_EXTENSION_TEST();

  const Extension* testExt = e.get(app.extensions());
  EXPECT_TRUE(testExt != nullptr);

  EXPECT_EQ(testExt->hasFileFormats(), false);
}

TEST(Extensions, CustomFormatConflictExt)
{
  const ExtFiles extensionFiles = {
    { "package.json", R"(
{
  "name": "test-custom-formats-conflict2",
  "displayName": "Custom format conflict with another extension",
  "contributes": {
    "scripts":  "./script.lua"
  }
})" },
    { "script.lua",   R"(
function init(plugin)
  plugin:newFileFormat{
    name="conflict2",
    onsave=function(filename, file, sprite) end,
    onload=function(filename, file) end,
    extensions={"format1", "format2", "format3", "format4"}
  }
end
)" },
  };
  const ExtFiles extensionFiles2 = {
    { "package.json", R"(
{
  "name": "test-custom-formats-conflict3",
  "displayName": "The extension that does the conflicting",
  "contributes": {
    "scripts":  "./script.lua"
  }
})" },
    { "script.lua",   R"(
function init(plugin)
  plugin:newFileFormat{
    name="conflict3",
    onsave=function(filename, file, sprite) end,
    onload=function(filename, file) end,
    extensions={"format2", "format20"}
  }
end
)" },
  };
  const ExtTestHelper e1("test-custom-formats-conflict2", extensionFiles);
  const ExtTestHelper e2("test-custom-formats-conflict3", extensionFiles2);
  INIT_EXTENSION_TEST();

  const Extension* testExt = e1.get(app.extensions());
  EXPECT_EQ(testExt->hasFileFormats(), true);

  const Extension* testExt2 = e2.get(app.extensions());
  EXPECT_TRUE(testExt2 != nullptr);
  EXPECT_EQ(testExt2->hasFileFormats(), false);
}

TEST(Extensions, CustomFormatsFailure)
{
  const ExtFiles extensionFiles = {
    { "package.json", R"(
{
  "name": "test-custom-formats-failure",
  "displayName": "Custom Formats Failure Test",
  "contributes": {
    "scripts":  "./script.lua"
  }
})" },
    { "script.lua",   R"(
local writeHandle = nil
local loadHandle = nil

function init(plugin)
  plugin:newFileFormat{
    name="lua",
    onsave=function(filename, file)
      writeHandle = file
      return false
    end,
    onload=function(filename, file)
      loadHandle = file
      bad:lua(1)
    end,
    extensions={"lua"}
  }
  plugin:newCommand{
    id="UseTheHandle",
    title="Use The Handle",
    onclick=function()
      writeHandle:write("!!!")
      writeHandle:flush()
      loadHandle:read("*a")
     end
  }
end)" },
  };

  const ExtTestHelper e("test-custom-formats-failure", extensionFiles);
  INIT_EXTENSION_TEST();

  const Extension* testExt = e.get(app.extensions());
  EXPECT_TRUE(testExt != nullptr);
  EXPECT_EQ(testExt->hasFileFormats(), true);
  EXPECT_EQ(App::instance()->context()->activeDocument(), nullptr);

  auto* ctx = App::instance()->context();
  ctx->executeCommand(
    Commands::instance()->byId(CommandId::OpenFile()),
    {
      { "filename", base::get_absolute_path(base::join_path(e.path(), "script.lua")) },
  });
  EXPECT_EQ(App::instance()->context()->activeDocument(), nullptr);

  ctx->executeCommand(Commands::instance()->byId(CommandId::NewFile()),
                      {
                        { "width",  "64" },
                        { "height", "64" }
  });
  EXPECT_EQ(ctx->commandResult().type(), CommandResult::kOk);
  EXPECT_EQ(App::instance()->context()->activeDocument()->isModified(), false);
  ctx->executeCommand(Commands::instance()->byId(CommandId::SpriteSize()),
                      {
                        { "width",  "32" },
                        { "height", "32" }
  });
  EXPECT_EQ(ctx->commandResult().type(), CommandResult::kOk);
  EXPECT_EQ(App::instance()->context()->activeDocument()->isModified(), true);

  const auto& savefile = base::get_absolute_path(base::join_path(e.path(), "should_be_empty.lua"));
  EXPECT_FALSE(base::is_file(savefile));
  ctx->executeCommand(Commands::instance()->byId(CommandId::SaveFile()),
                      {
                        { "filename", savefile }
  });
  EXPECT_EQ(ctx->commandResult().type(), CommandResult::kOk);
  EXPECT_TRUE(base::is_file(savefile));
  EXPECT_EQ(App::instance()->context()->activeDocument()->isModified(), true);

  ctx->executeCommand(Commands::instance()->byId("UseTheHandle"));

  const std::ifstream ifs(savefile);
  std::stringstream buffer;
  buffer << ifs.rdbuf();
  EXPECT_EQ(buffer.str(), "");
}

TEST(Extensions, CustomFormatsRecoverDocList)
{
  const ExtFiles extensionFiles = {
    { "package.json", R"(
{
  "name": "test-custom-formats-recover-doclist",
  "displayName": "Custom Formats Recover Doc List",
  "contributes": {
    "scripts":  "./script.lua"
  }
})" },
    { "script.lua",   R"(
function init(plugin)
  plugin:newFileFormat{
    name="lua",
    onload=function()
      local s1 = Sprite(10, 10)
      local s2 = Sprite(15, 15)
      bad:lua(1)
    end,
    extensions={"lua"}
  }
end)" },
  };

  const ExtTestHelper e("test-custom-formats-recover-doclist", extensionFiles);
  INIT_EXTENSION_TEST();

  auto* ctx = App::instance()->context();

  const Extension* testExt = e.get(app.extensions());
  EXPECT_TRUE(testExt != nullptr);
  EXPECT_EQ(testExt->hasFileFormats(), true);
  EXPECT_EQ(ctx->activeDocument(), nullptr);

  for (int i = 0; i < 3; i++) {
    ctx->executeCommand(Commands::instance()->byId(CommandId::NewFile()),
                        {
                          { "width",  "64" },
                          { "height", "64" }
    });
    EXPECT_EQ(ctx->commandResult().type(), CommandResult::kOk);
  }
  EXPECT_EQ(ctx->documents().size(), 3);
  auto* prevActiveDoc = ctx->activeDocument();

  ctx->executeCommand(
    Commands::instance()->byId(CommandId::OpenFile()),
    {
      { "filename", base::get_absolute_path(base::join_path(e.path(), "script.lua")) },
  });

  EXPECT_EQ(ctx->activeDocument(), prevActiveDoc);
  EXPECT_EQ(ctx->documents().size(), 3);
}
#endif

int app_main(int argc, char* argv[])
{
  g_exeName = argv[0];
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
