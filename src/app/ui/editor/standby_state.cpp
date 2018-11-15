// Aseprite
// Copyright (C) 2018  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/editor/standby_state.h"

#include "app/app.h"
#include "app/app_menus.h"
#include "app/color_picker.h"
#include "app/commands/cmd_eyedropper.h"
#include "app/commands/commands.h"
#include "app/commands/params.h"
#include "app/doc_range.h"
#include "app/ini_file.h"
#include "app/pref/preferences.h"
#include "app/tools/active_tool.h"
#include "app/tools/ink.h"
#include "app/tools/pick_ink.h"
#include "app/tools/tool.h"
#include "app/ui/app_menuitem.h"
#include "app/ui/doc_view.h"
#include "app/ui/editor/drawing_state.h"
#include "app/ui/editor/editor.h"
#include "app/ui/editor/editor_customization_delegate.h"
#include "app/ui/editor/glue.h"
#include "app/ui/editor/handle_type.h"
#include "app/ui/editor/moving_cel_state.h"
#include "app/ui/editor/moving_pixels_state.h"
#include "app/ui/editor/moving_selection_state.h"
#include "app/ui/editor/moving_slice_state.h"
#include "app/ui/editor/moving_symmetry_state.h"
#include "app/ui/editor/pivot_helpers.h"
#include "app/ui/editor/pixels_movement.h"
#include "app/ui/editor/scrolling_state.h"
#include "app/ui/editor/tool_loop_impl.h"
#include "app/ui/editor/transform_handles.h"
#include "app/ui/editor/zooming_state.h"
#include "app/ui/main_window.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/status_bar.h"
#include "app/ui/timeline/timeline.h"
#include "app/ui_context.h"
#include "app/util/new_image_from_mask.h"
#include "app/util/readable_time.h"
#include "base/bind.h"
#include "base/pi.h"
#include "doc/layer.h"
#include "doc/mask.h"
#include "doc/slice.h"
#include "doc/sprite.h"
#include "fixmath/fixmath.h"
#include "gfx/rect.h"
#include "os/surface.h"
#include "os/system.h"
#include "ui/alert.h"
#include "ui/message.h"
#include "ui/system.h"
#include "ui/view.h"

#include <cmath>
#include <cstring>

