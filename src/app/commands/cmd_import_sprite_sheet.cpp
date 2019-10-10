// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/commands/commands.h"
#include "app/commands/params.h"
#include "app/context.h"
#include "app/context_access.h"
#include "app/doc_access.h"
#include "app/doc_api.h"
#include "app/i18n/strings.h"
#include "app/modules/editors.h"
#include "app/modules/gui.h"
#include "app/modules/palettes.h"
#include "app/pref/preferences.h"
#include "app/tx.h"
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
#include "render/render.h"
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

    sheetType()->Change.connect(base::Bind<void>(&ImportSpriteSheetWindow::onSheetTypeChange, this));
    x()->Change.connect(base::Bind<void>(&ImportSpriteSheetWindow::onEntriesChange, this));
    y()->Change.connect(base::Bind<void>(&ImportSpriteSheetWindow::onEntriesChange, this));
    width()->Change.connect(base::Bind<void>(&ImportSpriteSheetWindow::onEntriesChange, this));
    height()->Change.connect(base::Bind<void>(&ImportSpriteSheetWindow::onEntriesChange, this));
    paddingEnabled()->Click.connect(base::Bind<void>(&ImportSpriteSheetWindow::onPaddingEnabledChange, this));
    horizontalPadding()->Change.connect(base::Bind<void>(&ImportSpriteSheetWindow::onEntriesChange, this));
    verticalPadding()->Change.connect(base::Bind<void>(&ImportSpriteSheetWindow::onEntriesChange, this));
    partialTiles()->Click.connect(base::Bind<void>(&ImportSpriteSheetWindow::onEntriesChange, this));
    selectFile()->Click.connect(base::Bind<void>(&ImportSpriteSheetWindow::onSelectFile, this));

    remapWindow();
    centerWindow();
    load_window_pos(this, "ImportSpriteSheet");

    if (m_context->activeDocument()) {
      selectActiveDocument();
      m_fileOpened = false;
    }

    onPaddingEnabledChange();
  }

  ~ImportSpriteSheetWindow() {
    releaseEditor();
  }

  SpriteSheetType sheetTypeValue() const {
    return (app::SpriteSheetType)(sheetType()->getSelectedItemIndex()+1);
  }

  bool partialTilesValue() const {
    return partialTiles()->isSelected();
  }

  bool paddingEnabledValue() const {
    return paddingEnabled()->isSelected();
  }

  bool ok() const {
    return closer() == import();
  }

  Doc* document() const {
    return m_document;
  }

  DocumentPreferences* docPref() const {
    return m_docPref;
  }

  gfx::Rect frameBounds() const {
    return m_rect;
  }

  gfx::Size paddingThickness() const {
    return m_padding;
  }

