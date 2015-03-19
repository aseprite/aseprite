// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/commands/commands.h"
#include "app/commands/params.h"
#include "app/context.h"
#include "app/context_access.h"
#include "app/document_access.h"
#include "app/document_api.h"
#include "app/modules/editors.h"
#include "app/modules/gui.h"
#include "app/modules/palettes.h"
#include "app/pref/preferences.h"
#include "app/transaction.h"
#include "app/ui/drop_down_button.h"
#include "app/ui/editor/editor.h"
#include "app/ui/editor/editor_decorator.h"
#include "app/ui/editor/select_box_state.h"
#include "app/ui/editor/standby_state.h"
#include "app/ui/workspace.h"
#include "base/bind.h"
#include "doc/cel.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/palette.h"
#include "doc/primitives.h"
#include "doc/sprite.h"
#include "ui/ui.h"

#include "generated_import_sprite_sheet.h"

namespace app {

using namespace ui;

class ImportSpriteSheetWindow : public app::gen::ImportSpriteSheet
                              , public SelectBoxDelegate {
public:
  ImportSpriteSheetWindow(Context* context)
    : m_context(context)
    , m_document(NULL)
    , m_editor(NULL)
    , m_fileOpened(false)
    , m_docPref(nullptr)
  {
    x()->EntryChange.connect(Bind<void>(&ImportSpriteSheetWindow::onEntriesChange, this));
    y()->EntryChange.connect(Bind<void>(&ImportSpriteSheetWindow::onEntriesChange, this));
    width()->EntryChange.connect(Bind<void>(&ImportSpriteSheetWindow::onEntriesChange, this));
    height()->EntryChange.connect(Bind<void>(&ImportSpriteSheetWindow::onEntriesChange, this));

    selectFile()->Click.connect(&ImportSpriteSheetWindow::onSelectFile, this);
    selectFile()->DropDownClick.connect(&ImportSpriteSheetWindow::onDropDown, this);
    import()->Click.connect(Bind<void>(&ImportSpriteSheetWindow::onImport, this));
    cancel()->Click.connect(Bind<void>(&ImportSpriteSheetWindow::onCancel, this));

    remapWindow();
    centerWindow();
    load_window_pos(this, "ImportSpriteSheet");
  }

  ~ImportSpriteSheetWindow()
  {
    releaseEditor();
  }

protected:

  void onSelectFile()
  {
    Document* oldActiveDocument = m_context->activeDocument();
    Command* openFile = CommandsModule::instance()->getCommandByName(CommandId::OpenFile);
    Params params;
    params.set("filename", "");
    openFile->loadParams(params);
    openFile->execute(m_context);

    // The user have selected another document.
    if (oldActiveDocument != m_context->activeDocument()) {
      selectActiveDocument();
      m_fileOpened = true;
    }
  }

  void onDropDown()
  {
    SharedPtr<Menu> menu(new Menu());
    MenuItem* item = new MenuItem("Use Current Sprite");
    item->Click.connect(&ImportSpriteSheetWindow::onUseCurrentSprite, this);

    if (m_editor || !current_editor || current_editor->document() == NULL)
      item->setEnabled(false);

    menu->addChild(item);

    const gfx::Rect& bounds = selectFile()->getBounds();
    menu->showPopup(gfx::Point(bounds.x, bounds.y+bounds.h));
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
    std::vector<ImageRef> animation;

    try {
      Sprite* sprite = m_document->sprite();
      frame_t currentFrame = m_context->activeLocation().frame();
      render::Render render;

      // As first step, we cut each tile and add them into "animation" list.
      for (int y=m_rect.y; y<sprite->height(); y += m_rect.h) {
        for (int x=m_rect.x; x<sprite->width(); x += m_rect.w) {
          ImageRef resultImage(
            Image::create(sprite->pixelFormat(), m_rect.w, m_rect.h));

          // Render the portion of sheet.
          render.renderSprite(resultImage, sprite, currentFrame,
            gfx::Clip(0, 0, x, y, m_rect.w, m_rect.h));

          animation.push_back(resultImage);
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
      ContextWriter writer(m_context);
      Transaction transaction(writer.context(), "Import Sprite Sheet", ModifyDocument);
      DocumentApi api = m_document->getApi(transaction);

      // Add the layer in the sprite.
      LayerImage* resultLayer = api.newLayer(sprite);

      // Add all frames+cels to the new layer
      for (size_t i=0; i<animation.size(); ++i) {
        // Create the cel.
        base::UniquePtr<Cel> resultCel(new Cel(frame_t(i), animation[i]));

        // Add the cel in the layer.
        api.addCel(resultLayer, resultCel);
        resultCel.release();
      }

      // Copy the list of layers (because we will modify it in the iteration).
      LayerList layers = sprite->folder()->getLayersList();

      // Remove all other layers
      for (LayerIterator it=layers.begin(), end=layers.end(); it!=end; ++it) {
        if (*it != resultLayer)
          api.removeLayer(*it);
      }

      // Change the number of frames
      api.setTotalFrames(sprite, frame_t(animation.size()));

      // Set the size of the sprite to the tile size.
      api.setSpriteSize(sprite, m_rect.w, m_rect.h);

      transaction.commit();

      ASSERT(m_docPref);
      if (m_docPref)
        m_docPref->importSpriteSheet.bounds(m_rect);
    }
    catch (...) {
      throw;
    }

    update_screen_for_document(m_document);
    closeWindow(NULL);
  }

  void onCancel()
  {
    closeWindow(NULL);
  }

  gfx::Rect getRectFromEntries()
  {
    int w = width()->getTextInt();
    int h = height()->getTextInt();

    return gfx::Rect(
      x()->getTextInt(),
      y()->getTextInt(),
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

  bool onProcessMessage(ui::Message* msg) override
  {
    switch (msg->type()) {
      case kCloseMessage:
        save_window_pos(this, "ImportSpriteSheet");
        break;
    }
    return Window::onProcessMessage(msg);
  }

  virtual void onBroadcastMouseMessage(WidgetsList& targets) override
  {
    Window::onBroadcastMouseMessage(targets);

    // Add the editor as receptor of mouse events too.
    if (m_editor)
      targets.push_back(View::getView(m_editor));
  }

  // SelectBoxDelegate impleentation
  virtual void onChangeRectangle(const gfx::Rect& rect) override
  {
    m_rect = rect;

    x()->setTextf("%d", m_rect.x);
    y()->setTextf("%d", m_rect.y);
    width()->setTextf("%d", m_rect.w);
    height()->setTextf("%d", m_rect.h);
  }

private:
  void selectActiveDocument()
  {
    Document* oldDocument = m_document;
    m_document = m_context->activeDocument();

    // If the user already have selected a file, we have to destroy
    // that file in order to select the new one.
    if (oldDocument && m_fileOpened) {
      releaseEditor();

      DocumentDestroyer destroyer(m_context, oldDocument);
      destroyer.destroyDocument();
    }

    captureEditor();

    if (m_document) {
      m_docPref = &App::instance()->preferences().document(m_document);

      onChangeRectangle(m_docPref->importSpriteSheet.bounds());
      onEntriesChange();
    }
  }

  void captureEditor()
  {
    ASSERT(m_editor == NULL);

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
  Editor* m_editor;
  gfx::Rect m_rect;

  // True if the user has been opened the file (instead of selecting
  // the current document).
  bool m_fileOpened;

  DocumentPreferences* m_docPref;
};

class ImportSpriteSheetCommand : public Command {
public:
  ImportSpriteSheetCommand();
  Command* clone() const override { return new ImportSpriteSheetCommand(*this); }

protected:
  virtual void onExecute(Context* context) override;
};

ImportSpriteSheetCommand::ImportSpriteSheetCommand()
  : Command("ImportSpriteSheet",
            "Import Sprite Sheet",
            CmdRecordableFlag)
{
}

void ImportSpriteSheetCommand::onExecute(Context* context)
{
  base::UniquePtr<Window> window(new ImportSpriteSheetWindow(context));
  window->openWindowInForeground();
}

Command* CommandFactory::createImportSpriteSheetCommand()
{
  return new ImportSpriteSheetCommand;
}

} // namespace app
