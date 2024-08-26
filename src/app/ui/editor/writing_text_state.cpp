// Aseprite
// Copyright (C) 2022-2024  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/editor/writing_text_state.h"

#include "app/app.h"
#include "app/color_utils.h"
#include "app/commands/command.h"
#include "app/extra_cel.h"
#include "app/font_info.h"
#include "app/pref/preferences.h"
#include "app/site.h"
#include "app/tx.h"
#include "app/ui/context_bar.h"
#include "app/ui/editor/editor.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/status_bar.h"
#include "app/ui_context.h"
#include "app/util/expand_cel_canvas.h"
#include "app/util/render_text.h"
#include "doc/blend_image.h"
#include "doc/blend_internals.h"
#include "doc/layer.h"
#include "render/dithering.h"
#include "render/quantization.h"
#include "render/render.h"
#include "ui/entry.h"
#include "ui/message.h"
#include "ui/paint_event.h"
#include "ui/system.h"

#ifdef LAF_SKIA
  #include "app/util/shader_helpers.h"
  #include "os/skia/skia_helpers.h"
  #include "os/skia/skia_surface.h"
#endif

namespace app {

using namespace ui;

class WritingTextState::TextEditor : public Entry {
public:
  TextEditor(Editor* editor,
             const Site& site,
             const gfx::Rect& bounds)
    : Entry(4096, "")
    , m_editor(editor)
    , m_doc(site.document())
    , m_extraCel(new ExtraCel) {
    // We have to draw the editor as background of this ui::Entry.
    setTransparent(true);

    setPersistSelection(true);

    // TODO move this opacity() to Site class
    int t, opacity = (site.layer()->isImage() ?
                      static_cast<LayerImage*>(site.layer())->opacity(): 255);
    Cel* cel = site.cel();
    if (cel) opacity = MUL_UN8(opacity, cel->opacity(), t);

    m_extraCel->create(
      site.tilemapMode(),
      site.sprite(),
      bounds,
      bounds.size(),
      site.frame(),
      255);

    m_extraCel->setType(render::ExtraType::PATCH);
    m_extraCel->setBlendMode(site.layer()->isImage() ?
                             static_cast<LayerImage*>(site.layer())->blendMode():
                             doc::BlendMode::NORMAL);

    renderExtraCelBase();

    FontInfo fontInfo = App::instance()->contextBar()->fontInfo();
    if (auto font = get_font_from_info(fontInfo))
      setFont(font);
  }

  ~TextEditor() {
    m_doc->setExtraCel(ExtraCelRef(nullptr));
    m_doc->generateMaskBoundaries();
  }

  // Returns the extra cel with the text rendered (but without the
  // selected text highlighted).
  ExtraCelRef extraCel() {
    renderExtraCelBase();
    renderExtraCelText(false);
    return m_extraCel;
  }

private:
  bool onProcessMessage(Message* msg) override {
    switch (msg->type()) {
      case kMouseDownMessage:
      case kMouseMoveMessage: {
        auto* mouseMsg = static_cast<MouseMessage*>(msg);
        // Ignore middle mouse button so we can scroll with it.
        if (mouseMsg->middle()) {
          auto* parent = this->parent();
          MouseMessage mouseMsg2(kMouseDownMessage,
                                 *mouseMsg,
                                 mouseMsg->position());
          mouseMsg2.setRecipient(parent);
          parent->sendMessage(&mouseMsg2);
          return true;
        }
        break;
      }
    }
    return Entry::onProcessMessage(msg);
  }

  void onInitTheme(InitThemeEvent& ev) override {
    Entry::onInitTheme(ev);
    setBgColor(gfx::ColorNone);
  }

