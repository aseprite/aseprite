// Aseprite
// Copyright (C) 2022-2025  Igara Studio S.A.
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
#include "app/fonts/font_info.h"
#include "app/i18n/strings.h"
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
#include "text/font_metrics.h"
#include "ui/entry.h"
#include "ui/message.h"
#include "ui/paint_event.h"
#include "ui/system.h"

#ifdef LAF_SKIA
  #include "app/util/shader_helpers.h"
  #include "os/skia/skia_helpers.h"
  #include "os/skia/skia_surface.h"
#endif

#include <cmath>

namespace app {

using namespace ui;

// Get ui::Paint to render text from context bar options / preferences
static ui::Paint get_paint_for_text()
{
  ui::Paint paint;
  if (auto* app = App::instance()) {
    if (auto* ctxBar = app->contextBar())
      paint = ctxBar->fontEntry()->paint();
  }
  paint.color(color_utils::color_for_ui(Preferences::instance().colorBar.fgColor()));
  return paint;
}

static gfx::RectF calc_blob_bounds(const text::TextBlobRef& blob)
{
  gfx::RectF bounds = get_text_blob_required_bounds(blob);
  ui::Paint paint = get_paint_for_text();
  if (paint.style() == ui::Paint::Style::Stroke ||
      paint.style() == ui::Paint::Style::StrokeAndFill) {
    bounds.enlarge(std::ceil(paint.strokeWidth()));
  }
  return bounds;
}

class WritingTextState::TextEditor : public Entry {
public:
  enum TextPreview {
    Intermediate, // With selection preview / user interface
    Final,        // Final to be rendered in the cel
  };

  TextEditor(Editor* editor, const Site& site, const gfx::Rect& bounds)
    : Entry(4096, "")
    , m_editor(editor)
    , m_doc(site.document())
    , m_extraCel(new ExtraCel)
  {
    // We have to draw the editor as background of this ui::Entry.
    setTransparent(true);

    setPersistSelection(true);

    createExtraCel(site, bounds);
    renderExtraCel(TextPreview::Intermediate);

    FontInfo fontInfo = App::instance()->contextBar()->fontInfo();
    if (auto font = Fonts::instance()->fontFromInfo(fontInfo))
      setFont(font);
  }

  ~TextEditor()
  {
    m_doc->setExtraCel(ExtraCelRef(nullptr));
    m_doc->generateMaskBoundaries();
  }

  // Returns the extra cel with the text rendered (but without the
  // selected text highlighted).
  ExtraCelRef extraCel(const TextPreview textPreview)
  {
    renderExtraCel(textPreview);
    return m_extraCel;
  }

  void setExtraCelBounds(const gfx::RectF& bounds)
  {
    doc::Image* extraImg = m_extraCel->image();
    if (!extraImg || std::ceil(bounds.w) != extraImg->width() ||
        std::ceil(bounds.h) != extraImg->height()) {
      createExtraCel(m_editor->getSite(), bounds);
    }
    else {
      m_baseBounds = bounds;
      m_extraCel->cel()->setBounds(bounds);
    }
    renderExtraCel(TextPreview::Intermediate);
  }

  obs::signal<void(const gfx::RectF&)> NewRequiredBounds;

private:
  void createExtraCel(const Site& site, const gfx::Rect& bounds)
  {
    m_baseBounds = bounds;
    m_extraCel->create(ExtraCel::Purpose::TextPreview,
                       site.tilemapMode(),
                       site.sprite(),
                       bounds,
                       gfx::Size(std::ceil(bounds.w), std::ceil(bounds.h)),
                       site.frame(),
                       255);

    m_extraCel->setType(render::ExtraType::PATCH);
    m_extraCel->setBlendMode(site.layer()->isImage() ?
                               static_cast<LayerImage*>(site.layer())->blendMode() :
                               doc::BlendMode::NORMAL);
  }

  bool onProcessMessage(Message* msg) override
  {
    switch (msg->type()) {
      case kMouseDownMessage:
      case kMouseMoveMessage: {
        auto* mouseMsg = static_cast<MouseMessage*>(msg);
        // Ignore middle mouse button so we can scroll with it.
        if (mouseMsg->middle()) {
          auto* parent = this->parent();
          MouseMessage mouseMsg2(kMouseDownMessage, *mouseMsg, mouseMsg->position());
          mouseMsg2.setRecipient(parent);
          parent->sendMessage(&mouseMsg2);
          return true;
        }
        break;
      }
    }
    return Entry::onProcessMessage(msg);
  }

  float onGetTextBaseline() const override
  {
    text::FontMetrics metrics;
    font()->metrics(&metrics);
    return scale().y * -metrics.ascent;
  }

