// Aseprite
// Copyright (C) 2018-2023  Igara Studio S.A.
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
#include "app/i18n/strings.h"
#include "app/ini_file.h"
#include "app/pref/preferences.h"
#include "app/tools/active_tool.h"
#include "app/tools/ink.h"
#include "app/tools/pick_ink.h"
#include "app/tools/tool.h"
#include "app/ui/app_menuitem.h"
#include "app/ui/doc_view.h"
#include "app/ui/editor/dragging_value_state.h"
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
#include "app/ui/editor/vec2.h"
#include "app/ui/editor/zooming_state.h"
#include "app/ui/main_window.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/status_bar.h"
#include "app/ui/timeline/timeline.h"
#include "app/ui_context.h"
#include "app/util/layer_utils.h"
#include "app/util/new_image_from_mask.h"
#include "app/util/readable_time.h"
#include "base/pi.h"
#include "base/vector2d.h"
#include "doc/grid.h"
#include "doc/layer.h"
#include "doc/mask.h"
#include "doc/slice.h"
#include "doc/sprite.h"
#include "fmt/format.h"
#include "gfx/rect.h"
#include "os/surface.h"
#include "os/system.h"
#include "ui/alert.h"
#include "ui/message.h"
#include "ui/view.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace app {

using namespace ui;

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
      [this, editor]{ onPivotChange(editor); });
  m_pivotPosConn =
    Preferences::instance().selection.pivotPosition.AfterChange.connect(
      [this, editor]{ onPivotChange(editor); });
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
        StatusBar::instance()->showTip(
          1000, Strings::statusbar_tips_cannot_move_bg_layer());
      }
      else if (!layer->isVisibleHierarchy()) {
        StatusBar::instance()->showTip(
          1000,
          fmt::format(Strings::statusbar_tips_layer_x_is_hidden(),
                      layer->name()));
      }
      else if (!layer->isMovable() || !layer->isEditableHierarchy()) {
        StatusBar::instance()->showTip(
          1000, fmt::format(Strings::statusbar_tips_layer_locked(), layer->name()));
      }
      else {
        MovingCelCollect collect(editor, layer);
        if (collect.empty()) {
          StatusBar::instance()->showTip(
            1000, Strings::statusbar_tips_nothing_to_move());
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
              1000, Strings::statusbar_tips_recovery_task_using_sprite());
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
          // If we click outside all slices, we clear the selection of slices.
          if (!hit.slice() || !site.selectedSlices().contains(hit.slice()->id())) {
            editor->clearSlicesSelection();
            editor->selectSlice(hit.slice());

            site = Site();
            editor->getSite(&site);
          }

          MovingSliceState* newState = new MovingSliceState(
            editor, msg, hit, site.selectedSlices());
          editor->setState(EditorStatePtr(newState));
        }
        else {
          Menu* popupMenu = AppMenus::instance()->getSlicePopupMenu();
          if (popupMenu) {
            Params params;
            // When the editor doesn't have a set of selected slices,
            // we set the specific clicked slice for the commands (in
            // other case, those commands will get the selected set of
            // slices from Site::selectedSlices() field).
            if (!editor->hasSelectedSlices())
              params.set("id", base::convert_to<std::string>(hit.slice()->id()).c_str());
            AppMenuItem::setContextParams(params);
            popupMenu->showPopup(msg->position(), editor->display());
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
      // Change to MovingPixelsState
      transformSelection(editor, msg, MovePixelsHandle);
      return true;
    }
  }

  // Start the Tool-Loop
  if (layer && (layer->isImage() || clickedInk->isSelection())) {
    tools::Pointer pointer = pointer_from_msg(editor, msg);

    // Shift+click on Pencil tool starts a line onMouseDown() when the
    // preview (onKeyDown) is disabled.
    if (!Preferences::instance().editor.straightLinePreview() &&
        checkStartDrawingStraightLine(editor, msg, &pointer)) {
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

    startDrawingState(editor, msg,
                      DrawingType::Regular,
                      pointer);

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
    // Drop pixels if we are in moving pixels state
    if (dynamic_cast<MovingPixelsState*>(editor->getState().get()))
      editor->backToPreviousState();

    // Start a tool-loop selecting tiles.
    startDrawingState(editor, msg,
                      DrawingType::SelectTiles,
                      pointer_from_msg(editor, msg));
    return true;
  }
  // Show slice properties when we double-click it
  else if (ink->isSlice()) {
    EditorHit hit = editor->calcHit(msg->position());
    if (hit.slice()) {
      Command* cmd = Commands::instance()->byId(CommandId::SliceProperties());
      Params params;
      params.set("id", base::convert_to<std::string>(hit.slice()->id()).c_str());
      UIContext::instance()->executeCommandFromMenuOrShortcut(cmd, params);
      return true;
    }
  }

  return false;
}

