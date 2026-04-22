// Aseprite
// Copyright (C) 2023-2025  Igara Studio S.A.
// Copyright (C) 2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_KEY_CONTEXT_H_INCLUDED
#define APP_UI_KEY_CONTEXT_H_INCLUDED
#pragma once

namespace app {

// Specifies a possible App state/context where the user can press a
// key and associate different actions/commands in that specific
// context. Useful to associate the same key to different actions
// depending on the current context.
//
// This list should be sorted in such a way that more specific
// contexts are below:
//
//   Normal > SelectionTool > Transformation
//
// This means that keys defined in SelectionTool are still valid in
// Transformation context.
//
// Other key context are just temporary in the Editor, like a mouse
// drag operation when we are transforming the
// selection. E.g. TranslatingSelection, ScalingSelection, etc.
enum class KeyContext {

  // Special context to define keys. This is not used as a "current
  // key context", but just to define keys for the whole range of key
  // contexts.
  Any,

  // Regular context in a standby user state, the user is not dragging
  // or clicking the mouse.
  Normal,

  // The user has a selection on the canvas and a selection-like tool
  // (selection ink) selected.
  SelectionTool,

  // Special context to modify a specific action when the mouse is
  // above the handles to translate/scale/rotate or is already
  // translating/scaling/rotating the selection.
  TranslatingSelection,
  ScalingSelection,
  RotatingSelection,

  // Special context when the user has a specific tool selected to
  // modify the tool behavior with a key:
  MoveTool,
  FreehandTool,
  ShapeTool,

  // When the mouse wheel is used above the editor.
  MouseWheel,

  // The user has a range of frames/cels selected in the timeline.
  FramesSelection,

  // The user has moved the selection and has yet to drop the pixels
  // (the user can drop or undo the transformation).
  Transformation,
};

} // namespace app

#endif
