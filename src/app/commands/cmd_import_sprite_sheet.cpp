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

#include "import_sprite_sheet.xml.h"

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
    , m_docPref(nullptr) {
    import()->setEnabled(false);

    static_assert(
      (int)app::SpriteSheetType::Horizontal == 1 &&
      (int)app::SpriteSheetType::Vertical == 2 &&
      (int)app::SpriteSheetType::Rows == 3 &&
      (int)app::SpriteSheetType::Columns == 4,
      "SpriteSheetType enum changed");

    sheetType()->addItem("Horizontal Strip");
    sheetType()->addItem("Vertical Strip");
    sheetType()->addItem("By Rows");
    sheetType()->addItem("By Columns");
    sheetType()->setSelectedItemIndex((int)app::SpriteSheetType::Rows-1);

    sheetType()->Change.connect(Bind<void>(&ImportSpriteSheetWindow::onSheetTypeChange, this));
    x()->Change.connect(Bind<void>(&ImportSpriteSheetWindow::onEntriesChange, this));
    y()->Change.connect(Bind<void>(&ImportSpriteSheetWindow::onEntriesChange, this));
    width()->Change.connect(Bind<void>(&ImportSpriteSheetWindow::onEntriesChange, this));
    height()->Change.connect(Bind<void>(&ImportSpriteSheetWindow::onEntriesChange, this));
    selectFile()->Click.connect(Bind<void>(&ImportSpriteSheetWindow::onSelectFile, this));

    remapWindow();
    centerWindow();
    load_window_pos(this, "ImportSpriteSheet");

    if (m_context->activeDocument()) {
      selectActiveDocument();
      m_fileOpened = false;
    }
  }

  ~ImportSpriteSheetWindow() {
    releaseEditor();
  }

  SpriteSheetType sheetTypeValue() const {
    return (app::SpriteSheetType)(sheetType()->getSelectedItemIndex()+1);
  }

  bool ok() const {
    return getKiller() == import();
  }

  Document* document() const {
    return m_document;
  }

  DocumentPreferences* docPref() const {
    return m_docPref;
  }

  gfx::Rect frameBounds() const {
    return m_rect;
  }

