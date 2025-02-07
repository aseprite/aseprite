// Aseprite
// Copyright (C) 2019-2024  Igara Studio S.A.
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
#include "app/commands/new_params.h"
#include "app/console.h"
#include "app/context.h"
#include "app/context_access.h"
#include "app/doc_access.h"
#include "app/doc_api.h"
#include "app/i18n/strings.h"
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

gfx::Size calcFrameSize(gfx::Size availSize,
                        int cols,
                        int rows,
                        const gfx::Point& frameOrigin,
                        const gfx::Size& padding)
{
  if (cols <= 0)
    cols = 1;
  if (rows <= 0)
    rows = 1;

  availSize.w -= (frameOrigin.x + padding.w * (cols - 1));
  availSize.h -= (frameOrigin.y + padding.h * (rows - 1));

  return gfx::Size(availSize.w / cols, availSize.h / rows);
}

struct ImportSpriteSheetParams : public NewParams {
  Param<bool> ui{ this, true, "ui" };
  Param<app::SpriteSheetType> type{ this, app::SpriteSheetType::None, "type" };
  Param<gfx::Rect> frameBounds{ this, gfx::Rect(0, 0, 0, 0), "frameBounds" };
  Param<gfx::Size> padding{ this, gfx::Size(0, 0), "padding" };
  Param<bool> partialTiles{ this, false, "partialTiles" };
  // Columns and rows are optional, and they just help in calculating
  // frameBounds automatically when the number of columns and rows are known
  // beforehand. So, if these are specified along frameBounds, then frameBounds
  // width/height might be recalculated.
  Param<int> columns{ this, 0, "columns" };
  Param<int> rows{ this, 0, "rows" };
};