bool StandbyState::onSetCursor(Editor* editor, const gfx::Point& mouseScreenPos)
{
  tools::Ink* ink = editor->getCurrentEditorInk();

  // See if the cursor is in some selection handle.
  if (m_decorator->onSetCursor(ink, editor, mouseScreenPos))
    return true;

  auto theme = skin::SkinTheme::get(editor);

  if (ink) {
    // If the current tool change selection (e.g. rectangular marquee, etc.)
    if (ink->isSelection()) {
      if (overSelectionEdges(editor, mouseScreenPos)) {
        editor->showMouseCursor(
          kCustomCursor, theme->cursors.moveSelection());
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

  return StateWithWheelBehavior::onSetCursor(editor, mouseScreenPos);
}

bool StandbyState::onKeyDown(Editor* editor, KeyMessage* msg)
{
  if (Preferences::instance().editor.straightLinePreview() &&
      checkStartDrawingStraightLine(editor, nullptr, nullptr))
    return false;

  Keys keys = KeyboardShortcuts::instance()
    ->getDragActionsFromKeyMessage(KeyContext::MouseWheel, msg);
  if (editor->hasMouse() && !keys.empty()) {
    // Don't enter DraggingValueState to change brush size if we are
    // in a selection-like tool
    if (keys.size() == 1 &&
        keys[0]->wheelAction() == WheelAction::BrushSize &&
        editor->getCurrentEditorInk()->isSelection()) {
      return false;
    }

    EditorStatePtr newState(new DraggingValueState(editor, keys));
    editor->setState(newState);
    return true;
  }

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
    editor->screenToEditorF(editor->mousePosInDisplay())
    - gfx::PointF(editor->mainTilePosition());

  if (!sprite) {
    StatusBar::instance()->showDefaultText();
  }
  // For eye-dropper
  else if (ink->isEyedropper()) {
    Site site = editor->getSite();
    EyedropperCommand cmd;
    app::Color color = Preferences::instance().colorBar.fgColor();
    doc::tile_t tile = Preferences::instance().colorBar.fgTile();
    cmd.pickSample(site,
                   spritePos,
                   editor->projection(),
                   color, tile);

    std::string buf =
      fmt::format(" :pos: {} {}",
                  int(std::floor(spritePos.x)),
                  int(std::floor(spritePos.y)));

    if (site.tilemapMode() == TilemapMode::Tiles)
      StatusBar::instance()->showTile(0, tile, buf);
    else
      StatusBar::instance()->showColor(0, color, buf);
  }
  else {
    Site site = editor->getSite();
    Mask* mask =
      (editor->document()->isMaskVisible() ?
       editor->document()->mask(): NULL);

    std::string buf = fmt::format(
      ":pos: {} {}",
      int(std::floor(spritePos.x)),
      int(std::floor(spritePos.y)));

    Cel* cel = nullptr;
    if (editor->showAutoCelGuides()) {
      cel = editor->getSite().cel();
    }

    if (cel) {
      buf += fmt::format(
        " :start: {} {} :size: {} {}",
        cel->bounds().x, cel->bounds().y,
        cel->bounds().w, cel->bounds().h);
    }
    else {
      buf += fmt::format(
        " :size: {} {}",
        sprite->width(),
        sprite->height());
    }

    if (mask)
      buf += fmt::format(" :selsize: {} {}",
                         mask->bounds().w,
                         mask->bounds().h);

    if (sprite->totalFrames() > 1) {
      buf += fmt::format(
        " :frame: {} :clock: {}/{}",
        site.frame()+editor->docPref().timeline.firstFrame(),
        human_readable_time(sprite->frameDuration(site.frame())).c_str(),
        human_readable_time(sprite->totalAnimationDuration()).c_str());
    }

    if ((editor->docPref().show.grid()) ||
        (site.layer() && site.layer()->isTilemap())) {
      doc::Grid grid = site.grid();
      if (!grid.isEmpty()) {
        gfx::Point pt = grid.canvasToTile(gfx::Point(spritePos));
        buf += fmt::format(" :grid: {} {}", pt.x, pt.y);

        // Show the tile index of this specific tile
        if (site.layer() &&
            site.layer()->isTilemap() &&
            site.image()) {
          if (site.image()->bounds().contains(pt)) {
            buf += fmt::format(" [{}]", site.image()->getPixel(pt.x, pt.y));
          }
        }
      }
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
            buf += fmt::format(" :slice: ...");
            break;
          }

          buf += fmt::format(" :slice: {}", slice->name());
        }
      }
    }

    StatusBar::instance()->setStatusText(0, buf);
  }

  return true;
}

