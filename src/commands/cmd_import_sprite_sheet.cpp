/* ASEPRITE
 * Copyright (C) 2001-2012  David Capello
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
#include "commands/params.h"
#include "dialogs/filesel.h"
#include "document_wrappers.h"
#include "gui/gui.h"
#include "ini_file.h"
#include "modules/editors.h"
#include "modules/gui.h"
#include "modules/palettes.h"
#include "raster/cel.h"
#include "raster/image.h"
#include "raster/layer.h"
#include "raster/palette.h"
#include "raster/sprite.h"
#include "raster/stock.h"
#include "undo_transaction.h"
#include "widgets/drop_down_button.h"
#include "widgets/editor/editor.h"
#include "widgets/editor/editor_decorator.h"
#include "widgets/editor/select_box_state.h"
#include "widgets/editor/standby_state.h"

#include <allegro/color.h>

//////////////////////////////////////////////////////////////////////
// ImportSpriteSheetFrame

class ImportSpriteSheetFrame : public Frame,
                               public SelectBoxDelegate
{
public:
  ImportSpriteSheetFrame(Context* context)
    : Frame(false, "Import Sprite Sheet")
    , m_context(context)
    , m_document(NULL)
    , m_grid(4, false)
    , m_selectFile("Select File")
    , m_x(4, "0")
    , m_y(4, "0")
    , m_width(4, "16")
    , m_height(4, "16")
    , m_import("Import")
    , m_cancel("Cancel")
    , m_editor(NULL)
    , m_fileOpened(false)
  {
    addChild(&m_grid);
    m_grid.addChildInCell(&m_selectFile, 4, 1, 0);
    m_grid.addChildInCell(new Label("X"), 1, 1, 0);
    m_grid.addChildInCell(&m_x, 1, 1, 0);
    m_grid.addChildInCell(new Label("Width"), 1, 1, 0);
    m_grid.addChildInCell(&m_width, 1, 1, 0);
    m_grid.addChildInCell(new Label("Y"), 1, 1, 0);
    m_grid.addChildInCell(&m_y, 1, 1, 0);
    m_grid.addChildInCell(new Label("Height"), 1, 1, 0);
    m_grid.addChildInCell(&m_height, 1, 1, 0);

    {
      Box* hbox1 = new Box(JI_HORIZONTAL);
      Box* hbox2 = new Box(JI_HORIZONTAL | JI_HOMOGENEOUS);
      hbox1->addChild(new BoxFiller);
      hbox1->addChild(hbox2);
      hbox2->addChild(&m_import);
      hbox2->addChild(&m_cancel);
      jwidget_set_min_size(&m_import, 60, 0);
      m_grid.addChildInCell(hbox1, 4, 1, 0);
    }

    m_x.EntryChange.connect(Bind<void>(&ImportSpriteSheetFrame::onEntriesChange, this));
    m_y.EntryChange.connect(Bind<void>(&ImportSpriteSheetFrame::onEntriesChange, this));
    m_width.EntryChange.connect(Bind<void>(&ImportSpriteSheetFrame::onEntriesChange, this));
    m_height.EntryChange.connect(Bind<void>(&ImportSpriteSheetFrame::onEntriesChange, this));

    m_selectFile.Click.connect(&ImportSpriteSheetFrame::onSelectFile, this);
    m_selectFile.DropDownClick.connect(&ImportSpriteSheetFrame::onDropDown, this);
    m_import.Click.connect(Bind<void>(&ImportSpriteSheetFrame::onImport, this));
    m_cancel.Click.connect(Bind<void>(&ImportSpriteSheetFrame::onCancel, this));
  }

  ~ImportSpriteSheetFrame()
  {
    releaseEditor();
  }

protected:

  void onSelectFile()
  {
    Document* oldActiveDocument = m_context->getActiveDocument();
    Command* openFile = CommandsModule::instance()->getCommandByName(CommandId::OpenFile);
    Params params;
    params.set("filename", "");
    openFile->loadParams(&params);
    openFile->execute(m_context);

    // The user have selected another document.
    if (oldActiveDocument != m_context->getActiveDocument()) {
      selectActiveDocument();
      m_fileOpened = true;
    }
  }

  void onDropDown()
  {
    SharedPtr<Menu> menu(new Menu());
    MenuItem* item = new MenuItem("Use Current Sprite");
    item->Click.connect(&ImportSpriteSheetFrame::onUseCurrentSprite, this);

    if (m_editor || current_editor->getDocument() == NULL)
      item->setEnabled(false);

    menu->addChild(item);

    menu->showPopup(m_selectFile.rc->x1, m_selectFile.rc->y2);
  }

  void onUseCurrentSprite()
  {
    selectActiveDocument();
    m_fileOpened = false;
  }

  void onImport()
  {
    // The user don't select a sheet yet.
    if (!m_document) {
      Alert::show("Import Sprite Sheet<<Select a sprite first.||&Close");
      return;
    }

    // The list of frames imported from the sheet
    std::vector<Image*> animation;

    try {
      Sprite* sprite = m_document->getSprite();

      // As first step, we cut each tile and add them into "animation" list.
      for (int y=m_rect.y; y<sprite->getHeight(); y += m_rect.h) {
        for (int x=m_rect.x; x<sprite->getWidth(); x += m_rect.w) {
          UniquePtr<Image> resultImage(Image::create(sprite->getImgType(), m_rect.w, m_rect.h));

          // Clear the image with mask color.
          image_clear(resultImage, 0);

          // Render the portion of sheet.
          sprite->render(resultImage, -x, -y);
          animation.push_back(resultImage);
          resultImage.release();
        }
      }

      if (animation.size() == 0) {
        Alert::show("Import Sprite Sheet"
                    "<<The specified rectangle does not create any tile."
                    "<<Select a rectangle inside the sprite region."
                    "||&OK");
        return;
      }

      // The following steps modify the sprite, so we wrap all
      // operations in a undo-transaction.
      UndoTransaction undoTransaction(m_document, "Import Sprite Sheet", undo::ModifyDocument);

      // Add the layer in the sprite.
      LayerImage* resultLayer = undoTransaction.newLayer();

      // Add all frames+cels to the new layer
      for (size_t i=0; i<animation.size(); ++i) {
        int indexInStock;

        // Add the image into the sprite's stock
        indexInStock = undoTransaction.addImageInStock(animation[i]);
        animation[i] = NULL;

        // Create the cel.
        UniquePtr<Cel> resultCel(new Cel(i, indexInStock));

        // Add the cel in the layer.
        undoTransaction.addCel(resultLayer, resultCel);
        resultCel.release();
      }

      // Copy the list of layers (because we will modify it in the iteration).
      LayerList layers = sprite->getFolder()->get_layers_list();

      // Remove all other layers
      for (LayerIterator it=layers.begin(), end=layers.end(); it!=end; ++it) {
        if (*it != resultLayer)
          undoTransaction.removeLayer(*it);
      }

      // Change the number of frames
      undoTransaction.setNumberOfFrames(animation.size());
      undoTransaction.setCurrentFrame(0);

      // Set the size of the sprite to the tile size.
      undoTransaction.setSpriteSize(m_rect.w, m_rect.h);

      undoTransaction.commit();
    }
    catch (...) {
      for (size_t i=0; i<animation.size(); ++i)
        delete animation[i];
      throw;
    }

    update_screen_for_document(m_document);
    closeWindow(NULL);
  }

  void onCancel()
  {
    closeWindow(NULL);
  }

  gfx::Rect getRectFromEntries() const
  {
    int w = m_width.getTextInt();
    int h = m_height.getTextInt();

    return gfx::Rect(m_x.getTextInt(),
                     m_y.getTextInt(),
                     std::max<int>(1, w),
                     std::max<int>(1, h));
  }

  void onEntriesChange()
  {
    m_rect = getRectFromEntries();

    // Redraw new rulers position
    if (m_editor) {
      EditorStatePtr state = m_editor->getState();
      if (SelectBoxState* boxState = dynamic_cast<SelectBoxState*>(state.get())) {
        boxState->setBoxBounds(m_rect);
        m_editor->invalidate();
      }
    }
  }

  virtual void onBroadcastMouseMessage(WidgetsList& targets) OVERRIDE
  {
    Frame::onBroadcastMouseMessage(targets);

    // Add the editor as receptor of mouse events too.
    if (m_editor)
      targets.push_back(View::getView(m_editor));
  }

  // SelectBoxDelegate impleentation
  virtual void onChangeRectangle(const gfx::Rect& rect) OVERRIDE
  {
    m_rect = rect;

    m_x.setTextf("%d", m_rect.x);
    m_y.setTextf("%d", m_rect.y);
    m_width.setTextf("%d", m_rect.w);
    m_height.setTextf("%d", m_rect.h);
  }

private:
  void selectActiveDocument()
  {
    Document* oldDocument = m_document;
    m_document = m_context->getActiveDocument();

    // If the user already have selected a file, we have to close that
    // file in order to select the new one.
    if (oldDocument && m_fileOpened) {
      m_context->setActiveDocument(oldDocument);

      Command* closeFile = CommandsModule::instance()->getCommandByName(CommandId::CloseFile);
      closeFile->execute(m_context);
    }

    captureEditor();
  }

  void captureEditor()
  {
    releaseEditor();

    if (m_document && !m_editor) {
      m_rect = getRectFromEntries();
      m_editor = current_editor;

      EditorStatePtr newState(new SelectBoxState(this, m_rect,
                                                 SelectBoxState::PaintRulers |
                                                 SelectBoxState::PaintGrid));
      m_editor->setState(newState);
    }
  }

  void releaseEditor()
  {
    if (m_editor) {
      m_editor->backToPreviousState();
      m_editor = NULL;
    }
  }

  Context* m_context;
  Document* m_document;
  Grid m_grid;
  DropDownButton m_selectFile;
  Entry m_x, m_y, m_width, m_height;
  Button m_import;
  Button m_cancel;
  Editor* m_editor;
  gfx::Rect m_rect;

  // True if the user has been opened the file (instead of selecting
  // the current document).
  bool m_fileOpened;
};

//////////////////////////////////////////////////////////////////////
// ImportSpriteSheetCommand

class ImportSpriteSheetCommand : public Command
{
public:
  ImportSpriteSheetCommand();
  Command* clone() { return new ImportSpriteSheetCommand(*this); }

protected:
  virtual void onExecute(Context* context) OVERRIDE;
};

ImportSpriteSheetCommand::ImportSpriteSheetCommand()
  : Command("ImportSpriteSheet",
            "Import Sprite Sheet",
            CmdRecordableFlag)
{
}

void ImportSpriteSheetCommand::onExecute(Context* context)
{
  FramePtr frame(new ImportSpriteSheetFrame(context));
  frame->open_window_fg();
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::createImportSpriteSheetCommand()
{
  return new ImportSpriteSheetCommand;
}