class ImportSpriteSheetWindow : public app::gen::ImportSpriteSheet,
                                public SelectBoxDelegate {
public:
  ImportSpriteSheetWindow(const ImportSpriteSheetParams& params, Context* context)
    : m_context(context)
    , m_document(NULL)
    , m_editor(NULL)
    , m_fileOpened(false)
    , m_docPref(nullptr)
  {
    import()->setEnabled(false);

    static_assert((int)app::SpriteSheetType::Horizontal == 1 &&
                    (int)app::SpriteSheetType::Vertical == 2 &&
                    (int)app::SpriteSheetType::Rows == 3 && (int)app::SpriteSheetType::Columns == 4,
                  "SpriteSheetType enum changed");

    sheetType()->addItem(Strings::import_sprite_sheet_type_horz());
    sheetType()->addItem(Strings::import_sprite_sheet_type_vert());
    sheetType()->addItem(Strings::import_sprite_sheet_type_rows());
    sheetType()->addItem(Strings::import_sprite_sheet_type_cols());
    sheetType()->setSelectedItemIndex((int)app::SpriteSheetType::Rows - 1);

    sheetType()->Change.connect([this] { onSheetTypeChange(); });
    x()->Change.connect([this] { onEntriesChange(); });
    y()->Change.connect([this] { onEntriesChange(); });
    width()->Change.connect([this] { onEntriesChange(); });
    height()->Change.connect([this] { onEntriesChange(); });
    columns()->Change.connect([this] { onColumnsChange(); });
    rows()->Change.connect([this] { onRowsChange(); });
    paddingEnabled()->Click.connect([this] { onPaddingEnabledChange(); });
    horizontalPadding()->Change.connect([this] { onEntriesChange(); });
    verticalPadding()->Change.connect([this] { onEntriesChange(); });
    partialTiles()->Click.connect([this] { onEntriesChange(); });
    selectFile()->Click.connect([this] { onSelectFile(); });

    remapWindow();
    centerWindow();
    load_window_pos(this, "ImportSpriteSheet");

    if (m_context->activeDocument()) {
      selectActiveDocument();
      m_fileOpened = false;
    }

    if (params.type.isSet())
      sheetType()->setSelectedItemIndex((int)params.type() - 1);

    if (params.frameBounds.isSet()) {
      x()->setTextf("%d", params.frameBounds().x);
      y()->setTextf("%d", params.frameBounds().y);
      width()->setTextf("%d", params.frameBounds().w);
      height()->setTextf("%d", params.frameBounds().h);
    }

    paddingEnabled()->setSelected(params.padding.isSet());
    if (params.padding.isSet()) {
      if (m_docPref)
        m_docPref->importSpriteSheet.paddingBounds(params.padding());
      else {
        horizontalPadding()->setTextf("%d", params.padding().w);
        verticalPadding()->setTextf("%d", params.padding().h);
      }
    }

    if (params.partialTiles.isSet())
      partialTiles()->setSelected(params.partialTiles());

    onPaddingEnabledChange();

    if (params.columns.isSet()) {
      columns()->setTextf("%d", params.columns());
      onColumnsChange();
    }

    if (params.rows.isSet()) {
      rows()->setTextf("%d", params.rows());
      onRowsChange();
    }
  }

  ~ImportSpriteSheetWindow() { releaseEditor(); }

  SpriteSheetType sheetTypeValue() const
  {
    return (app::SpriteSheetType)(sheetType()->getSelectedItemIndex() + 1);
  }

  bool partialTilesValue() const { return partialTiles()->isSelected(); }

  bool paddingEnabledValue() const { return paddingEnabled()->isSelected(); }

  bool ok() const { return closer() == import(); }

  Doc* document() const { return m_document; }

  DocumentPreferences* docPref() const { return m_docPref; }

  gfx::Rect frameBounds() const { return m_rect; }

  gfx::Size paddingThickness() const { return m_padding; }

  void updateParams(ImportSpriteSheetParams& params)
  {
    params.type(sheetTypeValue());
    params.frameBounds(frameBounds());
    params.partialTiles(partialTilesValue());

    if (paddingEnabledValue())
      params.padding(paddingThickness());
    else
      params.padding(gfx::Size(0, 0));
  }

protected:
  void onSheetTypeChange()
  {
    updateGridState();
    onEntriesChange();
  }

  void onSelectFile()
  {
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

  gfx::Rect getRectFromEntries()
  {
    int w = width()->textInt();
    int h = height()->textInt();

    return gfx::Rect(x()->textInt(), y()->textInt(), std::max<int>(1, w), std::max<int>(1, h));
  }

  gfx::Size getPaddingFromEntries()
  {
    int padW = horizontalPadding()->textInt();
    int padH = verticalPadding()->textInt();
    if (padW < 0)
      padW = 0;
    if (padH < 0)
      padH = 0;
    return gfx::Size(padW, padH);
  }

  void updateRulers()
  {
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

  void onEntriesChange()
  {
    updateRulers();
    columns()->setTextf("%d", calcColumns());
    rows()->setTextf("%d", calcRows());
  }

#define ON_COLROWS_CHANGE(widtheigth, wh)                                                          \
  if (!m_editor)                                                                                   \
    return;                                                                                        \
  auto frameSize = calcFrameSize(m_editor->sprite()->size(),                                       \
                                 columns()->textInt(),                                             \
                                 rows()->textInt(),                                                \
                                 getRectFromEntries().origin(),                                    \
                                 getPaddingFromEntries());                                         \
  widtheigth()->setTextf("%d", frameSize.wh);                                                      \
  updateRulers();

  void onColumnsChange() { ON_COLROWS_CHANGE(width, w); }

  void onRowsChange() { ON_COLROWS_CHANGE(height, h); }

#undef ON_COLROWS_CHANGE

  bool onProcessMessage(ui::Message* msg) override
  {
    switch (msg->type()) {
      case kCloseMessage: save_window_pos(this, "ImportSpriteSheet"); break;
    }
    return Window::onProcessMessage(msg);
  }

  void onBroadcastMouseMessage(const gfx::Point& screenPos, WidgetsList& targets) override
  {
    Window::onBroadcastMouseMessage(screenPos, targets);

    // Add the editor as receptor of mouse events too.
    if (m_editor)
      targets.push_back(View::getView(m_editor));
  }

#define CALC_SPANS(xy, wh)                                                                         \
  if (!m_editor)                                                                                   \
    return 0;                                                                                      \
  int spans = 0;                                                                                   \
  int rectwh = (m_rect.wh <= 0 ? 1 : m_rect.wh);                                                   \
  int wh = m_editor->sprite()->size().wh - m_rect.xy;                                              \
  while (wh > 0) {                                                                                 \
    wh -= rectwh;                                                                                  \
    if (wh >= 0) {                                                                                 \
      spans++;                                                                                     \
    }                                                                                              \
    wh -= m_padding.wh;                                                                            \
  }                                                                                                \
  return spans;

  int calcColumns()
  {
    if (sheetTypeValue() == SpriteSheetType::Vertical)
      return 1;
    CALC_SPANS(x, w);
  }

  int calcRows()
  {
    if (sheetTypeValue() == SpriteSheetType::Horizontal)
      return 1;
    CALC_SPANS(y, h);
  }

#undef CALC_SPANS

  // SelectBoxDelegate impleentation
  void onChangeRectangle(const gfx::Rect& rect) override
  {
    m_rect = rect;

    x()->setTextf("%d", m_rect.x);
    y()->setTextf("%d", m_rect.y);
    width()->setTextf("%d", m_rect.w);
    height()->setTextf("%d", m_rect.h);
    columns()->setTextf("%d", calcColumns());
    rows()->setTextf("%d", calcRows());
  }

  void onChangePadding(const gfx::Size& padding) override
  {
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
    columns()->setTextf("%d", calcColumns());
    rows()->setTextf("%d", calcRows());
  }

  std::string onGetContextBarHelp() override
  {
    return Strings::import_sprite_sheet_context_bar_help();
  }

private:
  void selectActiveDocument()
  {
    Doc* oldDocument = m_document;
    m_document = m_context->activeDocument();

    // If the user already have selected a file, we have to destroy
    // that file in order to select the new one.
    if (oldDocument) {
      releaseEditor();

      if (m_fileOpened) {
        try {
          DocDestroyer destroyer(m_context, oldDocument, 500);
          destroyer.destroyDocument();
        }
        catch (const LockedDocException& ex) {
          Console::showException(ex);
        }
      }
    }

    captureEditor();

    import()->setEnabled(m_document ? true : false);

    if (m_document) {
      m_docPref = &Preferences::instance().document(m_document);

      if (m_docPref->importSpriteSheet.type() >= app::SpriteSheetType::Horizontal &&
          m_docPref->importSpriteSheet.type() <= app::SpriteSheetType::Columns)
        sheetType()->setSelectedItemIndex((int)m_docPref->importSpriteSheet.type() - 1);
      else
        sheetType()->setSelectedItemIndex((int)app::SpriteSheetType::Rows - 1);

      gfx::Rect defBounds = m_docPref->importSpriteSheet.bounds();
      if (defBounds.isEmpty()) {
        if (m_document->isMaskVisible())
          defBounds = m_document->mask()->bounds();
        else
          defBounds = m_document->sprite()->gridBounds();
      }
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

  void captureEditor()
  {
    ASSERT(m_editor == NULL);

    if (m_document && !m_editor) {
      m_rect = getRectFromEntries();
      m_padding = getPaddingFromEntries();
      m_editor = Editor::activeEditor();
      m_editorState.reset(new SelectBoxState(
        this,
        m_rect,
        SelectBoxState::Flags(
          int(SelectBoxState::Flags::Rulers) | int(SelectBoxState::Flags::Grid) |
          int(SelectBoxState::Flags::DarkOutside) | int(SelectBoxState::Flags::PaddingRulers))));

      m_editor->setState(m_editorState);
      updateGridState();
    }
  }

  void updateGridState()
  {
    if (!m_editorState)
      return;

    int flags = (int)static_cast<SelectBoxState*>(m_editorState.get())->getFlags();
    flags = flags & ~((int)SelectBoxState::Flags::HGrid | (int)SelectBoxState::Flags::VGrid);
    switch (sheetTypeValue()) {
      case SpriteSheetType::Horizontal:
        flags |= int(SelectBoxState::Flags::HGrid);
        columns()->setEnabled(true);
        rows()->setEnabled(false);
        break;
      case SpriteSheetType::Vertical:
        flags |= int(SelectBoxState::Flags::VGrid);
        columns()->setEnabled(false);
        rows()->setEnabled(true);
        break;
      case SpriteSheetType::Rows:
      case SpriteSheetType::Columns:
        flags |= int(SelectBoxState::Flags::Grid);
        columns()->setEnabled(true);
        rows()->setEnabled(true);
        break;
    }

    static_cast<SelectBoxState*>(m_editorState.get())->setFlags(SelectBoxState::Flags(flags));
    m_editor->invalidate();
  }

  void releaseEditor()
  {
    if (m_editor) {
      m_editor->backToPreviousState();
      m_editor = NULL;
    }
  }

  void onPaddingEnabledChange()
  {
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

  void resize() { expandWindow(sizeHint()); }

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

class ImportSpriteSheetCommand : public CommandWithNewParams<ImportSpriteSheetParams> {
public:
  ImportSpriteSheetCommand();

protected:
  virtual void onExecute(Context* context) override;
};

ImportSpriteSheetCommand::ImportSpriteSheetCommand()
  : CommandWithNewParams(CommandId::ImportSpriteSheet(), CmdRecordableFlag)
{
}

void ImportSpriteSheetCommand::onExecute(Context* context)
{
  Doc* document;
  auto& params = this->params();

  if (context->isUIAvailable() && params.ui()) {
    ImportSpriteSheetWindow window(params, context);
    window.openWindowInForeground();
    if (!window.ok())
      return;

    document = window.document();
    if (!document)
      return;

    window.updateParams(params);

    DocumentPreferences* docPref = window.docPref();
    docPref->importSpriteSheet.type(params.type());
    docPref->importSpriteSheet.bounds(params.frameBounds());
    docPref->importSpriteSheet.partialTiles(params.partialTiles());
    docPref->importSpriteSheet.paddingBounds(params.padding());
    docPref->importSpriteSheet.paddingEnabled(window.paddingEnabledValue());
  }
  // We import the sprite sheet from the active document if there is
  // no UI.
  else {
    document = context->activeDocument();
    if (!document)
      return;

    Sprite* sprite = document->sprite();

    auto newFrameBounds = calcFrameSize(sprite->size(),
                                        params.columns(),
                                        params.rows(),
                                        params.frameBounds().origin(),
                                        params.padding());
    if (params.columns.isSet()) {
      auto fb = params.frameBounds();
      fb.w = newFrameBounds.w;
      params.frameBounds(fb);
    }

    if (params.rows.isSet()) {
      auto fb = params.frameBounds();
      fb.h = newFrameBounds.h;
      params.frameBounds(fb);
    }
  }

  // The list of frames imported from the sheet
  std::vector<ImageRef> animation;

  try {
    Sprite* sprite = document->sprite();
    frame_t currentFrame = context->activeSite().frame();
    gfx::Rect frameBounds = params.frameBounds();
    const gfx::Size padding = params.padding();
    render::Render render;
    render.setNewBlend(Preferences::instance().experimental.newBlend());

    if (frameBounds.isEmpty())
      frameBounds = sprite->bounds();

    // Each sprite in the sheet
    std::vector<gfx::Rect> tileRects;
    int widthStop = sprite->width();
    int heightStop = sprite->height();
    if (params.partialTiles()) {
      widthStop += frameBounds.w - 1;
      heightStop += frameBounds.h - 1;
    }

    switch (params.type()) {
      case app::SpriteSheetType::Horizontal:
        for (int x = frameBounds.x; x + frameBounds.w <= widthStop;
             x += frameBounds.w + padding.w) {
          tileRects.push_back(gfx::Rect(x, frameBounds.y, frameBounds.w, frameBounds.h));
        }
        break;
      case app::SpriteSheetType::Vertical:
        for (int y = frameBounds.y; y + frameBounds.h <= heightStop;
             y += frameBounds.h + padding.h) {
          tileRects.push_back(gfx::Rect(frameBounds.x, y, frameBounds.w, frameBounds.h));
        }
        break;
      case app::SpriteSheetType::Rows:
        for (int y = frameBounds.y; y + frameBounds.h <= heightStop;
             y += frameBounds.h + padding.h) {
          for (int x = frameBounds.x; x + frameBounds.w <= widthStop;
               x += frameBounds.w + padding.w) {
            tileRects.push_back(gfx::Rect(x, y, frameBounds.w, frameBounds.h));
          }
        }
        break;
      case app::SpriteSheetType::Columns:
        for (int x = frameBounds.x; x + frameBounds.w <= widthStop;
             x += frameBounds.w + padding.w) {
          for (int y = frameBounds.y; y + frameBounds.h <= heightStop;
               y += frameBounds.h + padding.h) {
            tileRects.push_back(gfx::Rect(x, y, frameBounds.w, frameBounds.h));
          }
        }
        break;
    }

    // As first step, we cut each tile and add them into "animation" list.
    for (const auto& tileRect : tileRects) {
      ImageRef resultImage(Image::create(sprite->pixelFormat(), tileRect.w, tileRect.h));

      // Render the portion of sheet.
      render.renderSprite(resultImage.get(), sprite, currentFrame, gfx::Clip(0, 0, tileRect));

      animation.push_back(resultImage);
    }

    if (animation.size() == 0) {
      Alert::show(Strings::alerts_empty_rect_importing_sprite_sheet());
      return;
    }

    // The following steps modify the sprite, so we wrap all
    // operations in a undo-transaction.
    ContextWriter writer(context);
    Tx tx(writer, Strings::import_sprite_sheet_title(), ModifyDocument);
    DocApi api = document->getApi(tx);

    // Add the layer in the sprite.
    LayerImage* resultLayer = api.newLayer(sprite->root(),
                                           Strings::import_sprite_sheet_layer_name());

    // Add all frames+cels to the new layer
    for (size_t i = 0; i < animation.size(); ++i) {
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