DrawingState* StandbyState::startDrawingState(
  Editor* editor,
  const ui::MouseMessage* msg,
  const DrawingType drawingType,
  const tools::Pointer& pointer)
{
  if (editor->layer()->isTilemap() &&
      editor->sprite()->hasTileManagementPlugin() &&
      !editor->layer()->cel(editor->frame())) {
    // Trigger event so the plugin can create a cel on-the-fly
    App::instance()->BeforePaintEmptyTilemap();

    if (!editor->layer() ||
        !editor->layer()->cel(editor->frame())) {
      return nullptr;
    }
    // In other case, it looks like PaintEmptyTilemap event created
    // the cel...
  }
  // We need to clear and redraw the brush boundaries after the
  // first mouse pressed/point shape if drawn. This is to avoid
  // graphical glitches (invalid areas in the ToolLoop's src/dst
  // images).
  HideBrushPreview hide(editor->brushPreview());

  tools::ToolLoop* toolLoop = create_tool_loop(
    editor,
    UIContext::instance(),
    pointer.button(),
    (drawingType == DrawingType::LineFreehand),
    (drawingType == DrawingType::SelectTiles));
  if (!toolLoop)
    return nullptr;

  EditorStatePtr newState(
    new DrawingState(editor,
                     toolLoop,
                     drawingType));
  editor->setState(newState);

  static_cast<DrawingState*>(newState.get())
    ->initToolLoop(editor, msg, pointer);

  return static_cast<DrawingState*>(newState.get());
}

