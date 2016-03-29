// Aseprite Undo Library
// Copyright (C) 2015-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gtest/gtest.h>

#include "undo/undo_command.h"
#include "undo/undo_history.h"

using namespace undo;

class Cmd : public UndoCommand {
public:
  Cmd(int& model, int redo_value, int undo_value)
    : m_model(model)
    , m_redo_value(redo_value)
    , m_undo_value(undo_value) {
  }
  void redo() override { m_model = m_redo_value; }
  void undo() override { m_model = m_undo_value; }
  void dispose() override { }
private:
  int& m_model;
  int m_redo_value;
  int m_undo_value;
};

TEST(Undo, Basics)
{
  int model = 0;
  Cmd cmd1(model, 1, 0);
  Cmd cmd2(model, 2, 1);

  EXPECT_EQ(0, model);
  cmd1.redo();
  EXPECT_EQ(1, model);
  cmd2.redo();
  EXPECT_EQ(2, model);

  UndoHistory history;
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
  // 1 --- 2
  //  |
  //   ------ 3 --- 4
  int model = 0;
  Cmd cmd1(model, 1, 0);
  Cmd cmd2(model, 2, 1);
  Cmd cmd3(model, 3, 1);
  Cmd cmd4(model, 4, 3);

  UndoHistory history;
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
  // 1 --- 2 --- 3 --- 4      ------ 7 --- 8
  //        |                |
  //         ------------- 5 --- 6
  int model = 0;
  Cmd cmd1(model, 1, 0);
  Cmd cmd2(model, 2, 1);
  Cmd cmd3(model, 3, 2);
  Cmd cmd4(model, 4, 3);
  Cmd cmd5(model, 5, 2);
  Cmd cmd6(model, 6, 5);
  Cmd cmd7(model, 7, 5);
  Cmd cmd8(model, 8, 7);

  UndoHistory history;
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