protected:

  void onSheetTypeChange() {
    updateGridState();
  }

  void onSelectFile() {
    Doc* oldActiveDocument = m_context->activeDocument();
    Command* openFile = Commands::instance()->byId(CommandId::OpenFile());
    Params params;
    params.set("filename", "");
    m_context->executeCommand(openFile, params);

    // The user have selected another document.
    if (oldActiveDocument != m_context->activeDocument()) {
      selectActiveDocument();
      m_fileOpened = true;
    }
  }

  gfx::Rect getRectFromEntries() {
    int w = width()->textInt();
    int h = height()->textInt();

    return gfx::Rect(
      x()->textInt(),
      y()->textInt(),
      std::max<int>(1, w),
      std::max<int>(1, h));
  }

  gfx::Size getPaddingFromEntries() {
    int padW = horizontalPadding()->textInt();
    int padH = verticalPadding()->textInt();
    if (padW < 0)
      padW = 0;
    if (padH < 0)
      padH = 0;
    return gfx::Size(padW, padH);
  }

  void onEntriesChange() {
    m_rect = getRectFromEntries();
    m_padding = getPaddingFromEntries();

    // Redraw new rulers position
    if (m_editor) {
      EditorStatePtr state = m_editor->getState();
      SelectBoxState* boxState = dynamic_cast<SelectBoxState*>(state.get());
      boxState->setBoxBounds(m_rect);
      boxState->setPaddingBounds(m_padding);
      if (partialTilesValue())
        boxState->setFlag(SelectBoxState::Flags::IncludePartialTiles);
      else
        boxState->clearFlag(SelectBoxState::Flags::IncludePartialTiles);
      m_editor->invalidate();
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

  void onChangePadding(const gfx::Size& padding) override {
    if (paddingEnabled()->isSelected()) {
      m_padding = padding;
      if (padding.w < 0)
        m_padding.w = 0;
      if (padding.h < 0)
        m_padding.h = 0;
      horizontalPadding()->setTextf("%d", m_padding.w);
      verticalPadding()->setTextf("%d", m_padding.h);
    }
    else {
      m_padding = gfx::Size(0, 0);
      horizontalPadding()->setTextf("%d", 0);
      verticalPadding()->setTextf("%d", 0);
    }
  }

  std::string onGetContextBarHelp() override {
    return "Select bounds to identify sprite frames";
  }

private:
  void selectActiveDocument() {
    Doc* oldDocument = m_document;
    m_document = m_context->activeDocument();

    // If the user already have selected a file, we have to destroy
    // that file in order to select the new one.
    if (oldDocument) {
      releaseEditor();

      if (m_fileOpened) {
        DocDestroyer destroyer(m_context, oldDocument, 100);
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

      gfx::Rect defBounds = m_docPref->importSpriteSheet.bounds();
      if (defBounds.isEmpty())
        defBounds = m_document->sprite()->gridBounds();
      onChangeRectangle(defBounds);

      gfx::Size defPaddingBounds = m_docPref->importSpriteSheet.paddingBounds();
      if (defPaddingBounds.w < 0 || defPaddingBounds.h < 0)
        defPaddingBounds = gfx::Size(0, 0);
      onChangePadding(defPaddingBounds);

      paddingEnabled()->setSelected(m_docPref->importSpriteSheet.paddingEnabled());
      partialTiles()->setSelected(m_docPref->importSpriteSheet.partialTiles());
      onEntriesChange();
      onPaddingEnabledChange();
    }
  }

  void captureEditor() {
    ASSERT(m_editor == NULL);

    if (m_document && !m_editor) {
      m_rect = getRectFromEntries();
      m_padding = getPaddingFromEntries();
      m_editor = current_editor;
      m_editorState.reset(
        new SelectBoxState(
          this, m_rect,
          SelectBoxState::Flags(
            int(SelectBoxState::Flags::Rulers) |
            int(SelectBoxState::Flags::Grid) |
            int(SelectBoxState::Flags::DarkOutside) |
            int(SelectBoxState::Flags::PaddingRulers)
            )));

      m_editor->setState(m_editorState);
      updateGridState();
    }
  }

  void updateGridState() {
    if (!m_editorState)
      return;

    int flags = (int)static_cast<SelectBoxState*>(m_editorState.get())->getFlags();
    flags = flags & ~((int)SelectBoxState::Flags::HGrid | (int)SelectBoxState::Flags::VGrid);
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

  void onPaddingEnabledChange() {
    const bool state = paddingEnabled()->isSelected();
    horizontalPaddingLabel()->setVisible(state);
    horizontalPadding()->setVisible(state);
    verticalPaddingLabel()->setVisible(state);
    verticalPadding()->setVisible(state);
    if (m_docPref) {
      if (state)
        onChangePadding(m_docPref->importSpriteSheet.paddingBounds());
      else {
        m_docPref->importSpriteSheet.paddingBounds(m_padding);
        onChangePadding(gfx::Size(0, 0));
      }
    }

    onEntriesChange();
    resize();
  }

  void resize() {
    gfx::Size reqSize = sizeHint();
    moveWindow(gfx::Rect(origin(), reqSize));
    layout();
    invalidate();
  }

  Context* m_context;
  Doc* m_document;
  Editor* m_editor;
  EditorStatePtr m_editorState;
  gfx::Rect m_rect;
  gfx::Size m_padding;

  // True if the user has been opened the file (instead of selecting
  // the current document).
  bool m_fileOpened;

  DocumentPreferences* m_docPref;
};

class ImportSpriteSheetCommand : public Command {
public:
  ImportSpriteSheetCommand();

protected:
  virtual void onExecute(Context* context) override;
};

ImportSpriteSheetCommand::ImportSpriteSheetCommand()
  : Command(CommandId::ImportSpriteSheet(), CmdRecordableFlag)
{
}

void ImportSpriteSheetCommand::onExecute(Context* context)
{
  ImportSpriteSheetWindow window(context);

  window.openWindowInForeground();
  if (!window.ok())
    return;

  Doc* document = window.document();
  DocumentPreferences* docPref = window.docPref();
  gfx::Rect frameBounds = window.frameBounds();
  gfx::Size padThickness = window.paddingThickness();
  bool partialTiles = window.partialTilesValue();
  bool paddingEnable = window.paddingEnabledValue();
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
    render.setNewBlend(Preferences::instance().experimental.newBlend());

    // Each sprite in the sheet
    std::vector<gfx::Rect> tileRects;
    int widthStop = sprite->width();
    int heightStop = sprite->height();
    if (partialTiles) {
      widthStop += frameBounds.w-1;
      heightStop += frameBounds.h-1;
    }

    switch (sheetType) {
      case app::SpriteSheetType::Horizontal:
        for (int x=frameBounds.x; x+frameBounds.w<=widthStop; x+=frameBounds.w+padThickness.w) {
          tileRects.push_back(gfx::Rect(x, frameBounds.y, frameBounds.w, frameBounds.h));
        }
        break;
      case app::SpriteSheetType::Vertical:
        for (int y=frameBounds.y; y+frameBounds.h<=heightStop; y+=frameBounds.h+padThickness.h) {
          tileRects.push_back(gfx::Rect(frameBounds.x, y, frameBounds.w, frameBounds.h));
        }
        break;
      case app::SpriteSheetType::Rows:
        for (int y=frameBounds.y; y+frameBounds.h<=heightStop; y+=frameBounds.h+padThickness.h) {
          for (int x=frameBounds.x; x+frameBounds.w<=widthStop; x+=frameBounds.w+padThickness.w) {
            tileRects.push_back(gfx::Rect(x, y, frameBounds.w, frameBounds.h));
          }
        }
        break;
      case app::SpriteSheetType::Columns:
        for (int x=frameBounds.x; x+frameBounds.w<=widthStop; x+=frameBounds.w+padThickness.w) {
          for (int y=frameBounds.y; y+frameBounds.h<=heightStop; y+=frameBounds.h+padThickness.h) {
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
      Alert::show(Strings::alerts_empty_rect_importing_sprite_sheet());
      return;
    }

    // The following steps modify the sprite, so we wrap all
    // operations in a undo-transaction.
    ContextWriter writer(context);
    Tx tx(writer.context(), "Import Sprite Sheet", ModifyDocument);
    DocApi api = document->getApi(tx);

    // Add the layer in the sprite.
    LayerImage* resultLayer = api.newLayer(sprite->root(), "Sprite Sheet");

    // Add all frames+cels to the new layer
    for (size_t i=0; i<animation.size(); ++i) {
      // Create the cel.
      std::unique_ptr<Cel> resultCel(new Cel(frame_t(i), animation[i]));

      // Add the cel in the layer.
      api.addCel(resultLayer, resultCel.get());
      resultCel.release();
    }

    // Copy the list of layers (because we will modify it in the iteration).
    LayerList layers = sprite->root()->layers();

    // Remove all other layers
    for (Layer* child : layers) {
      if (child != resultLayer)
        api.removeLayer(child);
    }

    // Change the number of frames
    api.setTotalFrames(sprite, frame_t(animation.size()));

    // Set the size of the sprite to the tile size.
    api.setSpriteSize(sprite, frameBounds.w, frameBounds.h);

    tx.commit();

    ASSERT(docPref);
    if (docPref) {
      docPref->importSpriteSheet.type(sheetType);
      docPref->importSpriteSheet.bounds(frameBounds);
      docPref->importSpriteSheet.partialTiles(partialTiles);
      docPref->importSpriteSheet.paddingBounds(padThickness);
      docPref->importSpriteSheet.paddingEnabled(paddingEnable);
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
