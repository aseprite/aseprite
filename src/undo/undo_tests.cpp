// Aseprite Undo Library
// Copyright (C) 2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gtest/gtest.h>

#include "undo/undo_command.h"
#include "undo/undo_history.h"

#ifdef _WIN32
  #include <functional>
  using namespace std;
#else
  #include <tr1/functional>
  using namespace std::tr1;
#endif

using namespace undo;

class Cmd : public UndoCommand {
public:
  template<typename UndoT, typename RedoT>
  Cmd(RedoT redoFunc, UndoT undoFunc)
    : m_redo(redoFunc), m_undo(undoFunc) {
  }
  void redo() override { m_redo(); }
  void undo() override { m_undo(); }
private:
  function<void()> m_redo;
  function<void()> m_undo;
};

TEST(Undo, Basics)
{
  UndoHistory history;

  int model = 0;
  EXPECT_EQ(0, model);

  Cmd cmd1(
    [&]{ model = 1; },          // redo
    [&]{ model = 0; });         // undo
  Cmd cmd2(
    [&]{ model = 2; },          // redo
    [&]{ model = 1; });         // undo

  EXPECT_EQ(0, model);
  cmd1.redo();
  EXPECT_EQ(1, model);
  cmd2.redo();
  EXPECT_EQ(2, model);

  EXPECT_FALSE(history.canUndo());
  EXPECT_FALSE(history.canRedo());
  history.add(&cmd1);
  EXPECT_TRUE(history.canUndo());
  EXPECT_FALSE(history.canRedo());
  history.add(&cmd2);
  EXPECT_TRUE(history.canUndo());
  EXPECT_FALSE(history.canRedo());

  history.undo();
  EXPECT_EQ(1, model);
  EXPECT_TRUE(history.canUndo());
  EXPECT_TRUE(history.canRedo());
  history.undo();
  EXPECT_EQ(0, model);
  EXPECT_FALSE(history.canUndo());
  EXPECT_TRUE(history.canRedo());

  history.redo();
  EXPECT_EQ(1, model);
  EXPECT_TRUE(history.canUndo());
  EXPECT_TRUE(history.canRedo());
  history.redo();
  EXPECT_EQ(2, model);
  EXPECT_TRUE(history.canUndo());
  EXPECT_FALSE(history.canRedo());
}

TEST(Undo, Tree)
{
  UndoHistory history;
  int model = 0;

  // 1 --- 2
  //  \
  //   ------ 3 --- 4
  Cmd cmd1([&]{ model = 1; }, [&]{ model = 0; });
  Cmd cmd2([&]{ model = 2; }, [&]{ model = 1; });
  Cmd cmd3([&]{ model = 3; }, [&]{ model = 1; });
  Cmd cmd4([&]{ model = 4; }, [&]{ model = 3; });

  cmd1.redo(); history.add(&cmd1);
  cmd2.redo(); history.add(&cmd2);
  history.undo();
  cmd3.redo(); history.add(&cmd3); // Creates a branch in the history
  cmd4.redo(); history.add(&cmd4);

  EXPECT_EQ(4, model);
  history.undo();
  EXPECT_EQ(3, model);
  history.undo();
  EXPECT_EQ(2, model);
  history.undo();
  EXPECT_EQ(1, model);
  history.undo();
  EXPECT_EQ(0,  model);
  EXPECT_FALSE(history.canUndo());
  history.redo();
  EXPECT_EQ(1, model);
  history.redo();
  EXPECT_EQ(2, model);
  history.redo();
  EXPECT_EQ(3, model);
  history.redo();
  EXPECT_EQ(4,  model);
  EXPECT_FALSE(history.canRedo());
}

TEST(Undo, ComplexTree)
{
  UndoHistory history;
  int model = 0;

  // 1 --- 2 --- 3 --- 4      ------ 7 --- 8
  //        \                /
  //         ------------- 5 --- 6
  Cmd cmd1([&]{ model = 1; }, [&]{ model = 0; });
  Cmd cmd2([&]{ model = 2; }, [&]{ model = 1; });
  Cmd cmd3([&]{ model = 3; }, [&]{ model = 2; });
  Cmd cmd4([&]{ model = 4; }, [&]{ model = 3; });
  Cmd cmd5([&]{ model = 5; }, [&]{ model = 2; });
  Cmd cmd6([&]{ model = 6; }, [&]{ model = 5; });
  Cmd cmd7([&]{ model = 7; }, [&]{ model = 5; });
  Cmd cmd8([&]{ model = 8; }, [&]{ model = 7; });

  cmd1.redo(); history.add(&cmd1);
  cmd2.redo(); history.add(&cmd2);
  cmd3.redo(); history.add(&cmd3);
  cmd4.redo(); history.add(&cmd4);
  history.undo();
  cmd5.redo(); history.add(&cmd5);
  cmd6.redo(); history.add(&cmd6);
  history.undo();
  cmd7.redo(); history.add(&cmd7);
  cmd8.redo(); history.add(&cmd8);

  EXPECT_EQ(8, model);
  history.undo();
  EXPECT_EQ(7, model);
  history.undo();
  EXPECT_EQ(6, model);
  history.undo();
  EXPECT_EQ(5, model);
  history.undo();
  EXPECT_EQ(4, model);
  history.undo();
  EXPECT_EQ(3, model);
  history.undo();
  EXPECT_EQ(2, model);
  history.undo();
  EXPECT_EQ(1, model);
  history.undo();
  EXPECT_EQ(0,  model);
  EXPECT_FALSE(history.canUndo());
  history.redo();
  EXPECT_EQ(1, model);
  history.redo();
  EXPECT_EQ(2, model);
  history.redo();
  EXPECT_EQ(3, model);
  history.redo();
  EXPECT_EQ(4,  model);
  history.redo();
  EXPECT_EQ(5,  model);
  history.redo();
  EXPECT_EQ(6,  model);
  history.redo();
  EXPECT_EQ(7,  model);
  history.redo();
  EXPECT_EQ(8,  model);
  EXPECT_FALSE(history.canRedo());
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
