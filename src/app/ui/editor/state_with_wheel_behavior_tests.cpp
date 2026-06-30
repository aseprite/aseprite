// Aseprite
// Copyright (C) 2026  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#include "tests/app_test.h"

#include "app/app.h"
#include "app/cli/app_options.h"
#include "app/doc.h"
#include "app/doc_api.h"
#include "app/doc_undo.h"
#include "app/pref/preferences.h"
#include "app/tools/active_tool.h"
#include "app/tools/tool_box.h"
#include "app/tx.h"
#include "app/ui/doc_view.h"
#include "app/ui/editor/editor.h"
#include "app/ui/editor/state_with_wheel_behavior.h"
#include "app/ui/keyboard_shortcuts.h"
#include "app/ui_context.h"
#include "os/system.h"
#include "ui/message.h"

#include <set>
#include <vector>

using namespace app;
using namespace ui;

namespace {

class WheelBehaviorTestState : public StateWithWheelBehavior {
public:
  using StateWithWheelBehavior::processWheelAction;

  // Needs override as processWheelAction onToolChange uses ToolBar::instance()
  // that doesn't exist in these tests
  void onToolChange(tools::Tool* tool) override
  {
    auto* atm = App::instance()->activeToolManager();
    if (atm)
      atm->setSelectedTool(tool);
  }
};

MouseMessage make_wheel_message(KeyModifiers modifiers,
                                MouseButton button,
                                bool isX1Pressed,
                                bool isX2Pressed,
                                const gfx::Point& delta)
{
  return MouseMessage(kMouseWheelMessage,
                      PointerType::Mouse,
                      button,
                      modifiers,
                      gfx::Point(0, 0),
                      isX1Pressed,
                      isX2Pressed,
                      delta,
                      false);
}

} // namespace

TEST(StateWithWheelBehavior, UndoRedoWithSideButton)
{
  os::SystemRef system = os::System::make();
  const char* argv[] = { "state_with_wheel_behaviour_tests", "--batch" };
  const app::AppOptions options(sizeof(argv) / sizeof(argv[0]), argv);
  app::App app;
  app.initialize(options);

  // Create context
  auto* ctx = UIContext::instance();
  ASSERT_TRUE(ctx != nullptr);

  // Create doc
  Doc* doc = ctx->documents().add(2, 2);
  ASSERT_TRUE(doc != nullptr);

  // Create sprite
  Sprite* sprite = doc->sprite();
  ASSERT_TRUE(sprite != nullptr);
  const int originalWidth = sprite->width();
  const int newWidth = originalWidth + 1;

  // Resize sprite
  {
    Tx tx(sprite, "Resize Sprite");
    doc->getApi(tx).setSpriteSize(sprite, newWidth, sprite->height());
    tx.commit();
  }

  // sprite->width() should have the new with and doc should now have undo history
  EXPECT_EQ(newWidth, sprite->width());
  EXPECT_TRUE(doc->undoHistory()->canUndo());

  // Create keyboard shortcut for undo/redo wheel action
  auto* ks = KeyboardShortcuts::instance();
  ks->clear();
  KeyPtr key = ks->wheelAction(WheelAction::UndoRedo);
  key->add(Shortcut(kKeyNoneModifier, kButtonX1), KeySource::UserDefined, *ks);

  WheelBehaviorTestState state;

  // Create undo wheel message
  auto msg = make_wheel_message(kKeyNoneModifier, kButtonX1, true, false, gfx::Point(0, -1));
  // Create undo action with message
  WheelAction action = ks->getWheelActionFromMouseMessage(KeyContext::MouseWheel, &msg);
  EXPECT_EQ(WheelAction::UndoRedo, action);

  // Apply undo action
  gfx::Point delta = msg.wheelDelta();
  state.processWheelAction(nullptr,
                           action,
                           msg.position(),
                           delta,
                           delta.x + delta.y,
                           StateWithWheelBehavior::ScrollBigSteps::Off,
                           StateWithWheelBehavior::PreciseWheel::Off,
                           StateWithWheelBehavior::FromMouseWheel::On);

  // sprite->with() should be back to original width and doc
  // should have history do redo
  EXPECT_EQ(originalWidth, sprite->width());
  EXPECT_TRUE(doc->undoHistory()->canRedo());

  // Create redo message
  msg = make_wheel_message(kKeyNoneModifier, kButtonX1, true, false, gfx::Point(0, 1));
  // Create redo action with message
  action = ks->getWheelActionFromMouseMessage(KeyContext::MouseWheel, &msg);
  EXPECT_EQ(WheelAction::UndoRedo, action);

  // Apply redo action
  delta = msg.wheelDelta();
  state.processWheelAction(nullptr,
                           action,
                           msg.position(),
                           delta,
                           delta.x + delta.y,
                           StateWithWheelBehavior::ScrollBigSteps::Off,
                           StateWithWheelBehavior::PreciseWheel::Off,
                           StateWithWheelBehavior::FromMouseWheel::On);

  // sprite->with() should be back to new width
  EXPECT_EQ(newWidth, sprite->width());

  app.run(false);
}