bool StandbyState::checkStartDrawingStraightLine(Editor* editor,
                                                 const ui::MouseMessage* msg,
                                                 const tools::Pointer* pointer)
{
  // Start line preview with shift key
  if (canCheckStartDrawingStraightLine() &&
      editor->startStraightLineWithFreehandTool(pointer)) {
    tools::Pointer::Button pointerButton =
      (pointer ? pointer->button(): tools::Pointer::Left);

    DrawingState* drawingState =
      startDrawingState(editor, msg,
                        DrawingType::LineFreehand,
                        tools::Pointer(
                          editor->document()->lastDrawingPoint(),
                          tools::Vec2(0.0f, 0.0f),
                          pointerButton,
                          pointer ? pointer->type(): PointerType::Unknown,
                          pointer ? pointer->pressure(): 0.0f));
    if (drawingState) {
      drawingState->sendMovementToToolLoop(
        tools::Pointer(
          pointer ? pointer->point(): editor->screenToEditor(editor->mousePosInDisplay()),
          tools::Vec2(0.0f, 0.0f),
          pointerButton,
          pointer ? pointer->type(): tools::Pointer::Type::Unknown,
          pointer ? pointer->pressure(): 0.0f));
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

  if (auto movingPixels = dynamic_cast<MovingPixelsState*>(editor->getState().get())) {
    movingPixels->translate(gfx::PointF(move));
    if (std::fabs(angle) > 1e-5)
      movingPixels->rotate(angle);
  }
}

void StandbyState::startFlipTransformation(Editor* editor, doc::algorithm::FlipType flipType)
{
  transformSelection(editor, NULL, NoHandle);

  if (auto movingPixels = dynamic_cast<MovingPixelsState*>(editor->getState().get()))
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
      1000,
      fmt::format(Strings::statusbar_tips_non_transformable_reference_layer(),
                        layer->name()));
    return;
  }

  if (layer_is_locked(editor))
    return;

  try {
    Site site = editor->getSite();
    ImageRef tmpImage;

    if (site.layer() &&
        site.layer()->isTilemap() &&
        site.tilemapMode() == TilemapMode::Tiles) {
      tmpImage.reset(new_tilemap_from_mask(site, site.document()->mask()));
    }
    else {
      tmpImage.reset(new_image_from_mask(site,
                                         Preferences::instance().experimental.newBlend()));
    }

    ASSERT(tmpImage);
    if (!tmpImage) {
      // We've received a bug report with this case, we're not sure
      // yet how to reproduce it. Probably new_tilemap_from_mask() can
      // return nullptr (e.g. when site.cel() is nullptr?)
      return;
    }

    // Clear brush preview, as the extra cel will be replaced with the
    // transformed image.
    editor->brushPreview().hide();

    PixelsMovementPtr pixelsMovement(
      new PixelsMovement(UIContext::instance(),
                         site,
                         tmpImage.get(),
                         document->mask(),
                         "Transformation"));

    // If the Ctrl key is pressed start dragging a copy of the selection
    EditorCustomizationDelegate* customization = editor->getCustomizationDelegate();
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
    StatusBar::instance()->showTip(
      1000, Strings::statusbar_tips_sprite_locked_somewhere());
    editor->showMouseCursor(kForbiddenCursor);
  }
  catch (const std::bad_alloc&) {
    StatusBar::instance()->showTip(
      1000, Strings::statusbar_tips_not_enough_transform_memory());
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
      editor->document()->hasMaskBoundaries() &&
      // TODO improve this check, how we can know that we aren't in the MovingPixelsState
      !dynamic_cast<MovingPixelsState*>(editor->getState().get())) {
    gfx::Point mainOffset(editor->mainTilePosition());

    // For each selection edge
    for (const auto& seg : editor->document()->maskBoundaries()) {
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
    auto theme = skin::SkinTheme::get(editor);
    const Transformation transformation(m_standbyState->getTransformation(editor));
    TransformHandles* tr = getTransformHandles(editor);
    HandleType handle = tr->getHandleAtPoint(
      editor, mouseScreenPos, transformation);

    CursorType newCursorType = kArrowCursor;
    const Cursor* newCursor = nullptr;

    auto corners = transformation.transformedCorners();
    auto A = corners[Transformation::Corners::LEFT_TOP];
    auto B = corners[Transformation::Corners::RIGHT_TOP];
    auto C = corners[Transformation::Corners::LEFT_BOTTOM];
    auto D = corners[Transformation::Corners::RIGHT_BOTTOM];
    vec2 v;

    switch (handle) {
      case ScaleNWHandle: case RotateNWHandle: v = to_vec2(A - D); break;
      case ScaleNEHandle: case RotateNEHandle: v = to_vec2(B - C); break;
      case ScaleSWHandle: case RotateSWHandle: v = to_vec2(C - B); break;
      case ScaleSEHandle: case RotateSEHandle: v = to_vec2(D - A); break;
      case ScaleNHandle: v = to_vec2(A - C); break;
      case ScaleEHandle: v = to_vec2(B - A); break;
      case ScaleSHandle: v = to_vec2(C - A); break;
      case ScaleWHandle: v = to_vec2(A - B); break;
      case SkewNHandle:
        v = to_vec2(B - A);
        v = vec2(v.y, -v.x);
        break;
      case SkewEHandle:
        v = to_vec2(D - B);
        v = vec2(v.y, -v.x);
        break;
      case SkewSHandle:
        v = to_vec2(C - D);
        v = vec2(v.y, -v.x);
        break;
      case SkewWHandle:
        v = to_vec2(A - C);
        v = vec2(v.y, -v.x);
        break;
      case PivotHandle:
        break;
      default:
        // The cursor will be set by Editor::onSetCursor()
        return false;
    }

    double angle = v.angle();
    angle = base::fmod_radians(angle) + PI;
    ASSERT(angle >= 0.0 && angle <= 2*PI);
    const int angleInt = std::clamp<int>(std::floor(8.0 * angle / (2.0*PI) + 0.5), 0, 8) % 8;

    if (handle == PivotHandle) {
      newCursorType = kHandCursor;
    }
    else if (handle >= ScaleNWHandle &&
             handle <= ScaleSEHandle) {
      const CursorType rotated_size_cursors[8] = {
        kSizeWCursor, kSizeNWCursor, kSizeNCursor, kSizeNECursor,
        kSizeECursor, kSizeSECursor, kSizeSCursor, kSizeSWCursor
      };
      newCursorType = rotated_size_cursors[angleInt];
    }
    else if (handle >= RotateNWHandle &&
             handle <= RotateSEHandle) {
      const Cursor* rotated_rotate_cursors[8] = {
        theme->cursors.rotateW(), theme->cursors.rotateNw(), theme->cursors.rotateN(), theme->cursors.rotateNe(),
        theme->cursors.rotateE(), theme->cursors.rotateSe(), theme->cursors.rotateS(), theme->cursors.rotateSw()
      };
      newCursor = rotated_rotate_cursors[angleInt];
      newCursorType = kCustomCursor;
    }
    else if (handle >= SkewNHandle &&
             handle <= SkewSHandle) {
      const Cursor* rotated_skew_cursors[8] = {
        theme->cursors.skewW(), theme->cursors.skewNw(), theme->cursors.skewN(), theme->cursors.skewNe(),
        theme->cursors.skewE(), theme->cursors.skewSe(), theme->cursors.skewS(), theme->cursors.skewSw()
      };
      newCursor = rotated_skew_cursors[angleInt];
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
      getTransformHandles(editor)
        ->drawHandles(editor, render->getGraphics(),
                      m_standbyState->getTransformation(editor));

      m_standbyState->m_transformSelectionHandlesAreVisible = true;
    }
  }

  // Draw transformation handles (if the mask is visible and isn't frozen).
  Handles handles;
  if (StandbyState::Decorator::getSymmetryHandles(editor, handles)) {
    auto theme = skin::SkinTheme::get(editor);
    os::Surface* part = theme->parts.transformationHandle()->bitmap(0);
    ScreenGraphics g(editor->display());
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
      auto theme = skin::SkinTheme::get(editor);
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