namespace app {

using namespace ui;

static CursorType rotated_size_cursors[8] = {
  kSizeECursor,
  kSizeNECursor,
  kSizeNCursor,
  kSizeNWCursor,
  kSizeWCursor,
  kSizeSWCursor,
  kSizeSCursor,
  kSizeSECursor
};

#ifdef _MSC_VER
#pragma warning(disable:4355) // warning C4355: 'this' : used in base member initializer list
#endif

StandbyState::StandbyState()
  : m_decorator(new Decorator(this))
  , m_transformSelectionHandlesAreVisible(false)
{
}

StandbyState::~StandbyState()
{
  delete m_decorator;
}

void StandbyState::onEnterState(Editor* editor)
{
  StateWithWheelBehavior::onEnterState(editor);

  editor->setDecorator(m_decorator);

  m_pivotVisConn =
    Preferences::instance().selection.pivotVisibility.AfterChange.connect(
      base::Bind<void>(&StandbyState::onPivotChange, this, editor));
  m_pivotPosConn =
    Preferences::instance().selection.pivotPosition.AfterChange.connect(
      base::Bind<void>(&StandbyState::onPivotChange, this, editor));
}

void StandbyState::onActiveToolChange(Editor* editor, tools::Tool* tool)
{
  // If the user change from a selection tool to a non-selection tool,
  // or viceversa, we've to show or hide the transformation handles.
  bool needDecorators = (tool && tool->getInk(0)->isSelection());
  if (m_transformSelectionHandlesAreVisible != needDecorators ||
      !editor->layer() ||
      !editor->layer()->isReference()) {
    m_transformSelectionHandlesAreVisible = false;
    editor->invalidate();
  }
}

bool StandbyState::onMouseDown(Editor* editor, MouseMessage* msg)
{
  if (editor->hasCapture())
    return true;

  UIContext* context = UIContext::instance();
  tools::Ink* clickedInk = editor->getCurrentEditorInk();
  Site site;
  editor->getSite(&site);
  Doc* document = site.document();
  Layer* layer = site.layer();

  // When an editor is clicked the current view is changed.
  context->setActiveView(editor->getDocView());

  // Move symmetry
  Decorator::Handles handles;
  if (m_decorator->getSymmetryHandles(editor, handles)) {
    for (const auto& handle : handles) {
      if (handle.bounds.contains(msg->position())) {
        auto mode = (handle.align & (TOP | BOTTOM) ? app::gen::SymmetryMode::HORIZONTAL:
                                                     app::gen::SymmetryMode::VERTICAL);
        bool horz = (mode == app::gen::SymmetryMode::HORIZONTAL);
        auto& symmetry = Preferences::instance().document(editor->document()).symmetry;
        auto& axis = (horz ? symmetry.xAxis:
                             symmetry.yAxis);
        editor->setState(
          EditorStatePtr(new MovingSymmetryState(editor, msg, mode, axis)));
        return true;
      }
    }
  }

  // Start scroll loop
  if (editor->checkForScroll(msg) ||
      editor->checkForZoom(msg))
    return true;

  // Move cel X,Y coordinates
  if (clickedInk->isCelMovement()) {
    // Handle "Auto Select Layer"
    if (editor->isAutoSelectLayer()) {
      gfx::PointF cursor = editor->screenToEditorF(msg->position());
      ColorPicker picker;
      picker.pickColor(site, cursor,
                       editor->projection(),
                       ColorPicker::FromComposition);

      auto range = App::instance()->timeline()->range();
      if (picker.layer() &&
          !range.contains(picker.layer())) {
        layer = picker.layer();
        if (layer) {
          editor->setLayer(layer);
          editor->flashCurrentLayer();
        }
      }
    }

    if (layer) {
      // TODO we should be able to move the `Background' with tiled mode
      if (layer->isBackground()) {
        StatusBar::instance()->showTip(1000,
          "The background layer cannot be moved");
      }
      else if (!layer->isVisibleHierarchy()) {
        StatusBar::instance()->showTip(1000,
          "Layer '%s' is hidden", layer->name().c_str());
      }
      else if (!layer->isMovable() || !layer->isEditableHierarchy()) {
        StatusBar::instance()->showTip(1000,
          "Layer '%s' is locked", layer->name().c_str());
      }
      else {
        MovingCelCollect collect(editor, layer);
        if (collect.empty()) {
          StatusBar::instance()->showTip(
            1000, "Nothing to move");
        }
        else {
          try {
            // Change to MovingCelState
            HandleType handle = MovePixelsHandle;
            if (resizeCelBounds(editor).contains(msg->position()))
              handle = ScaleSEHandle;

            MovingCelState* newState = new MovingCelState(
              editor, msg, handle, collect);
            editor->setState(EditorStatePtr(newState));
          }
          catch (const LockedDocException&) {
            // TODO break the background task that is locking this sprite
            StatusBar::instance()->showTip(
              1000, "Sprite is used by a backup/data recovery task");
          }
        }
      }
    }

    return true;
  }

  // Call the eyedropper command
  if (clickedInk->isEyedropper()) {
    editor->captureMouse();
    callEyedropper(editor, msg);
    return true;
  }

  if (clickedInk->isSlice()) {
    EditorHit hit = editor->calcHit(msg->position());
    switch (hit.type()) {
      case EditorHit::SliceBounds:
      case EditorHit::SliceCenter:
        if (msg->left()) {
          MovingSliceState* newState = new MovingSliceState(editor, msg, hit);
          editor->setState(EditorStatePtr(newState));
        }
        else {
          Menu* popupMenu = AppMenus::instance()->getSlicePopupMenu();
          if (popupMenu) {
            Params params;
            params.set("id", base::convert_to<std::string>(hit.slice()->id()).c_str());
            AppMenuItem::setContextParams(params);
            popupMenu->showPopup(msg->position());
            AppMenuItem::setContextParams(Params());
          }
        }
        return true;
    }
  }

  // Only if the selected tool or quick tool is selection, we give the
  // possibility to transform/move the selection. In other case,
  // e.g. when selection is used with right-click mode, the
  // transformation is disabled.
  auto activeToolManager = App::instance()->activeToolManager();
  if (clickedInk->isSelection() &&
      ((activeToolManager->selectedTool() &&
        activeToolManager->selectedTool()->getInk(0)->isSelection()) ||
       (activeToolManager->quickTool() &&
        activeToolManager->quickTool()->getInk(0)->isSelection()))) {
    // Transform selected pixels
    if (editor->isActive() &&
        document->isMaskVisible() &&
        m_decorator->getTransformHandles(editor) &&
        (!Preferences::instance().selection.modifiersDisableHandles() ||
         msg->modifiers() == kKeyNoneModifier)) {
      TransformHandles* transfHandles = m_decorator->getTransformHandles(editor);

      // Get the handle covered by the mouse.
      HandleType handle = transfHandles->getHandleAtPoint(editor,
        msg->position(),
        getTransformation(editor));

      if (handle != NoHandle) {
        int x, y, opacity;
        Image* image = site.image(&x, &y, &opacity);
        if (layer && image) {
          if (!layer->isEditableHierarchy()) {
            StatusBar::instance()->showTip(1000,
              "Layer '%s' is locked", layer->name().c_str());
            return true;
          }

          // Change to MovingPixelsState
          transformSelection(editor, msg, handle);
        }
        return true;
      }
    }

    // Move selection edges
    if (overSelectionEdges(editor, msg->position())) {
      transformSelection(editor, msg, MoveSelectionHandle);
      return true;
    }

    // Move selected pixels
    if (layer && editor->canStartMovingSelectionPixels() && msg->left()) {
      if (!layer->isEditableHierarchy()) {
        StatusBar::instance()->showTip(1000,
          "Layer '%s' is locked", layer->name().c_str());
        return true;
      }

      // Change to MovingPixelsState
      transformSelection(editor, msg, MovePixelsHandle);
      return true;
    }
  }

  // Start the Tool-Loop
  if (layer && (layer->isImage() || clickedInk->isSelection())) {
    // Shift+click on Pencil tool starts a line onMouseDown() when the
    // preview (onKeyDown) is disabled.
    if (!Preferences::instance().editor.straightLinePreview() &&
        checkStartDrawingStraightLine(editor, msg)) {
      // Send first mouse down to draw the straight line and start the
      // freehand mode.
      editor->getState()->onMouseDown(editor, msg);
      return true;
    }

    // Disable layer edges to avoid showing the modified cel
    // information by ExpandCelCanvas (i.e. the cel origin is changed
    // to 0,0 coordinate.)
    auto& layerEdgesOption = editor->docPref().show.layerEdges;
    bool layerEdges = layerEdgesOption();
    if (layerEdges)
      layerEdgesOption(false);

    startDrawingState(editor,
                      DrawingType::Regular,
                      pointer_from_msg(editor, msg));

    // Restore layer edges
    if (layerEdges)
      layerEdgesOption(true);
    return true;
  }

  return true;
}

bool StandbyState::onMouseUp(Editor* editor, MouseMessage* msg)
{
  editor->releaseMouse();
  return true;
}

bool StandbyState::onMouseMove(Editor* editor, MouseMessage* msg)
{
  // We control eyedropper tool from here. TODO move this to another place
  if (msg->left() || msg->right()) {
    tools::Ink* clickedInk = editor->getCurrentEditorInk();
    if (clickedInk->isEyedropper() &&
        editor->hasCapture()) {
      callEyedropper(editor, msg);
    }
  }

  editor->updateStatusBar();
  return true;
}

bool StandbyState::onDoubleClick(Editor* editor, MouseMessage* msg)
{
  if (editor->hasCapture())
    return false;

  tools::Ink* ink = editor->getCurrentEditorInk();

  // Select a tile with double-click
  if (ink->isSelection() &&
      Preferences::instance().selection.doubleclickSelectTile()) {
    Command* selectTileCmd =
      Commands::instance()->byId(CommandId::SelectTile());

    Params params;
    if (int(editor->getToolLoopModifiers()) & int(tools::ToolLoopModifiers::kAddSelection))
      params.set("mode", "add");
    else if (int(editor->getToolLoopModifiers()) & int(tools::ToolLoopModifiers::kSubtractSelection))
      params.set("mode", "subtract");
    else if (int(editor->getToolLoopModifiers()) & int(tools::ToolLoopModifiers::kIntersectSelection))
      params.set("mode", "intersect");

    UIContext::instance()->executeCommand(selectTileCmd, params);
    return true;
  }

  return false;
}

bool StandbyState::onSetCursor(Editor* editor, const gfx::Point& mouseScreenPos)
{
  tools::Ink* ink = editor->getCurrentEditorInk();

  // See if the cursor is in some selection handle.
  if (m_decorator->onSetCursor(ink, editor, mouseScreenPos))
    return true;

  if (ink) {
    // If the current tool change selection (e.g. rectangular marquee, etc.)
    if (ink->isSelection()) {
      if (overSelectionEdges(editor, mouseScreenPos)) {
        editor->showMouseCursor(
          kCustomCursor, skin::SkinTheme::instance()->cursors.moveSelection());
        return true;
      }

      // Move pixels
      if (editor->canStartMovingSelectionPixels()) {
        EditorCustomizationDelegate* customization = editor->getCustomizationDelegate();
        if ((customization) &&
            int(customization->getPressedKeyAction(KeyContext::TranslatingSelection) & KeyAction::CopySelection))
          editor->showMouseCursor(kArrowPlusCursor);
        else
          editor->showMouseCursor(kMoveCursor);
      }
      else
        editor->showBrushPreview(mouseScreenPos);
      return true;
    }
    else if (ink->isEyedropper()) {
      editor->showMouseCursor(
        kCustomCursor, skin::SkinTheme::instance()->cursors.eyedropper());
      return true;
    }
    else if (ink->isZoom()) {
      editor->showMouseCursor(
        kCustomCursor, skin::SkinTheme::instance()->cursors.magnifier());
      return true;
    }
    else if (ink->isScrollMovement()) {
      editor->showMouseCursor(kScrollCursor);
      return true;
    }
    else if (ink->isCelMovement()) {
      if (resizeCelBounds(editor).contains(mouseScreenPos))
        editor->showMouseCursor(kSizeSECursor);
      else
        editor->showMouseCursor(kMoveCursor);
      return true;
    }
    else if (ink->isSlice()) {
      EditorHit hit = editor->calcHit(mouseScreenPos);
      switch (hit.type()) {
        case EditorHit::None:
          // Do nothing, continue
          break;
        case EditorHit::SliceBounds:
        case EditorHit::SliceCenter:
          switch (hit.border()) {
            case CENTER | MIDDLE:
              editor->showMouseCursor(kMoveCursor);
              break;
            case TOP | LEFT:
              editor->showMouseCursor(kSizeNWCursor);
              break;
            case TOP:
              editor->showMouseCursor(kSizeNCursor);
              break;
            case TOP | RIGHT:
              editor->showMouseCursor(kSizeNECursor);
              break;
            case LEFT:
              editor->showMouseCursor(kSizeWCursor);
              break;
            case RIGHT:
              editor->showMouseCursor(kSizeECursor);
              break;
            case BOTTOM | LEFT:
              editor->showMouseCursor(kSizeSWCursor);
              break;
            case BOTTOM:
              editor->showMouseCursor(kSizeSCursor);
              break;
            case BOTTOM | RIGHT:
              editor->showMouseCursor(kSizeSECursor);
              break;
          }
          return true;
      }
    }
  }

  // Draw
  if (editor->canDraw()) {
    editor->showBrushPreview(mouseScreenPos);
  }
  // Forbidden
  else {
    editor->showMouseCursor(kForbiddenCursor);
  }

  return true;
}

bool StandbyState::onKeyDown(Editor* editor, KeyMessage* msg)
{
  if (Preferences::instance().editor.straightLinePreview() &&
      checkStartDrawingStraightLine(editor, nullptr))
    return false;
  return false;
}

bool StandbyState::onKeyUp(Editor* editor, KeyMessage* msg)
{
  return false;
}

bool StandbyState::onUpdateStatusBar(Editor* editor)
{
  tools::Ink* ink = editor->getCurrentEditorInk();
  const Sprite* sprite = editor->sprite();
  gfx::PointF spritePos =
    editor->screenToEditorF(ui::get_mouse_position())
    - gfx::PointF(editor->mainTilePosition());

  if (!sprite) {
    StatusBar::instance()->showDefaultText();
  }
  // For eye-dropper
  else if (ink->isEyedropper()) {
    EyedropperCommand cmd;
    app::Color color = Preferences::instance().colorBar.fgColor();
    cmd.pickSample(editor->getSite(),
                   spritePos,
                   editor->projection(),
                   color);

    char buf[256];
    sprintf(buf, " :pos: %d %d",
            int(std::floor(spritePos.x)),
            int(std::floor(spritePos.y)));

    StatusBar::instance()->showColor(0, buf, color);
  }
  else {
    Mask* mask =
      (editor->document()->isMaskVisible() ?
       editor->document()->mask(): NULL);

    char buf[1024];
    sprintf(
            buf, ":pos: %d %d :size: %d %d",
            int(std::floor(spritePos.x)),
            int(std::floor(spritePos.y)),
            sprite->width(),
            sprite->height());

    if (mask)
      sprintf(buf+std::strlen(buf), " :selsize: %d %d",
              mask->bounds().w,
              mask->bounds().h);

    if (sprite->totalFrames() > 1) {
      sprintf(
        buf+std::strlen(buf), " :frame: %d :clock: %s/%s",
        editor->frame()+editor->docPref().timeline.firstFrame(),
        human_readable_time(sprite->frameDuration(editor->frame())).c_str(),
        human_readable_time(sprite->totalAnimationDuration()).c_str());
    }

    if (editor->docPref().show.grid()) {
      auto gb = editor->docPref().grid.bounds();
      int col = int((std::floor(spritePos.x) - (gb.x % gb.w)) / gb.w);
      int row = int((std::floor(spritePos.y) - (gb.y % gb.h)) / gb.h);
      sprintf(
        buf+std::strlen(buf), " :grid: %d %d", col, row);
    }

    if (editor->docPref().show.slices()) {
      int count = 0;
      for (auto slice : editor->document()->sprite()->slices()) {
        auto key = slice->getByFrame(editor->frame());
        if (key &&
            key->bounds().contains(
              int(std::floor(spritePos.x)),
              int(std::floor(spritePos.y)))) {
          if (++count == 3) {
            sprintf(
              buf+std::strlen(buf), " :slice: ...");
            break;
          }

          sprintf(
            buf+std::strlen(buf), " :slice: %s",
            slice->name().c_str());
        }
      }
    }

    StatusBar::instance()->setStatusText(0, buf);
  }

  return true;
}

DrawingState* StandbyState::startDrawingState(Editor* editor,
                                              const DrawingType drawingType,
                                              const tools::Pointer& pointer)
{
  // We need to clear and redraw the brush boundaries after the
  // first mouse pressed/point shape if drawn. This is to avoid
  // graphical glitches (invalid areas in the ToolLoop's src/dst
  // images).
  HideBrushPreview hide(editor->brushPreview());

  tools::ToolLoop* toolLoop = create_tool_loop(
    editor,
    UIContext::instance(),
    pointer.button(),
    (drawingType == DrawingType::LineFreehand));
  if (!toolLoop)
    return nullptr;

  EditorStatePtr newState(
    new DrawingState(editor,
                     toolLoop,
                     drawingType));
  editor->setState(newState);

  static_cast<DrawingState*>(newState.get())
    ->initToolLoop(editor,
                   pointer);

  return static_cast<DrawingState*>(newState.get());
}

bool StandbyState::checkStartDrawingStraightLine(Editor* editor,
                                                 const ui::MouseMessage* msg)
{
  // Start line preview with shift key
  if (editor->startStraightLineWithFreehandTool(msg)) {
    tools::Pointer::Button pointerButton =
      (msg ? button_from_msg(msg): tools::Pointer::Left);

    DrawingState* drawingState =
      startDrawingState(editor,
                        DrawingType::LineFreehand,
                        tools::Pointer(
                          editor->document()->lastDrawingPoint(),
                          pointerButton));
    if (drawingState) {
      drawingState->sendMovementToToolLoop(
        tools::Pointer(
          editor->screenToEditor(msg ? msg->position():
                                       ui::get_mouse_position()),
          pointerButton));
      return true;
    }
  }
  return false;
}

Transformation StandbyState::getTransformation(Editor* editor)
{
  Transformation t = editor->document()->getTransformation();
  set_pivot_from_preferences(t);
  return t;
}

void StandbyState::startSelectionTransformation(Editor* editor,
                                                const gfx::Point& move,
                                                double angle)
{
  transformSelection(editor, NULL, NoHandle);

  if (MovingPixelsState* movingPixels = dynamic_cast<MovingPixelsState*>(editor->getState().get())) {
    movingPixels->translate(move);
    if (std::fabs(angle) > 1e-5)
      movingPixels->rotate(angle);
  }
}

void StandbyState::startFlipTransformation(Editor* editor, doc::algorithm::FlipType flipType)
{
  transformSelection(editor, NULL, NoHandle);

  if (MovingPixelsState* movingPixels = dynamic_cast<MovingPixelsState*>(editor->getState().get()))
    movingPixels->flip(flipType);
}

void StandbyState::transformSelection(Editor* editor, MouseMessage* msg, HandleType handle)
{
  Doc* document = editor->document();
  for (auto docView : UIContext::instance()->getAllDocViews(document)) {
    if (docView->editor()->isMovingPixels()) {
      // TODO Transfer moving pixels state to this editor
      docView->editor()->dropMovingPixels();
    }
  }

  // Special case: Move only selection edges
  if (handle == MoveSelectionHandle) {
    EditorStatePtr newState(new MovingSelectionState(editor, msg));
    editor->setState(newState);
    return;
  }

  Layer* layer = editor->layer();
  if (layer && layer->isReference()) {
    StatusBar::instance()->showTip(
      1000, "Layer '%s' is reference, cannot be transformed",
      layer->name().c_str());
    return;
  }

  try {
    // Clear brush preview, as the extra cel will be replaced with the
    // transformed image.
    editor->brushPreview().hide();

    EditorCustomizationDelegate* customization = editor->getCustomizationDelegate();
    std::unique_ptr<Image> tmpImage(new_image_from_mask(editor->getSite()));

    PixelsMovementPtr pixelsMovement(
      new PixelsMovement(UIContext::instance(),
                         editor->getSite(),
                         tmpImage.get(),
                         document->mask(),
                         "Transformation"));

    // If the Ctrl key is pressed start dragging a copy of the selection
    if ((customization) &&
        int(customization->getPressedKeyAction(KeyContext::TranslatingSelection) & KeyAction::CopySelection))
      pixelsMovement->copyMask();
    else
      pixelsMovement->cutMask();

    editor->setState(EditorStatePtr(new MovingPixelsState(editor, msg, pixelsMovement, handle)));
  }
  catch (const LockedDocException&) {
    // Other editor is locking the document.

    // TODO steal the PixelsMovement of the other editor and use it for this one.
    StatusBar::instance()->showTip(1000, "The sprite is locked in other editor");
    editor->showMouseCursor(kForbiddenCursor);
  }
  catch (const std::bad_alloc&) {
    StatusBar::instance()->showTip(1000, "Not enough memory to transform the selection");
    editor->showMouseCursor(kForbiddenCursor);
  }
}

void StandbyState::callEyedropper(Editor* editor, const ui::MouseMessage* msg)
{
  tools::Ink* clickedInk = editor->getCurrentEditorInk();
  if (!clickedInk->isEyedropper())
    return;

  EyedropperCommand* eyedropper =
    (EyedropperCommand*)Commands::instance()->byId(CommandId::Eyedropper());
  bool fg = (static_cast<tools::PickInk*>(clickedInk)->target() == tools::PickInk::Fg);

  eyedropper->executeOnMousePos(UIContext::instance(), editor,
                                msg->position(), fg);
}

void StandbyState::onPivotChange(Editor* editor)
{
  if (editor->isActive() &&
      editor->editorFlags() & Editor::kShowMask &&
      editor->document()->isMaskVisible() &&
      !editor->document()->mask()->isFrozen()) {
    editor->invalidate();
  }
}

gfx::Rect StandbyState::resizeCelBounds(Editor* editor) const
{
  gfx::Rect bounds;
  Cel* refCel = (editor->layer() &&
                 editor->layer()->isReference() ?
                 editor->layer()->cel(editor->frame()): nullptr);
  if (refCel) {
    bounds = editor->editorToScreen(refCel->boundsF());
    bounds.w /= 4;
    bounds.h /= 4;
    bounds.x += 3*bounds.w;
    bounds.y += 3*bounds.h;
  }
  return bounds;
}

bool StandbyState::overSelectionEdges(Editor* editor,
                                      const gfx::Point& mouseScreenPos) const
{
  // Move selection edges
  if (Preferences::instance().selection.moveEdges() &&
      editor->isActive() &&
      editor->document()->isMaskVisible() &&
      editor->document()->getMaskBoundaries() &&
      // TODO improve this check, how we can know that we aren't in the MovingPixelsState
      !dynamic_cast<MovingPixelsState*>(editor->getState().get())) {
    gfx::Point mainOffset(editor->mainTilePosition());

    // For each selection edge
    for (const auto& seg : *editor->document()->getMaskBoundaries()) {
      gfx::Rect segBounds = seg.bounds();
      segBounds.offset(mainOffset);
      segBounds = editor->editorToScreen(segBounds);
      if (seg.vertical())
        segBounds.w = 1;
      else
        segBounds.h = 1;

      if (gfx::Rect(segBounds).enlarge(2*guiscale()).contains(mouseScreenPos) &&
          !gfx::Rect(segBounds).shrink(2*guiscale()).contains(mouseScreenPos)) {
        return true;
      }
    }
  }
  return false;
}

//////////////////////////////////////////////////////////////////////
// Decorator

StandbyState::Decorator::Decorator(StandbyState* standbyState)
  : m_transfHandles(NULL)
  , m_standbyState(standbyState)
{
}

StandbyState::Decorator::~Decorator()
{
  delete m_transfHandles;
}

TransformHandles* StandbyState::Decorator::getTransformHandles(Editor* editor)
{
  if (!m_transfHandles)
    m_transfHandles = new TransformHandles();

  return m_transfHandles;
}

bool StandbyState::Decorator::onSetCursor(tools::Ink* ink, Editor* editor, const gfx::Point& mouseScreenPos)
{
  if (!editor->isActive())
    return false;

  if (ink &&
      ink->isSelection() &&
      editor->document()->isMaskVisible() &&
      (!Preferences::instance().selection.modifiersDisableHandles() ||
       os::instance()->keyModifiers() == kKeyNoneModifier)) {
    auto theme = skin::SkinTheme::instance();
    const Transformation transformation(m_standbyState->getTransformation(editor));
    TransformHandles* tr = getTransformHandles(editor);
    HandleType handle = tr->getHandleAtPoint(
      editor, mouseScreenPos, transformation);

    CursorType newCursorType = kArrowCursor;
    const Cursor* newCursor = nullptr;

    switch (handle) {
      case ScaleNWHandle:         newCursorType = kSizeNWCursor; break;
      case ScaleNHandle:          newCursorType = kSizeNCursor; break;
      case ScaleNEHandle:         newCursorType = kSizeNECursor; break;
      case ScaleWHandle:          newCursorType = kSizeWCursor; break;
      case ScaleEHandle:          newCursorType = kSizeECursor; break;
      case ScaleSWHandle:         newCursorType = kSizeSWCursor; break;
      case ScaleSHandle:          newCursorType = kSizeSCursor; break;
      case ScaleSEHandle:         newCursorType = kSizeSECursor; break;
      case RotateNWHandle:        newCursor = theme->cursors.rotateNw(); break;
      case RotateNHandle:         newCursor = theme->cursors.rotateN(); break;
      case RotateNEHandle:        newCursor = theme->cursors.rotateNe(); break;
      case RotateWHandle:         newCursor = theme->cursors.rotateW(); break;
      case RotateEHandle:         newCursor = theme->cursors.rotateE(); break;
      case RotateSWHandle:        newCursor = theme->cursors.rotateSw(); break;
      case RotateSHandle:         newCursor = theme->cursors.rotateS(); break;
      case RotateSEHandle:        newCursor = theme->cursors.rotateSe(); break;
      case PivotHandle:           newCursorType = kHandCursor; break;
      default:
        return false;
    }

    // Adjust the cursor depending the current transformation angle.
    fixmath::fixed angle = fixmath::ftofix(128.0 * transformation.angle() / PI);
    angle = fixmath::fixadd(angle, fixmath::itofix(16));
    angle &= (255<<16);
    angle >>= 16;
    angle /= 32;

    if (newCursorType >= kSizeNCursor &&
        newCursorType <= kSizeNWCursor) {
      size_t num = sizeof(rotated_size_cursors) / sizeof(rotated_size_cursors[0]);
      size_t c;
      for (c=num-1; c>0; --c)
        if (rotated_size_cursors[c] == newCursorType)
          break;

      newCursorType = rotated_size_cursors[(c+angle) % num];
    }
    else if (newCursor) {
      auto theme = skin::SkinTheme::instance();
      const Cursor* rotated_rotate_cursors[8] = {
        theme->cursors.rotateE(),
        theme->cursors.rotateNe(),
        theme->cursors.rotateN(),
        theme->cursors.rotateNw(),
        theme->cursors.rotateW(),
        theme->cursors.rotateSw(),
        theme->cursors.rotateS(),
        theme->cursors.rotateSe()
      };
      size_t num = sizeof(rotated_rotate_cursors) / sizeof(rotated_rotate_cursors[0]);
      size_t c;
      for (c=num-1; c>0; --c)
        if (rotated_rotate_cursors[c] == newCursor)
          break;

      newCursor = rotated_rotate_cursors[(c+angle) % num];
      newCursorType = kCustomCursor;
    }

    editor->showMouseCursor(newCursorType, newCursor);
    return true;
  }

  // Move symmetry
  Handles handles;
  if (getSymmetryHandles(editor, handles)) {
    for (const auto& handle : handles) {
      if (handle.bounds.contains(mouseScreenPos)) {
        switch (handle.align) {
          case TOP:
          case BOTTOM:
            editor->showMouseCursor(kSizeWECursor);
            break;
          case LEFT:
          case RIGHT:
            editor->showMouseCursor(kSizeNSCursor);
            break;
        }
        return true;
      }
    }
  }

  return false;
}

void StandbyState::Decorator::postRenderDecorator(EditorPostRender* render)
{
  Editor* editor = render->getEditor();

  // Draw transformation handles (if the mask is visible and isn't frozen).
  if (editor->isActive() &&
      editor->editorFlags() & Editor::kShowMask &&
      editor->document()->isMaskVisible() &&
      !editor->document()->mask()->isFrozen()) {
    // And draw only when the user has a selection tool as active tool.
    tools::Ink* ink = editor->getCurrentEditorInk();

    if (ink->isSelection()) {
      getTransformHandles(editor)->drawHandles(editor,
        m_standbyState->getTransformation(editor));

      m_standbyState->m_transformSelectionHandlesAreVisible = true;
    }
  }

  // Draw transformation handles (if the mask is visible and isn't frozen).
  Handles handles;
  if (StandbyState::Decorator::getSymmetryHandles(editor, handles)) {
    skin::SkinTheme* theme = static_cast<skin::SkinTheme*>(ui::get_theme());
    os::Surface* part = theme->parts.transformationHandle()->bitmap(0);
    ScreenGraphics g;
    for (const auto& handle : handles)
      g.drawRgbaSurface(part, handle.bounds.x, handle.bounds.y);
  }
}

void StandbyState::Decorator::getInvalidDecoratoredRegion(Editor* editor, gfx::Region& region)
{
  Handles handles;
  if (getSymmetryHandles(editor, handles)) {
    for (const auto& handle : handles)
      region.createUnion(region, gfx::Region(handle.bounds));
  }
}

bool StandbyState::Decorator::getSymmetryHandles(Editor* editor, Handles& handles)
{
  // Draw transformation handles (if the mask is visible and isn't frozen).
  if (editor->isActive() &&
      editor->editorFlags() & Editor::kShowSymmetryLine &&
      Preferences::instance().symmetryMode.enabled()) {
    const auto& symmetry = Preferences::instance().document(editor->document()).symmetry;
    auto mode = symmetry.mode();
    if (mode != app::gen::SymmetryMode::NONE) {
      gfx::Rect mainTileBounds(editor->mainTilePosition(),
                               editor->sprite()->bounds().size());
      gfx::Rect canvasBounds(gfx::Point(0, 0),
                             editor->canvasSize());
      gfx::RectF editorViewport(View::getView(editor)->viewportBounds());
      skin::SkinTheme* theme = static_cast<skin::SkinTheme*>(ui::get_theme());
      os::Surface* part = theme->parts.transformationHandle()->bitmap(0);

      if (int(mode) & int(app::gen::SymmetryMode::HORIZONTAL)) {
        double pos = symmetry.xAxis();
        gfx::PointF pt1, pt2;

        pt1 = gfx::PointF(mainTileBounds.x+pos, canvasBounds.y);
        pt1 = editor->editorToScreenF(pt1);
        pt2 = gfx::PointF(mainTileBounds.x+pos, canvasBounds.y2());
        pt2 = editor->editorToScreenF(pt2);
        pt1.y = std::max(pt1.y-part->height(), editorViewport.y);
        pt2.y = std::min(pt2.y, editorViewport.point2().y-part->height());
        pt1.x -= part->width()/2;
        pt2.x -= part->width()/2;

        handles.push_back(
          Handle(TOP,
                 gfx::Rect(int(pt1.x), int(pt1.y), part->width(), part->height())));
        handles.push_back(
          Handle(BOTTOM,
                 gfx::Rect(int(pt2.x), int(pt2.y), part->width(), part->height())));
      }

      if (int(mode) & int(app::gen::SymmetryMode::VERTICAL)) {
        double pos = symmetry.yAxis();
        gfx::PointF pt1, pt2;

        pt1 = gfx::PointF(canvasBounds.x, mainTileBounds.y+pos);
        pt1 = editor->editorToScreenF(pt1);
        pt2 = gfx::PointF(canvasBounds.x2(), mainTileBounds.y+pos);
        pt2 = editor->editorToScreenF(pt2);
        pt1.x = std::max(pt1.x-part->width(), editorViewport.x);
        pt2.x = std::min(pt2.x, editorViewport.point2().x-part->width());
        pt1.y -= part->height()/2;
        pt2.y -= part->height()/2;

        handles.push_back(
          Handle(LEFT,
                 gfx::Rect(int(pt1.x), int(pt1.y), part->width(), part->height())));
        handles.push_back(
          Handle(RIGHT,
                 gfx::Rect(int(pt2.x), int(pt2.y), part->width(), part->height())));
      }

      return true;
    }
  }
  return false;
}

} // namespace app
