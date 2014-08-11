/* Aseprite
 * Copyright (C) 2001-2014  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/commands/cmd_export_sprite_sheet.h"

#include "app/commands/cmd_save_file.h"
#include "app/commands/command.h"
#include "app/commands/commands.h"
#include "app/commands/params.h"
#include "app/context.h"
#include "app/context_access.h"
#include "app/document.h"
#include "app/document_api.h"
#include "app/document_undo.h"
#include "app/ini_file.h"
#include "app/modules/editors.h"
#include "app/modules/gui.h"
#include "app/modules/palettes.h"
#include "app/ui/editor/editor.h"
#include "app/undo_transaction.h"
#include "base/bind.h"
#include "raster/cel.h"
#include "raster/image.h"
#include "raster/layer.h"
#include "raster/palette.h"
#include "raster/primitives.h"
#include "raster/sprite.h"
#include "raster/stock.h"
#include "ui/ui.h"

#include "generated_export_sprite_sheet.h"

#include <limits>

namespace app {

using namespace ui;

class ExportSpriteSheetWindow : public app::gen::ExportSpriteSheet {
public:
  typedef ExportSpriteSheetCommand::SpriteSheetType SpriteSheetType;
  typedef ExportSpriteSheetCommand::ExportAction ExportAction;

  ExportSpriteSheetWindow(Context* context)
  {
    sheetType()->addItem("Horizontal Strip");
    sheetType()->addItem("Vertical Strip");
    sheetType()->addItem("Matrix");

    exportAction()->addItem("Save Copy As...");
    exportAction()->addItem("Save As...");
    exportAction()->addItem("Save");
    exportAction()->addItem("Do Not Save");

    sheetType()->Change.connect(&ExportSpriteSheetWindow::onSheetTypeChange, this);
    onSheetTypeChange();
  }

  bool ok() {
    return getKiller() == exportButton();
  }

  SpriteSheetType spriteSheetTypeValue() {
    return (SpriteSheetType)sheetType()->getSelectedItemIndex();
  }

  ExportAction exportActionValue() {
    return (ExportAction)exportAction()->getSelectedItemIndex();
  }

  int columnsValue() {
    return columns()->getTextInt();
  }

protected:

  void onSheetTypeChange()
  {
    bool state = false;
    switch (sheetType()->getSelectedItemIndex()) {
      case ExportSpriteSheetCommand::Matrix:
        state = true;
        break;
    }
    columnsLabel()->setVisible(state);
    columns()->setVisible(state);

    gfx::Size reqSize = getPreferredSize();
    moveWindow(gfx::Rect(getOrigin(), reqSize));

    invalidate();
  }

};

ExportSpriteSheetCommand::ExportSpriteSheetCommand()
  : Command("ExportSpriteSheet",
            "Export Sprite Sheet",
            CmdRecordableFlag)
  , m_useUI(true)
{
}

bool ExportSpriteSheetCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable);
}

void ExportSpriteSheetCommand::onExecute(Context* context)
{
  if (m_useUI) {
    ExportSpriteSheetWindow window(context);
    window.openWindowInForeground();
    if (!window.ok())
      return;

    setType(window.spriteSheetTypeValue());
    setAction(window.exportActionValue());
    setColumns(window.columnsValue());
  }

  Document* document(context->activeDocument());
  Sprite* sprite = document->sprite();
  FrameNumber nframes = sprite->totalFrames();
  int columns;

  switch (m_type) {
    case HorizontalStrip:
      columns = nframes;
      break;
    case VerticalStrip:
      columns = 1;
      break;
    case Matrix:
      columns = m_columns;
      break;
  }

  columns = MID(1, columns, nframes);

  int sheet_w = sprite->width()*columns;
  int sheet_h = sprite->height()*((nframes/columns)+((nframes%columns)>0?1:0));
  base::UniquePtr<Image> resultImage(Image::create(sprite->pixelFormat(), sheet_w, sheet_h));
  base::UniquePtr<Image> tempImage(Image::create(sprite->pixelFormat(), sprite->width(), sprite->height()));
  raster::clear_image(resultImage, 0);

  int column = 0, row = 0;
  for (FrameNumber frame(0); frame<nframes; ++frame) {
    // TODO "tempImage" could not be necessary if we could specify
    // destination clipping bounds in Sprite::render() function.
    tempImage->clear(0);
    sprite->render(tempImage, 0, 0, frame);
    resultImage->copy(tempImage, column*sprite->width(), row*sprite->height());

    if (++column >= columns) {
      column = 0;
      ++row;
    }
  }

  // Store the frame in the current editor so we can restore it
  // after change and restore the setTotalFrames() number.
  FrameNumber oldSelectedFrame = (current_editor ? current_editor->frame():
    FrameNumber(0));

  {
    // The following steps modify the sprite, so we wrap all
    // operations in a undo-transaction.
    ContextWriter writer(context);
    UndoTransaction undoTransaction(writer.context(), "Export Sprite Sheet", undo::ModifyDocument);
    DocumentApi api = document->getApi();

    // Add the layer in the sprite.
    LayerImage* resultLayer = api.newLayer(sprite);

    // Add the image into the sprite's stock
    int indexInStock = api.addImageInStock(sprite, resultImage);
    resultImage.release();

    // Create the cel.
    base::UniquePtr<Cel> resultCel(new Cel(FrameNumber(0), indexInStock));

    // Add the cel in the layer.
    api.addCel(resultLayer, resultCel);
    resultCel.release();

    // Copy the list of layers (because we will modify it in the iteration).
    LayerList layers = sprite->folder()->getLayersList();

    // Remove all other layers
    for (LayerIterator it=layers.begin(), end=layers.end(); it!=end; ++it) {
      if (*it != resultLayer)
        api.removeLayer(*it);
    }

    // Change the number of frames (just one, the sprite sheet). As
    // we are using the observable API, all DocumentView will change
    // its current frame to frame 1. We'll try to restore the
    // selected frame for the current_editor later.
    api.setTotalFrames(sprite, FrameNumber(1));

    // Set the size of the sprite to the tile size.
    api.setSpriteSize(sprite, sheet_w, sheet_h);

    undoTransaction.commit();

    // Draw the document with the new dimensions in the screen.
    update_screen_for_document(document);
  }

  // This flag indicates if we've to undo the last action (so we
  // back to the original sprite dimensions).
  bool undo = false;

  Params params;
  if (!m_filename.empty())
    params.set("filename", m_filename.c_str());

  SaveFileBaseCommand* command = NULL;

  // Do the "Export Action"
  switch (m_action) {

    case SaveCopyAs:
      command = static_cast<SaveFileBaseCommand*>(CommandsModule::instance()
        ->getCommandByName(CommandId::SaveFileCopyAs));

      context->executeCommand(command, &params);

      // Always go back, as we are using "Save Copy As", so the user
      // wants to continue editing the original sprite.
      undo = true;
      break;

    case SaveAs:
      command = static_cast<SaveFileBaseCommand*>(CommandsModule::instance()
        ->getCommandByName(CommandId::SaveFileAs));

      context->executeCommand(command, &params);

      // If the command was cancelled, we go back to the original
      // state, if the sprite sheet was saved then we don't undo
      // because the user wants to edit the sprite sheet.
      undo = (document->isModified());
      break;

    case Save:
      command = static_cast<SaveFileBaseCommand*>(CommandsModule::instance()
        ->getCommandByName(CommandId::SaveFile));

      context->executeCommand(command, &params);

      // Same case as "Save As"
      undo = (document->isModified());
      break;

    case DoNotSave:
      // Do not undo as the user wants to edit the sprite sheet
      // before to save the file.
      undo = false;
      break;
  }

  // Set export data
  if (command != NULL) {
    doc::ExportData::Type type;
    switch (m_type) {
      case ExportSpriteSheetCommand::HorizontalStrip:
        type = doc::ExportData::HorizontalStrip;
        break;
      case ExportSpriteSheetCommand::VerticalStrip:
        type = doc::ExportData::VerticalStrip;
        break;
      case ExportSpriteSheetCommand::Matrix:
        type = doc::ExportData::Matrix;
        break;
    }
    doc::ExportDataPtr data(new doc::ExportData);
    data->setType(type);
    data->setColumns(columns);
    data->setFilename(command->selectedFilename());
    document->setExportData(data);
  }

  // Undo the sprite sheet conversion
  if (undo) {
    if (document->getUndo()->canUndo()) {
      document->getUndo()->doUndo();

      // We've to restore the previously selected frame. As we've
      // called setTotalFrames(), all document observers
      // (current_editor included) have changed its current frame to
      // the first one (to a visible/editable frame). The "undo"
      // action doesn't restore the previously selected frame in
      // observers, so at least we can restore the current_editor's
      // frame.
      if (current_editor)
        current_editor->setFrame(oldSelectedFrame);
    }

    document->generateMaskBoundaries();
    document->destroyExtraCel(); // Regenerate extras

    // Redraw the sprite.
    update_screen_for_document(document);
  }
}

Command* CommandFactory::createExportSpriteSheetCommand()
{
  return new ExportSpriteSheetCommand;
}

} // namespace app
