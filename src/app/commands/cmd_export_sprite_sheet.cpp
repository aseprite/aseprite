/* ASEPRITE
 * Copyright (C) 2001-2013  David Capello
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

#include "app/commands/command.h"
#include "app/commands/commands.h"
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
#include "raster/sprite.h"
#include "raster/stock.h"
#include "ui/ui.h"

#include <limits>

namespace app {

using namespace ui;

class ExportSpriteSheetWindow : public Window {
  enum SpriteSheetType { HorizontalStrip, VerticalStrip, Matrix };
  enum ExportAction { SaveCopyAs, SaveAs, Save, DoNotSave };

public:
  ExportSpriteSheetWindow(Context* context)
    : Window(false, "Export Sprite Sheet")
    , m_context(context)
    , m_document(context->getActiveDocument())
    , m_grid(4, false)
    , m_columnsLabel("Columns:")
    , m_columns(4, "4")
    , m_exportActionLabel("Export Action:")
    , m_export("Export")
    , m_cancel("Cancel")
  {
    m_sheetType.addItem("Horizontal Strip");
    m_sheetType.addItem("Vertical Strip");
    m_sheetType.addItem("Matrix");

    m_exportAction.addItem("Save Copy As...");
    m_exportAction.addItem("Save As...");
    m_exportAction.addItem("Save");
    m_exportAction.addItem("Do Not Save");

    addChild(&m_grid);
    m_grid.addChildInCell(new Label("Sheet Type:"), 1, 1, 0);
    m_grid.addChildInCell(&m_sheetType, 3, 1, 0);
    m_grid.addChildInCell(&m_columnsLabel, 1, 1, 0);
    m_grid.addChildInCell(&m_columns, 3, 1, 0);
    m_grid.addChildInCell(&m_exportActionLabel, 1, 1, 0);
    m_grid.addChildInCell(&m_exportAction, 3, 1, 0);

    {
      Box* hbox1 = new Box(JI_HORIZONTAL);
      Box* hbox2 = new Box(JI_HORIZONTAL | JI_HOMOGENEOUS);
      hbox1->addChild(new BoxFiller);
      hbox1->addChild(hbox2);
      hbox2->addChild(&m_export);
      hbox2->addChild(&m_cancel);
      jwidget_set_min_size(&m_export, 60, 0);
      m_grid.addChildInCell(hbox1, 4, 1, 0);
    }

    m_sheetType.Change.connect(&ExportSpriteSheetWindow::onSheetTypeChange, this);
    m_export.Click.connect(Bind<void>(&ExportSpriteSheetWindow::onExport, this));
    m_cancel.Click.connect(Bind<void>(&ExportSpriteSheetWindow::onCancel, this));

    onSheetTypeChange();
  }

  ~ExportSpriteSheetWindow()
  {
  }

protected:

  void onSheetTypeChange()
  {
    bool state = false;
    switch (m_sheetType.getSelectedItemIndex()) {
      case Matrix:
        state = true;
        break;
    }
    m_columnsLabel.setVisible(state);
    m_columns.setVisible(state);

    gfx::Size reqSize = getPreferredSize();
    moveWindow(gfx::Rect(rc->x1, rc->y1, reqSize.w, reqSize.h));

    invalidate();
  }

  void onExport()
  {
    Sprite* sprite = m_document->getSprite();
    FrameNumber nframes = sprite->getTotalFrames();
    int columns;

    switch (m_sheetType.getSelectedItemIndex()) {
      case HorizontalStrip:
        columns = nframes;
        break;
      case VerticalStrip:
        columns = 1;
        break;
      case Matrix:
        columns = m_columns.getTextInt();
        break;
    }

    columns = MID(1, columns, nframes);

    int sheet_w = sprite->getWidth()*columns;
    int sheet_h = sprite->getHeight()*((nframes/columns)+((nframes%columns)>0?1:0));
    base::UniquePtr<Image> resultImage(Image::create(sprite->getPixelFormat(), sheet_w, sheet_h));
    base::UniquePtr<Image> tempImage(Image::create(sprite->getPixelFormat(), sprite->getWidth(), sprite->getHeight()));
    image_clear(resultImage, 0);

    int column = 0, row = 0;
    for (FrameNumber frame(0); frame<nframes; ++frame) {
      // TODO "tempImage" could not be necessary if we could specify
      // destination clipping bounds in Sprite::render() function.
      tempImage->clear(0);
      sprite->render(tempImage, 0, 0, frame);
      resultImage->copy(tempImage, column*sprite->getWidth(), row*sprite->getHeight());

      if (++column >= columns) {
        column = 0;
        ++row;
      }
    }

    // Store the frame in the current editor so we can restore it
    // after change and restore the setTotalFrames() number.
    FrameNumber oldSelectedFrame = (current_editor ? current_editor->getFrame():
                                                     FrameNumber(0));

    {
      // The following steps modify the sprite, so we wrap all
      // operations in a undo-transaction.
      ContextWriter writer(m_context);
      UndoTransaction undoTransaction(writer.context(), "Export Sprite Sheet", undo::ModifyDocument);
      DocumentApi api = m_document->getApi();

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
      LayerList layers = sprite->getFolder()->getLayersList();

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
      update_screen_for_document(m_document);
    }

    // This flag indicates if we've to undo the last action (so we
    // back to the original sprite dimensions).
    bool undo = false;

    // Do the "Export Action"
    switch (m_exportAction.getSelectedItemIndex()) {

      case SaveCopyAs:
        {
          Command* command = CommandsModule::instance()
            ->getCommandByName(CommandId::SaveFileCopyAs);

          m_context->executeCommand(command);
        }

        // Always go back, as we are using "Save Copy As", so the user
        // wants to continue editing the original sprite.
        undo = true;
        break;

      case SaveAs:
        {
          Command* command = CommandsModule::instance()
            ->getCommandByName(CommandId::SaveFileAs);

          m_context->executeCommand(command);
        }

        // If the command was cancelled, we go back to the original
        // state, if the sprite sheet was saved then we don't undo
        // because the user wants to edit the sprite sheet.
        undo = (m_document->isModified());
        break;

      case Save:
        {
          Command* command = CommandsModule::instance()
            ->getCommandByName(CommandId::SaveFile);

          m_context->executeCommand(command);
        }

        // Same case as "Save As"
        undo = (m_document->isModified());
        break;

      case DoNotSave:
        // Do not undo as the user wants to edit the sprite sheet
        // before to save the file.
        undo = false;
        break;
    }

    // Undo the sprite sheet conversion
    if (undo) {
      if (m_document->getUndo()->canUndo()) {
        m_document->getUndo()->doUndo();

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

      m_document->generateMaskBoundaries();
      m_document->destroyExtraCel(); // Regenerate extras

      // Redraw the sprite.
      update_screen_for_document(m_document);
    }

    closeWindow(NULL);
  }

  void onCancel()
  {
    closeWindow(NULL);
  }

private:
  Context* m_context;
  Document* m_document;
  Grid m_grid;
  ComboBox m_sheetType;
  Label m_columnsLabel;
  Entry m_columns;
  Label m_exportActionLabel;
  ComboBox m_exportAction;
  Button m_export;
  Button m_cancel;
};

class ExportSpriteSheetCommand : public Command {
public:
  ExportSpriteSheetCommand();
  Command* clone() { return new ExportSpriteSheetCommand(*this); }

protected:
  virtual bool onEnabled(Context* context) OVERRIDE;
  virtual void onExecute(Context* context) OVERRIDE;
};

ExportSpriteSheetCommand::ExportSpriteSheetCommand()
  : Command("ExportSpriteSheet",
            "Export Sprite Sheet",
            CmdRecordableFlag)
{
}

bool ExportSpriteSheetCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable);
}

void ExportSpriteSheetCommand::onExecute(Context* context)
{
  base::UniquePtr<Window> window(new ExportSpriteSheetWindow(context));
  window->openWindowInForeground();
}

Command* CommandFactory::createExportSpriteSheetCommand()
{
  return new ExportSpriteSheetCommand;
}

} // namespace app