TEST(StateWithWheelBehavior, ChangeToolCustomToolsetWithWheelAction)
{
  os::SystemRef system = os::System::make();
  const char* argv[] = { "state_with_wheel_behaviour_tests", "--batch" };
  const app::AppOptions options(sizeof(argv) / sizeof(argv[0]), argv);
  app::App app;
  app.initialize(options);

  // Create context
  auto* ctx = UIContext::instance();
  ASSERT_TRUE(ctx != nullptr);

  // Create custom toolset
  auto& customToolset = Preferences::instance().customToolset;
  for (OptionBase* option : customToolset.optionList()) {
    option->resetToDefault();
  }
  customToolset.rectangularMarquee(true);
  customToolset.pencil(true);
  customToolset.text(true);

  // Create toolbox
  auto* toolBox = App::instance()->toolBox();
  ASSERT_TRUE(toolBox != nullptr);

  // Create custom toolset vector to use for comparisons
  std::set<std::string> customToolsetIds = {
    "rectangular_marquee",
    "pencil",
    "text",
  };
  std::vector<tools::Tool*> customToolsetTools;
  for (tools::Tool* tool : *toolBox) {
    if (customToolsetIds.count(tool->getId()))
      customToolsetTools.push_back(tool);
  }
  ASSERT_EQ(3u, customToolsetTools.size());

  // Create tool manager and set first tool as current
  auto* atm = App::instance()->activeToolManager();
  ASSERT_TRUE(atm != nullptr);

  atm->setSelectedTool(customToolsetTools[0]);
  EXPECT_EQ(customToolsetTools[0], atm->activeTool());

  // Create keyboard shortcut for change tool (custom toolset) wheel action
  auto* ks = KeyboardShortcuts::instance();
  ks->clear();
  KeyPtr key = ks->wheelAction(WheelAction::ToolCustomToolset);
  key->add(Shortcut(kKeyNoneModifier, kButtonX1), KeySource::UserDefined, *ks);

  WheelBehaviorTestState state;

  // Create tool wheel message (scroll forward)
  auto msg = make_wheel_message(kKeyNoneModifier, kButtonX1, true, false, gfx::Point(0, 1));
  // Create tool wheel action with message
  WheelAction action = ks->getWheelActionFromMouseMessage(KeyContext::MouseWheel, &msg);
  EXPECT_EQ(WheelAction::ToolCustomToolset, action);

  // Apply tool action
  gfx::Point delta = msg.wheelDelta();
  state.processWheelAction(nullptr,
                           action,
                           msg.position(),
                           delta,
                           delta.x + delta.y,
                           StateWithWheelBehavior::ScrollBigSteps::Off,
                           StateWithWheelBehavior::PreciseWheel::Off,
                           StateWithWheelBehavior::FromMouseWheel::On);

  EXPECT_EQ(customToolsetTools[1], atm->activeTool());

  // Scroll forward again
  msg = make_wheel_message(kKeyNoneModifier, kButtonX1, true, false, gfx::Point(0, 1));
  action = ks->getWheelActionFromMouseMessage(KeyContext::MouseWheel, &msg);
  EXPECT_EQ(WheelAction::ToolCustomToolset, action);

  delta = msg.wheelDelta();
  state.processWheelAction(nullptr,
                           action,
                           msg.position(),
                           delta,
                           delta.x + delta.y,
                           StateWithWheelBehavior::ScrollBigSteps::Off,
                           StateWithWheelBehavior::PreciseWheel::Off,
                           StateWithWheelBehavior::FromMouseWheel::On);

  EXPECT_EQ(customToolsetTools[2], atm->activeTool());

  // Scroll forward again to check if its going back to the first
  msg = make_wheel_message(kKeyNoneModifier, kButtonX1, true, false, gfx::Point(0, 1));
  action = ks->getWheelActionFromMouseMessage(KeyContext::MouseWheel, &msg);
  EXPECT_EQ(WheelAction::ToolCustomToolset, action);

  delta = msg.wheelDelta();
  state.processWheelAction(nullptr,
                           action,
                           msg.position(),
                           delta,
                           delta.x + delta.y,
                           StateWithWheelBehavior::ScrollBigSteps::Off,
                           StateWithWheelBehavior::PreciseWheel::Off,
                           StateWithWheelBehavior::FromMouseWheel::On);

  EXPECT_EQ(customToolsetTools[0], atm->activeTool());

  // Scroll backward to check if it goes to the last tool
  msg = make_wheel_message(kKeyNoneModifier, kButtonX1, true, false, gfx::Point(0, -1));
  action = ks->getWheelActionFromMouseMessage(KeyContext::MouseWheel, &msg);
  EXPECT_EQ(WheelAction::ToolCustomToolset, action);

  delta = msg.wheelDelta();
  state.processWheelAction(nullptr,
                           action,
                           msg.position(),
                           delta,
                           delta.x + delta.y,
                           StateWithWheelBehavior::ScrollBigSteps::Off,
                           StateWithWheelBehavior::PreciseWheel::Off,
                           StateWithWheelBehavior::FromMouseWheel::On);

  EXPECT_EQ(customToolsetTools[2], atm->activeTool());

  app.run(false);
}