  void onPaint(PaintEvent& ev) override {
    Graphics* g = ev.graphics();

    // Don't paint the base Entry borders
    //Entry::onPaint(ev);

    if (!hasText())
      return;

    // Paint border
    {
      ui::Paint paint;
      paint.style(ui::Paint::Stroke);
      set_checkered_paint_mode(paint, 0,
                               gfx::rgba(0, 0, 0, 255),
                               gfx::rgba(255, 255, 255, 255));
      g->drawRect(clientBounds(), paint);
    }

    try {
      if (!textBlob()) {
        m_doc->setExtraCel(nullptr);
        m_editor->invalidate();
        return;
      }

      // Render extra cel with text + selected text
      renderExtraCelBase();
      renderExtraCelText(true);
      m_doc->setExtraCel(m_extraCel);

      // Paint caret
      if (isCaretVisible()) {
        int scroll, caret;
        getEntryThemeInfo(&scroll, &caret, nullptr, nullptr);

        gfx::RectF caretBounds = getCharBoxBounds(caret);
        caretBounds *= gfx::SizeF(scale());
        caretBounds.w = 1;
        g->fillRect(gfx::rgba(0, 0, 0), caretBounds);
      }

      m_editor->invalidate();
    }
    catch (const std::exception& e) {
      StatusBar::instance()->showTip(500, std::string(e.what()));
    }
  }

  void renderExtraCelBase() {
    auto extraImg = m_extraCel->image();
    extraImg->clear(extraImg->maskColor());
    render::Render().renderLayer(
      extraImg,
      m_editor->layer(),
      m_editor->frame(),
      gfx::Clip(0, 0, m_extraCel->cel()->bounds()),
      doc::BlendMode::SRC);
  }

  void renderExtraCelText(const bool withSelection) {
    const auto textColor =
      color_utils::color_for_image(
        Preferences::instance().colorBar.fgColor(),
        IMAGE_RGB);

    text::TextBlobRef blob = textBlob();
    if (!blob)
      return;

    doc::ImageRef image = render_text_blob(blob, textColor);
    if (!image)
      return;

    // Invert selected range in the image
    if (withSelection) {
      Range range;
      getEntryThemeInfo(nullptr, nullptr, nullptr, &range);
      if (!range.isEmpty()) {
        gfx::RectF selectedBounds =
          getCharBoxBounds(range.from) |
          getCharBoxBounds(range.to-1);

        if (!selectedBounds.isEmpty()) {
#ifdef LAF_SKIA
          sk_sp<SkSurface> skSurface = wrap_docimage_in_sksurface(image.get());
          os::SurfaceRef surface = base::make_ref<os::SkiaSurface>(skSurface);

          os::Paint paint;
          paint.blendMode(os::BlendMode::Xor);
          paint.color(textColor);
          surface->drawRect(selectedBounds, paint);
#endif // LAF_SKIA
        }
      }
    }

    doc::blend_image(
      m_extraCel->image(), image.get(),
      gfx::Clip(image->bounds().size()),
      m_doc->sprite()->palette(m_editor->frame()),
      255, doc::BlendMode::NORMAL);
  }