  void onInitTheme(InitThemeEvent& ev) override
  {
    Entry::onInitTheme(ev);
    setBgColor(gfx::ColorNone);
  }

  void onSetText() override
  {
    Entry::onSetText();
    onNewTextBlob();
  }

  void onSetFont() override
  {
    Entry::onSetFont();
    onNewTextBlob();
  }

  text::ShaperFeatures onGetTextShaperFeatures() const override
  {
    const FontInfo fontInfo = App::instance()->contextBar()->fontInfo();
    text::ShaperFeatures features;
    features.ligatures = fontInfo.ligatures();
    return features;
  }

  void onNewTextBlob()
  {
    text::TextBlobRef blob = textBlob();
    if (!blob)
      return;

    // Notify that we could make the text editor bigger to show this
    // text blob.
    NewRequiredBounds(calc_blob_bounds(blob));
  }

  void onPaint(PaintEvent& ev) override
  {
    Graphics* g = ev.graphics();

    // Don't paint the base Entry borders
    // Entry::onPaint(ev);

    if (!hasText())
      return;

    // Paint border
    {
      ui::Paint paint;
      paint.style(ui::Paint::Stroke);
      set_checkered_paint_mode(paint, 0, gfx::rgba(0, 0, 0, 255), gfx::rgba(255, 255, 255, 255));
      g->drawRect(clientBounds(), paint);
    }

    try {
      if (!textBlob()) {
        m_doc->setExtraCel(nullptr);
        m_editor->invalidate();
        return;
      }

      // Render extra cel with text + selected text
      renderExtraCel(TextPreview::Intermediate);
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

  void renderExtraCel(const TextPreview textPreview)
  {
    doc::Image* extraImg = m_extraCel->image();
    ASSERT(extraImg);
    if (!extraImg)
      return;

    extraImg->clear(extraImg->maskColor());

    text::TextBlobRef blob = textBlob();
    doc::ImageRef blobImage;
    gfx::RectF bounds;
    if (blob) {
      const ui::Paint paint = get_paint_for_text();
      bounds = calc_blob_bounds(blob);
      blobImage = render_text_blob(blob, bounds, get_paint_for_text());
      if (!blobImage)
        return;

      // Invert selected range in the image
      if (textPreview == TextPreview::Intermediate) {
        Range range;
        getEntryThemeInfo(nullptr, nullptr, nullptr, &range);
        if (!range.isEmpty()) {
          gfx::RectF selectedBounds = getCharBoxBounds(range.from) | getCharBoxBounds(range.to - 1);

          if (!selectedBounds.isEmpty()) {
            selectedBounds.offset(-bounds.origin());

#ifdef LAF_SKIA
            sk_sp<SkSurface> skSurface = wrap_docimage_in_sksurface(blobImage.get());
            os::SurfaceRef surface = base::make_ref<os::SkiaSurface>(skSurface);

            os::Paint paint2 = paint;
            paint2.blendMode(os::BlendMode::Xor);
            paint2.style(os::Paint::Style::Fill);
            surface->drawRect(selectedBounds, paint2);
#endif // LAF_SKIA
          }
        }
      }
    }

    doc::Cel* extraCel = m_extraCel->cel();
    ASSERT(extraCel);
    if (!extraCel)
      return;

    extraCel->setPosition(m_baseBounds.x + bounds.x, m_baseBounds.y + bounds.y);

    render::Render().renderLayer(extraImg,
                                 m_editor->layer(),
                                 m_editor->frame(),
                                 gfx::Clip(0, 0, extraCel->bounds()),
                                 doc::BlendMode::SRC);

    if (blobImage) {
      doc::blend_image(extraImg,
                       blobImage.get(),
                       gfx::Clip(blobImage->bounds().size()),
                       m_doc->sprite()->palette(m_editor->frame()),
                       255,
                       doc::BlendMode::NORMAL);
    }
  }

  Editor* m_editor;
  Doc* m_doc;
  ExtraCelRef m_extraCel;

  // Initial bounds for the entry field. This can be modified later to
  // render the text in case some initial letter/glyph needs some
  // extra room at the left side.
  gfx::Rect m_baseBounds;
};

WritingTextState::WritingTextState(Editor* editor, const gfx::Rect& bounds)
  : m_delayedMouseMove(this, editor, 5)
  , m_editor(editor)
  , m_bounds(bounds)
  , m_entry(new TextEditor(editor, editor->getSite(), bounds))
{
  m_beforeCmdConn = UIContext::instance()->BeforeCommandExecution.connect(
    &WritingTextState::onBeforeCommandExecution,
    this);

  m_fontChangeConn =
    App::instance()->contextBar()->FontChange.connect(&WritingTextState::onFontChange, this);

  m_entry->NewRequiredBounds.connect([this](const gfx::RectF& blobBounds) {
    if (m_bounds.w < blobBounds.w || m_bounds.h < blobBounds.h) {
      m_bounds.w = std::max(m_bounds.w, blobBounds.w);
      m_bounds.h = std::max(m_bounds.h, blobBounds.h);
      m_entry->setExtraCelBounds(m_bounds);
      m_entry->setBounds(calcEntryBounds());
    }
  });

  onEditorResize(editor);
}

WritingTextState::~WritingTextState()
{
}

bool WritingTextState::onMouseDown(Editor* editor, MouseMessage* msg)
{
  if (!editor->hasCapture())
    m_delayedMouseMove.reset();

  m_delayedMouseMove.onMouseDown(msg);
  m_hit = calcHit(editor, msg->position());

  if (msg->left()) {
    if (m_hit == Hit::Edges) {
      m_movingBounds = true;
      m_cursorStart = editor->screenToEditorF(msg->position());
      m_boundsOrigin = m_bounds.origin();

      editor->captureMouse();
      return true;
    }

    // On mouse down with the left button, we just drop the text
    // directly when we click outside the edges.
    drop();
    return true;
  }
  else if (msg->right()) {
    cancel();
    return true;
  }

  return StandbyState::onMouseDown(editor, msg);
}

bool WritingTextState::onMouseUp(Editor* editor, MouseMessage* msg)
{
  m_delayedMouseMove.onMouseUp(msg);

  const bool result = StandbyState::onMouseUp(editor, msg);
  if (m_movingBounds)
    m_movingBounds = false;

  // Drop if the user just clicked (so other text box is created)
  if (m_delayedMouseMove.canInterpretMouseMovementAsJustOneClick()) {
    drop();
  }

  return result;
}

bool WritingTextState::onMouseMove(Editor* editor, ui::MouseMessage* msg)
{
  m_delayedMouseMove.onMouseMove(msg);

  // Use StandbyState implementation
  return StandbyState::onMouseMove(editor, msg);
}

void WritingTextState::onCommitMouseMove(Editor* editor, const gfx::PointF& spritePos)
{
  if (!m_movingBounds)
    return;

  gfx::PointF delta(spritePos - m_cursorStart);
  if (delta.x == 0 && delta.y == 0)
    return;

  m_bounds.setOrigin(delta + m_boundsOrigin);
  m_entry->setExtraCelBounds(m_bounds);
  m_entry->setBounds(calcEntryBounds());
}

bool WritingTextState::onSetCursor(Editor* editor, const gfx::Point& mouseScreenPos)
{
  if (calcHit(editor, mouseScreenPos) == Hit::Edges) {
    editor->showMouseCursor(kMoveCursor);
    return true;
  }

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
  const gfx::PointF scale(editor->projection().scaleX(), editor->projection().scaleY());
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
      ExtraCelRef extraCel = m_entry->extraCel(TextEditor::Final);
      Tx tx(site.document(), Strings::tools_text());
      ExpandCelCanvas expand(site, site.layer(), TiledMode::NONE, tx, ExpandCelCanvas::None);

      expand.validateDestCanvas(gfx::Region(extraCel->cel()->bounds()));

      expand.getDestCanvas()->copy(
        extraCel->image(),
        gfx::Clip(extraCel->cel()->position(), extraCel->image()->bounds()));

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
  if ( // Undo/Redo/Cancel will cancel this state
    ev.command()->id() == CommandId::Undo() || ev.command()->id() == CommandId::Redo() ||
    ev.command()->id() == CommandId::Cancel()) {
    cancel();
  }
  else {
    drop();
  }
}

void WritingTextState::onFontChange(const FontInfo& fontInfo, FontEntry::From fromField)
{
  if (auto font = Fonts::instance()->fontFromInfo(fontInfo)) {
    m_entry->setFont(font);
    m_entry->invalidate();
    m_editor->invalidate();

    // This is useful to show changes to the anti-alias option
    // immediately.
    auto dummy = m_entry->extraCel(TextEditor::Intermediate);

    if (fromField == FontEntry::From::Popup) {
      if (m_entry)
        m_entry->requestFocus();
    }
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

WritingTextState::Hit WritingTextState::calcHit(Editor* editor, const gfx::Point& mouseScreenPos)
{
  auto edges = editor->editorToScreen(m_bounds);
  if (!edges.contains(mouseScreenPos) && edges.enlarge(32 * guiscale()).contains(mouseScreenPos)) {
    return Hit::Edges;
  }

  return Hit::Normal;
}

} // namespace app
