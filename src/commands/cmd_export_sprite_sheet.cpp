/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2011  David Capello
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

#include "config.h"

#include "base/bind.h"
#include "commands/command.h"
#include "commands/commands.h"
#include "document_wrappers.h"
#include "gui/gui.h"
#include "ini_file.h"
#include "modules/gui.h"
#include "modules/palettes.h"
#include "raster/cel.h"
#include "raster/image.h"
#include "raster/layer.h"
#include "raster/palette.h"
#include "raster/sprite.h"
#include "raster/stock.h"
#include "undo/undo_history.h"
#include "undo_transaction.h"

#include <limits>

//////////////////////////////////////////////////////////////////////
// ExportSpriteSheetFrame

class ExportSpriteSheetFrame : public Frame
{
  enum SpriteSheetType { HorizontalStrip, VerticalStrip, Matrix };
  enum ExportAction { SaveCopyAs, SaveAs, Save, DoNotSave };

public:
  ExportSpriteSheetFrame(Context* context)
    : Frame(false, "Export Sprite Sheet")
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

    m_sheetType.Change.connect(&ExportSpriteSheetFrame::onSheetTypeChange, this);
    m_export.Click.connect(Bind<void>(&ExportSpriteSheetFrame::onExport, this));
    m_cancel.Click.connect(Bind<void>(&ExportSpriteSheetFrame::onCancel, this));

    onSheetTypeChange();
  }

  ~ExportSpriteSheetFrame()
  {
  }

protected:

  void onSheetTypeChange()
  {
    bool state = false;
    switch (m_sheetType.getSelectedItem()) {
      case Matrix:
	state = true;
	break;
    }
    m_columnsLabel.setVisible(state);
    m_columns.setVisible(state);

    gfx::Size reqSize = getPreferredSize();
    JRect rect = jrect_new(rc->x1, rc->y1, rc->x1+reqSize.w, rc->y1+reqSize.h);
    move_window(rect);
    jrect_free(rect);

    invalidate();
  }

  void onExport()
  {
    Sprite* sprite = m_document->getSprite();
    int nframes = sprite->getTotalFrames();
    int columns;

    switch (m_sheetType.getSelectedItem()) {
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
    UniquePtr<Image> resultImage(image_new(sprite->getImgType(), sheet_w, sheet_h));
    image_clear(resultImage, 0);

    int oldFrame = sprite->getCurrentFrame();
    int column = 0, row = 0;
    for (int frame=0; frame<nframes; ++frame) {
      sprite->setCurrentFrame(frame);
      sprite->render(resultImage, column*sprite->getWidth(), row*sprite->getHeight());

      if (++column >= columns) {
	column = 0;
	++row;
      }
    }
    sprite->setCurrentFrame(oldFrame);

    {
      // The following steps modify the sprite, so we wrap all
      // operations in a undo-transaction.
      UndoTransaction undoTransaction(m_document, "Export Sprite Sheet", undo::ModifyDocument);

      // Add the layer in the sprite.
      LayerImage* resultLayer = undoTransaction.newLayer();

      // Add the image into the sprite's stock
      int indexInStock = undoTransaction.addImageInStock(resultImage);
      resultImage.release();

      // Create the cel.
      UniquePtr<Cel> resultCel(new Cel(0, indexInStock));

      // Add the cel in the layer.
      undoTransaction.addCel(resultLayer, resultCel);
      resultCel.release();

      // Copy the list of layers (because we will modify it in the iteration).
      LayerList layers = sprite->getFolder()->get_layers_list();

      // Remove all other layers
      for (LayerIterator it=layers.begin(), end=layers.end(); it!=end; ++it) {
	if (*it != resultLayer)
	  undoTransaction.removeLayer(*it);
      }

      // Change the number of frames (just one, the sprite sheet)
      undoTransaction.setNumberOfFrames(1);
      undoTransaction.setCurrentFrame(0);

      // Set the size of the sprite to the tile size.
      undoTransaction.setSpriteSize(sheet_w, sheet_h);
      undoTransaction.commit();

      // Draw the document with the new dimensions in the screen.
      update_screen_for_document(m_document);
    }

    bool undo = false;

    // Do the "Export Action"
    switch (m_exportAction.getSelectedItem()) {

      case SaveCopyAs:
	{
	  Command* command = CommandsModule::instance()
	    ->getCommandByName(CommandId::SaveFileCopyAs);

	  m_context->executeCommand(command);
	}
	undo = true;
	break;

      case SaveAs:
	{
	  Command* command = CommandsModule::instance()
	    ->getCommandByName(CommandId::SaveFileAs);

	  m_context->executeCommand(command);
	}
	undo = (m_document->isModified());
	break;

      case Save:
	{
	  Command* command = CommandsModule::instance()
	    ->getCommandByName(CommandId::SaveFile);

	  m_context->executeCommand(command);
	}
	undo = (m_document->isModified());
	break;

      case DoNotSave:
	undo = false;
	break;
    }

    // Undo the sprite sheet conversion
    if (undo) {
      m_document->getUndoHistory()->doUndo();
      m_document->generateMaskBoundaries();
      m_document->destroyExtraCel(); // Regenerate extras

      // Redraw the document just in case it has changed (the
      // transaction was rollbacked).
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

//////////////////////////////////////////////////////////////////////
// ExportSpriteSheetCommand

class ExportSpriteSheetCommand : public Command
{
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
  FramePtr frame(new ExportSpriteSheetFrame(context));
  frame->open_window_fg();
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::createExportSpriteSheetCommand()
{
  return new ExportSpriteSheetCommand;
}