TEST(StateWithWheelBehavior, ChangePlaybackSpeedWithWheelAction)
{
  os::SystemRef system = os::System::make();
  const char* argv[] = { "state_with_wheel_behaviour_tests" };
  const app::AppOptions options(sizeof(argv) / sizeof(argv[0]), argv);
  app::App app;
  app.initialize(options);

  // Test needs gui
  if (!app.isGui())
    GTEST_SKIP() << "Playback speed wheel action requires GUI";

  // Create context
  auto* ctx = UIContext::instance();
  ASSERT_TRUE(ctx != nullptr);

  // Create doc
  Doc* doc = ctx->documents().add(2, 2);
  ASSERT_TRUE(doc != nullptr);

  // Create editor
  DocView* docView = ctx->getFirstDocView(doc);
  ASSERT_TRUE(docView != nullptr);
  Editor* editor = docView->editor();
  ASSERT_TRUE(editor != nullptr);

  // Set base animation speed multiplier
  editor->setAnimationSpeedMultiplier(1.0);
  EXPECT_NEAR(1.0, editor->getAnimationSpeedMultiplier(), 0.0001);

  // Create keyboard shortcut for playback speed wheel action
  auto* ks = KeyboardShortcuts::instance();
  ks->clear();
  KeyPtr key = ks->wheelAction(WheelAction::PlaybackSpeed);
  key->add(Shortcut(kKeyNoneModifier, kButtonX1), KeySource::UserDefined, *ks);

  WheelBehaviorTestState state;

  // Create playback speed wheel message (scroll backward)
  auto msg = make_wheel_message(kKeyNoneModifier, kButtonX1, true, false, gfx::Point(0, -1));
  // Create playback speed wheel action with message
  WheelAction action = ks->getWheelActionFromMouseMessage(KeyContext::MouseWheel, &msg);
  EXPECT_EQ(WheelAction::PlaybackSpeed, action);

  // Apply playback speed action
  gfx::Point delta = msg.wheelDelta();
  state.processWheelAction(editor,
                           action,
                           msg.position(),
                           delta,
                           delta.x + delta.y,
                           StateWithWheelBehavior::ScrollBigSteps::Off,
                           StateWithWheelBehavior::PreciseWheel::Off,
                           StateWithWheelBehavior::FromMouseWheel::On);

  EXPECT_NEAR(1.5, editor->getAnimationSpeedMultiplier(), 0.0001);

  // Scroll forward to decrease speed
  msg = make_wheel_message(kKeyNoneModifier, kButtonX1, true, false, gfx::Point(0, 1));
  action = ks->getWheelActionFromMouseMessage(KeyContext::MouseWheel, &msg);
  EXPECT_EQ(WheelAction::PlaybackSpeed, action);

  delta = msg.wheelDelta();
  state.processWheelAction(editor,
                           action,
                           msg.position(),
                           delta,
                           delta.x + delta.y,
                           StateWithWheelBehavior::ScrollBigSteps::Off,
                           StateWithWheelBehavior::PreciseWheel::Off,
                           StateWithWheelBehavior::FromMouseWheel::On);

  EXPECT_NEAR(1.0, editor->getAnimationSpeedMultiplier(), 0.0001);

  app.run(false);
}
