// Aseprite
// Copyright (C) 2023-present  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_H_INCLUDED
#define APP_CMD_H_INCLUDED
#pragma once

#include "app/cmd_serial.h"
#include "app/cmdtype.h"
#include "base/disable_copying.h"
#include "base/ints.h"
#include "undo/undo_command.h"

#include <cstdio>
#include <string>

namespace app {

class Context;

// Objects:
// Bg : Background Layer
// Cl : Cel
// Cs : Color Space / Color Profile
// Fr : Frame
// Gr : Grid
// Im : Image
// Ly : Layer
// Pl : Palette
// Rc : Rectangle
// Rg : Region
// Se : Selection / Mask
// Sk : Slice Key
// Sl : Slice
// Sp : Sprite
// Sq : Sequence
// Tc : Transparent Color
// Tg : Tag
// Ti : Tile
// Tm : Tilemap(s)
// Tr : Tile Region
// Ts : Tileset
// Tx : Transaction
// Ud : User Data

// Actions:
// ad : add
// bi : base index
// bm : set blend mode
// bo : set bounds
// cl : clear
// cm : set color mode
// cp : assign / set / copy / replace
// da : set user data
// de : delete / deselect / remove / subtract
// di : set anidir
// du : set duration
// fp : flip
// fr : from
// fs : set flags
// im : set image
// in : intersect / crop
// mv : move
// op : set opacity
// pa : patch / union
// po : set position
// pr : set property
// ps : set properties
// pu : set tile management plugin
// px : set pixel ratio
// ra : set range
// re : remap
// rn : rename / set name
// rt : repeat
// sf : set frame
// sz : set size
// to : configure to / convert to
// ts : set tileset
// un : unlink
// zi : set z-index

#define CMDTYPE(a, b, c, d)                                                                        \
  static constexpr cmdtype_t kType = make_cmdtype(a, b, c, d);                                     \
  cmdtype_t onType() const override                                                                \
  {                                                                                                \
    return kType;                                                                                  \
  }

#define CMDTYPE2(a, b, c, d, CmdName)                                                              \
  CMDTYPE(a, b, c, d);                                                                             \
  CmdName(CmdSerial& s)                                                                            \
  {                                                                                                \
    serialize(s);                                                                                  \
  }

class Cmd : public undo::UndoCommand {
public:
  Cmd();
  virtual ~Cmd();

  cmdtype_t type() const { return onType(); }
  std::string typeStr() const;

  void execute(Context* ctx);

  // undo::UndoCommand impl
  void undo(undo::UndoContext* ctx) override;
  void redo(undo::UndoContext* ctx) override;
  void dispose() override;

  std::string label() const;
  size_t memSize() const;

  bool isSerializable() const;
  void serialize(CmdSerial& s);

  // Reads from CmdSerial the next cmdtype_t and creating and decoding
  // the specific-Cmd.
  static Cmd* Decode(CmdSerial& s);

protected:
  virtual cmdtype_t onType() const = 0;

  virtual void onExecute(Context* ctx);
  virtual void onUndo(Context* ctx);
  virtual void onRedo(Context* ctx);
  virtual void onFireNotifications(Context* ctx);

  virtual std::string onLabel() const;
  virtual size_t onMemSize() const;
  virtual bool onIsSerializable() const { return true; }
  virtual void onSerialize(CmdSerial& serial);

private:
#if _DEBUG
  enum class State { NotExecuted, Executed, Undone, Redone };
  State m_state;
#endif

  DISABLE_COPYING(Cmd);
};

} // namespace app

#endif