  Editor* m_editor;
  Doc* m_doc;
  ExtraCelRef m_extraCel;
};

WritingTextState::WritingTextState(Editor* editor,
                                   const gfx::Rect& bounds)
  : m_editor(editor)
  , m_bounds(bounds)
  , m_entry(new TextEditor(editor, editor->getSite(), bounds))
{
  m_beforeCmdConn =
    UIContext::instance()->BeforeCommandExecution.connect(
      &WritingTextState::onBeforeCommandExecution, this);

  m_fontChangeConn =
    App::instance()->contextBar()->FontChange.connect(
      &WritingTextState::onFontChange, this);

  onEditorResize(editor);
}

WritingTextState::~WritingTextState()
{
}

bool WritingTextState::onMouseDown(Editor* editor, MouseMessage* msg)
{
  if (msg->left()) {
    drop();
  }
  else if (msg->right()) {
    cancel();
    return true;
  }

  return StandbyState::onMouseDown(editor, msg);
}

bool WritingTextState::onMouseUp(Editor* editor, MouseMessage* msg)
{
  const bool result = StandbyState::onMouseUp(editor, msg);
  if (msg->middle()) {
    if (m_entry)
      m_entry->requestFocus();
  }
  return result;
}

bool WritingTextState::onSetCursor(Editor* editor, const gfx::Point&)
{
  editor->showMouseCursor(kArrowCursor);
  return true;
}

bool WritingTextState::onKeyDown(Editor*, KeyMessage*)
{
  return false;
}

bool WritingTextState::onKeyUp(Editor*, KeyMessage* msg)
{
  // Cancel loop pressing Esc key
  if (msg->scancode() == ui::kKeyEsc) {
    cancel();
  }
  // Drop text pressing Enter key
  else if (msg->scancode() == ui::kKeyEnter) {
    drop();
  }
  return true;
}

void WritingTextState::onEditorGotFocus(Editor* editor)
{
  // Focus the entry when we focus the editor, it happens when we
  // change the font settings, so we keep the focus in the entry
  // field.
  if (m_entry)
    m_entry->requestFocus();
}

void WritingTextState::onEditorResize(Editor* editor)
{
  const gfx::PointF scale(editor->projection().scaleX(),
                          editor->projection().scaleY());
  m_entry->setScale(scale);
  m_entry->setBounds(calcEntryBounds());
}

gfx::Rect WritingTextState::calcEntryBounds()
{
  const View* view = View::getView(m_editor);
  const gfx::Rect vp = view->viewportBounds();
  const gfx::Point scroll = view->viewScroll();
  const auto& padding = m_editor->padding();
  const auto& proj = m_editor->projection();
  gfx::Point pt1(m_bounds.origin());
  gfx::Point pt2(m_bounds.point2());
  pt1.x = vp.x - scroll.x + padding.x + proj.applyX(pt1.x);
  pt1.y = vp.y - scroll.y + padding.y + proj.applyY(pt1.y);
  pt2.x = vp.x - scroll.x + padding.x + proj.applyX(pt2.x);
  pt2.y = vp.y - scroll.y + padding.y + proj.applyY(pt2.y);
  return gfx::Rect(pt1, pt2);
}

void WritingTextState::onEnterState(Editor* editor)
{
  StandbyState::onEnterState(editor);

  editor->invalidate();

  editor->addChild(m_entry.get());
  m_entry->requestFocus();
}

EditorState::LeaveAction WritingTextState::onLeaveState(Editor* editor, EditorState* newState)
{
  if (!newState || !newState->isTemporalState()) {
    if (!m_discarded) {
      // Paints the text in the active layer/sprite creating an
      // undoable transaction.
      Site site = m_editor->getSite();
      ExtraCelRef extraCel = m_entry->extraCel();
      Tx tx(site.document(), "Text Tool");
      ExpandCelCanvas expand(
        site, site.layer(),
        TiledMode::NONE, tx,
        ExpandCelCanvas::None);

      expand.validateDestCanvas(
        gfx::Region(extraCel->cel()->bounds()));

      doc::blend_image(
        expand.getDestCanvas(),
        extraCel->image(),
        gfx::Clip(extraCel->cel()->position(),
                  extraCel->image()->bounds()),
        site.palette(),
        255, doc::BlendMode::NORMAL);

      expand.commit();
      tx.commit();
    }
    m_editor->releaseMouse();
    m_editor->document()->notifyGeneralUpdate();
    return DiscardState;
  }

  editor->releaseMouse();
  return KeepState;
}

void WritingTextState::onBeforePopState(Editor* editor)
{
  editor->removeChild(m_entry.get());
  m_beforeCmdConn.disconnect();
  m_fontChangeConn.disconnect();

  StandbyState::onBeforePopState(editor);
}

void WritingTextState::onBeforeCommandExecution(CommandExecutionEvent& ev)
{
  if (// Undo/Redo/Cancel will cancel this state
      ev.command()->id() == CommandId::Undo() ||
      ev.command()->id() == CommandId::Redo() ||
      ev.command()->id() == CommandId::Cancel()) {
    cancel();
  }
  else {
    drop();
  }
}

void WritingTextState::onFontChange()
{
  const FontInfo fontInfo = App::instance()->contextBar()->fontInfo();
  if (auto font = get_font_from_info(fontInfo)) {
    m_entry->setFont(font);
    m_entry->invalidate();
    m_editor->invalidate();

    // This is useful to show changes to the anti-alias option
    // immediately.
    auto dummy = m_entry->extraCel();

    ui::execute_from_ui_thread([this]{
      if (m_entry)
        m_entry->requestFocus();
    });
  }
}

void WritingTextState::cancel()
{
  m_discarded = true;

  m_editor->backToPreviousState();
  m_editor->invalidate();
}

void WritingTextState::drop()
{
  m_editor->backToPreviousState();
  m_editor->invalidate();
}

} // namespace app