protected:

  void onSheetTypeChange() {
    updateGridState();
  }

  void onSelectFile() {
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

  gfx::Rect getRectFromEntries() {
    int w = width()->getTextInt();
    int h = height()->getTextInt();

    return gfx::Rect(
      x()->getTextInt(),
      y()->getTextInt(),
      std::max<int>(1, w),
      std::max<int>(1, h));
  }

  void onEntriesChange() {
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

  bool onProcessMessage(ui::Message* msg) override {
    switch (msg->type()) {
      case kCloseMessage:
        save_window_pos(this, "ImportSpriteSheet");
        break;
    }
    return Window::onProcessMessage(msg);
  }

  void onBroadcastMouseMessage(WidgetsList& targets) override {
    Window::onBroadcastMouseMessage(targets);

    // Add the editor as receptor of mouse events too.
    if (m_editor)
      targets.push_back(View::getView(m_editor));
  }

  // SelectBoxDelegate impleentation
  void onChangeRectangle(const gfx::Rect& rect) override {
    m_rect = rect;

    x()->setTextf("%d", m_rect.x);
    y()->setTextf("%d", m_rect.y);
    width()->setTextf("%d", m_rect.w);
    height()->setTextf("%d", m_rect.h);
  }

  std::string onGetContextBarHelp() override {
    return "Select bounds to identify sprite frames";
  }

private:
  void selectActiveDocument() {
    Document* oldDocument = m_document;
    m_document = m_context->activeDocument();

    // If the user already have selected a file, we have to destroy
    // that file in order to select the new one.
    if (oldDocument) {
      releaseEditor();

      if (m_fileOpened) {
        DocumentDestroyer destroyer(m_context, oldDocument, 100);
        destroyer.destroyDocument();
      }
    }

    captureEditor();

    import()->setEnabled(m_document ? true: false);

    if (m_document) {
      m_docPref = &Preferences::instance().document(m_document);

      if (m_docPref->importSpriteSheet.type() >= app::SpriteSheetType::Horizontal &&
          m_docPref->importSpriteSheet.type() <= app::SpriteSheetType::Columns)
        sheetType()->setSelectedItemIndex((int)m_docPref->importSpriteSheet.type()-1);
      else
        sheetType()->setSelectedItemIndex((int)app::SpriteSheetType::Rows-1);

      onChangeRectangle(m_docPref->importSpriteSheet.bounds());
      onEntriesChange();
    }
  }

  void captureEditor() {
    ASSERT(m_editor == NULL);

    if (m_document && !m_editor) {
      m_rect = getRectFromEntries();
      m_editor = current_editor;
      m_editorState.reset(
        new SelectBoxState(
          this, m_rect,
          SelectBoxState::Flags(
            int(SelectBoxState::Flags::Rulers) |
            int(SelectBoxState::Flags::Grid))));

      m_editor->setState(m_editorState);
      updateGridState();
    }
  }

  void updateGridState() {
    if (!m_editorState)
      return;

    int flags = int(SelectBoxState::Flags::Rulers);
    switch (sheetTypeValue()) {
      case SpriteSheetType::Horizontal:
        flags |= int(SelectBoxState::Flags::HGrid);
        break;
      case SpriteSheetType::Vertical:
        flags |= int(SelectBoxState::Flags::VGrid);
        break;
      case SpriteSheetType::Rows:
      case SpriteSheetType::Columns:
        flags |= int(SelectBoxState::Flags::Grid);
        break;
    }

    static_cast<SelectBoxState*>(m_editorState.get())->setFlags(SelectBoxState::Flags(flags));
    m_editor->invalidate();
  }

  void releaseEditor() {
    if (m_editor) {
      m_editor->backToPreviousState();
      m_editor = NULL;
    }
  }

  Context* m_context;
  Document* m_document;
  Editor* m_editor;
  EditorStatePtr m_editorState;
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
  ImportSpriteSheetWindow window(context);

  window.openWindowInForeground();
  if (!window.ok())
    return;

  Document* document = window.document();
  DocumentPreferences* docPref = window.docPref();
  gfx::Rect frameBounds = window.frameBounds();
  auto sheetType = window.sheetTypeValue();

  ASSERT(document);
  if (!document)
    return;

  // The list of frames imported from the sheet
  std::vector<ImageRef> animation;

  try {
    Sprite* sprite = document->sprite();
    frame_t currentFrame = context->activeSite().frame();
    render::Render render;

    // Each sprite in the sheet
    std::vector<gfx::Rect> tileRects;

    switch (sheetType) {
      case app::SpriteSheetType::Horizontal:
        for (int x=frameBounds.x; x+frameBounds.w<=sprite->width(); x += frameBounds.w) {
          tileRects.push_back(gfx::Rect(x, frameBounds.y, frameBounds.w, frameBounds.h));
        }
        break;
      case app::SpriteSheetType::Vertical:
        for (int y=frameBounds.y; y+frameBounds.h<=sprite->height(); y += frameBounds.h) {
          tileRects.push_back(gfx::Rect(frameBounds.x, y, frameBounds.w, frameBounds.h));
        }
        break;
      case app::SpriteSheetType::Rows:
        for (int y=frameBounds.y; y+frameBounds.h<=sprite->height(); y += frameBounds.h) {
          for (int x=frameBounds.x; x+frameBounds.w<=sprite->width(); x += frameBounds.w) {
            tileRects.push_back(gfx::Rect(x, y, frameBounds.w, frameBounds.h));
          }
        }
        break;
      case app::SpriteSheetType::Columns:
        for (int x=frameBounds.x; x+frameBounds.w<=sprite->width(); x += frameBounds.w) {
          for (int y=frameBounds.y; y+frameBounds.h<=sprite->height(); y += frameBounds.h) {
            tileRects.push_back(gfx::Rect(x, y, frameBounds.w, frameBounds.h));
          }
        }
        break;
    }

    // As first step, we cut each tile and add them into "animation" list.
    for (const auto& tileRect : tileRects) {
      ImageRef resultImage(
        Image::create(
          sprite->pixelFormat(), tileRect.w, tileRect.h));

      // Render the portion of sheet.
      render.renderSprite(
        resultImage.get(), sprite, currentFrame,
        gfx::Clip(0, 0, tileRect));

      animation.push_back(resultImage);
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
    ContextWriter writer(context);
    Transaction transaction(writer.context(), "Import Sprite Sheet", ModifyDocument);
    DocumentApi api = document->getApi(transaction);

    // Add the layer in the sprite.
    LayerImage* resultLayer = api.newLayer(sprite, "Sprite Sheet");

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
    api.setSpriteSize(sprite, frameBounds.w, frameBounds.h);

    transaction.commit();

    ASSERT(docPref);
    if (docPref) {
      docPref->importSpriteSheet.type(sheetType);
      docPref->importSpriteSheet.bounds(frameBounds);
    }
  }
  catch (...) {
    throw;
  }

  update_screen_for_document(document);
}

Command* CommandFactory::createImportSpriteSheetCommand()
{
  return new ImportSpriteSheetCommand;
}

} // namespace app
