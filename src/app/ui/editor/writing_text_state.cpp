// Aseprite
// Copyright (C) 2022-2026  Igara Studio S.A.
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
#include "text/draw_text.h"
#include "text/font_metrics.h"
#include "ui/message.h"
#include "ui/paint_event.h"
#include "ui/system.h"
#include "ui/textedit.h"

#ifdef LAF_SKIA
  #include "app/util/shader_helpers.h"
  #include "os/skia/skia_helpers.h"
  #include "os/skia/skia_surface.h"
#endif

#include <cmath>
#include <limits>

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

class WritingTextState::TextEditor : public TextEdit {
public:
  enum TextPreview {
    Intermediate, // With selection preview / user interface
    Final,        // Final to be rendered in the cel
  };

  TextEditor(Editor* editor,
             const Site& site,
             const gfx::Rect& bounds,
             const std::string& initialText = std::string())
    : m_editor(editor)
    , m_doc(site.document())
    , m_extraCel(new ExtraCel)
  {
    // We have to draw the editor as background of this ui::TextEdit.
    setTransparent(true);

    createExtraCel(site, bounds);
    renderExtraCel(TextPreview::Intermediate);

    FontInfo fontInfo = App::instance()->contextBar()->fontInfo();
    if (auto font = Fonts::instance()->fontFromInfo(fontInfo))
      setFont(font);

    if (!initialText.empty()) {
      setText(initialText);
    }
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

  void invalidateTextCache() { m_textDirty = true; }

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
      case kMouseWheelMessage: {
        auto* mouseMsg = static_cast<MouseMessage*>(msg);
        if (m_editor->getState()->onMouseWheel(m_editor, mouseMsg))
          return true;
        break;
      }
      case kTouchMagnifyMessage: {
        auto* touchMsg = static_cast<TouchMessage*>(msg);
        if (m_editor->getState()->onTouchMagnify(m_editor, touchMsg))
          return true;
        break;
      }
    }
    return TextEdit::onProcessMessage(msg);
  }

  float onGetTextBaseline() const override
  {
    text::FontMetrics metrics;
    font()->metrics(&metrics);
    return scale().y * -metrics.ascent;
  }

  void onInitTheme(InitThemeEvent& ev) override
  {
    TextEdit::onInitTheme(ev);
    setBgColor(gfx::ColorNone);
  }

  void onSetText() override
  {
    TextEdit::onSetText();
    invalidateTextCache();
    onNewTextBlob();
  }

  void onSetFont() override
  {
    TextEdit::onSetFont();
    invalidateTextCache();
    onNewTextBlob();
  }

  void onTextChanged() override
  {
    TextEdit::onTextChanged();
    invalidateTextCache();
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
    if (lines().empty())
      return;

    // Notify that we could make the text editor bigger to show this
    // multi-line text.
    const gfx::RectF bounds = calcMultiLineBounds();
    if (!bounds.isEmpty())
      NewRequiredBounds(bounds);
  }

  Caret caretFromPosition(const gfx::Point& position) override
  {
    if (lines().empty())
      return Caret(&lines(), 0, 0);

    // Check if position is within widget bounds (screen coordinates)
    if (!bounds().contains(position)) {
      if (position.y < bounds().y)
        return Caret(&lines(), 0, 0);
      if (position.y > bounds().y2())
        return Caret(&lines(), lines().size() - 1, lines().back().glyphCount);
      return Caret();
    }

    gfx::PointF localPos(position - bounds().origin());
    gfx::PointF offsetPosition(localPos.x / scale().x, localPos.y / scale().y);
    Caret newCaret(&lines());

    // Check if the position is below all lines
    if (offsetPosition.y >= maxHeight()) {
      newCaret.setLine(lines().size() - 1);
      newCaret.setPos(
        (offsetPosition.x > newCaret.lineObj().width / 2) ? newCaret.lineObj().glyphCount : 0);
      return newCaret;
    }

    int lineStartY = 0;
    for (const Line& line : lines()) {
      const int lineEndY = lineStartY + line.height;
      if (offsetPosition.y < lineStartY || offsetPosition.y >= lineEndY) {
        lineStartY = lineEndY;
        continue;
      }
      newCaret.setLine(line.i);
      if (!line.blob)
        break;
      if (offsetPosition.x > line.width) {
        newCaret.setPos(line.glyphCount);
        break;
      }

      int advance = 0;
      int best = 0;
      float bestDiff = std::numeric_limits<float>::max();
      float bestGlyphCenterX = 0;
      line.blob->visitRuns([&](text::TextBlob::RunInfo& run) {
        for (size_t i = 0; i < run.glyphCount; ++i) {
          gfx::RectF glyphBounds = run.getGlyphBounds(i);
          const float centerX = glyphBounds.center().x;
          const float diff = std::fabs(centerX - offsetPosition.x);
          if (diff < bestDiff) {
            best = advance;
            bestDiff = diff;
            bestGlyphCenterX = centerX;
          }
          ++advance;
        }
      });

      // Round the caret position according the glyph center
      if (offsetPosition.x > bestGlyphCenterX)
        newCaret.setPos(best + 1);
      else
        newCaret.setPos(best);
      break;
    }

    return newCaret;
  }

  void onPaint(PaintEvent& ev) override
  {
    Graphics* g = ev.graphics();

    // Don't paint the base TextEdit
    // TextEdit::onPaint(ev);

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
        gfx::RectF caretBounds = getCaretBoundsForPaint();
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

  gfx::RectF getCaretBoundsForPaint() const
  {
    if (lines().empty())
      return gfx::RectF();

    const Line& line = caret().lineObj();
    float x = 0;

    if (caret().inEol())
      x = line.width;
    else if (caret().pos() > 0)
      x = line.getBounds(caret().pos()).x;

    // Calculate y position by summing heights of previous lines
    float y = 0;
    for (const auto& l : lines()) {
      if (l.i >= caret().line())
        break;
      y += l.height;
    }

    return gfx::RectF(x, y, 1, line.height);
  }

  void drawSelectionRects(os::Surface* surface, const os::Paint& paint) const
  {
    if (selection().isEmpty() || lines().empty())
      return;

    const Caret& start = selection().start();
    const Caret& end = selection().end();
    float yOffset = 0;
    for (const auto& l : lines()) {
      if (l.i >= start.line())
        break;
      yOffset += l.height;
    }
    for (int lineIdx = start.line(); lineIdx <= end.line(); ++lineIdx) {
      const Line& line = lines()[lineIdx];

      float startX = 0;
      float endX = line.width;

      if (lineIdx == start.line() && start.pos() > 0)
        startX = line.getBounds(start.pos()).x;

      if (lineIdx == end.line()) {
        if (end.inEol())
          endX = line.width;
        else if (end.pos() > 0)
          endX = line.getBounds(end.pos()).x;
        else
          endX = 0;
      }

      const gfx::RectF lineRect(startX, yOffset, endX - startX, line.height);
      if (!lineRect.isEmpty())
        surface->drawRect(lineRect, paint);

      yOffset += line.height;
    }
  }

  void renderExtraCel(const TextPreview textPreview)
  {
    doc::Image* extraImg = m_extraCel->image();
    ASSERT(extraImg);
    if (!extraImg)
      return;

    extraImg->clear(extraImg->maskColor());

    if (lines().empty() || !hasText()) {
      m_cachedTextImage.reset();
      return;
    }
    if (m_textDirty || !m_cachedTextImage)
      renderTextToCache();
    if (!m_cachedTextImage)
      return;

    doc::Cel* extraCel = m_extraCel->cel();
    ASSERT(extraCel);
    if (!extraCel)
      return;

    extraCel->setPosition(m_baseBounds.x + m_cachedTextBounds.x,
                          m_baseBounds.y + m_cachedTextBounds.y);

    render::Render().renderLayer(extraImg,
                                 m_editor->layer(),
                                 m_editor->frame(),
                                 gfx::Clip(0, 0, extraCel->bounds()),
                                 doc::BlendMode::SRC);

    if (textPreview == TextPreview::Intermediate && !selection().isEmpty()) {
      doc::ImageRef finalImage(doc::Image::createCopy(m_cachedTextImage.get()));
#ifdef LAF_SKIA
      sk_sp<SkSurface> skSurface = wrap_docimage_in_sksurface(finalImage.get());
      os::SurfaceRef surface = base::make_ref<os::SkiaSurface>(skSurface);

      os::Paint paint2 = get_paint_for_text();
      paint2.blendMode(os::BlendMode::Xor);
      paint2.style(os::Paint::Style::Fill);
      drawSelectionRects(surface.get(), paint2);
#endif
      doc::blend_image(extraImg,
                       finalImage.get(),
                       gfx::Clip(finalImage->bounds().size()),
                       m_doc->sprite()->palette(m_editor->frame()),
                       255,
                       doc::BlendMode::NORMAL);
    }
    else {
      doc::blend_image(extraImg,
                       m_cachedTextImage.get(),
                       gfx::Clip(m_cachedTextImage->bounds().size()),
                       m_doc->sprite()->palette(m_editor->frame()),
                       255,
                       doc::BlendMode::NORMAL);
    }
  }

  // Render text to cached image (called only when text content changes)
  void renderTextToCache()
  {
    m_cachedTextBounds = calcMultiLineBounds();
    if (m_cachedTextBounds.isEmpty()) {
      m_cachedTextImage.reset();
      return;
    }

    m_cachedTextImage.reset(doc::Image::create(doc::IMAGE_RGB,
                                               std::ceil(m_cachedTextBounds.w),
                                               std::ceil(m_cachedTextBounds.h)));
    m_cachedTextImage->clear(m_cachedTextImage->maskColor());

#ifdef LAF_SKIA
    sk_sp<SkSurface> skSurface = wrap_docimage_in_sksurface(m_cachedTextImage.get());
    os::SurfaceRef surface = base::make_ref<os::SkiaSurface>(skSurface);

    const ui::Paint paint = get_paint_for_text();

    float yOffset = 0;
    for (const auto& line : lines()) {
      if (line.blob) {
        gfx::RectF lineBlobBounds = calc_blob_bounds(line.blob);
        gfx::PointF drawPos(-lineBlobBounds.x, yOffset - lineBlobBounds.y);
        text::draw_text(surface.get(), line.blob, drawPos, &paint);
      }
      yOffset += line.height;
    }
#endif // LAF_SKIA

    m_textDirty = false;
  }

  gfx::RectF calcMultiLineBounds() const
  {
    if (lines().empty())
      return gfx::RectF();

    float maxWidth = 0;
    float totalHeight = 0;
    float minX = 0;
    float minY = 0;
    bool first = true;

    for (const auto& line : lines()) {
      if (line.blob) {
        gfx::RectF lineBounds = calc_blob_bounds(line.blob);
        if (first) {
          minX = lineBounds.x;
          minY = lineBounds.y;
          first = false;
        }
        else {
          minX = std::min(minX, lineBounds.x);
        }
        maxWidth = std::max(maxWidth, lineBounds.w);
      }
      else {
        maxWidth = std::max(maxWidth, 1.0f);
      }
      totalHeight += line.height;
    }

    if (maxWidth < 1)
      maxWidth = 1;
    if (totalHeight < 1)
      totalHeight = 1;

    return gfx::RectF(minX, minY, maxWidth, totalHeight);
  }

  Editor* m_editor;
  Doc* m_doc;
  ExtraCelRef m_extraCel;

  // Initial bounds for the entry field. This can be modified later to
  // render the text in case some initial letter/glyph needs some
  // extra room at the left side.
  gfx::Rect m_baseBounds;

  // Cached rendered text image to avoid re-rendering on position changes
  doc::ImageRef m_cachedTextImage;
  gfx::RectF m_cachedTextBounds;
  bool m_textDirty = true;
};

WritingTextState::WritingTextState(Editor* editor,
                                   const gfx::Rect& bounds,
                                   const std::string& initialText)
  : m_delayedMouseMove(this, editor, 5)
  , m_editor(editor)
  , m_bounds(bounds)
  , m_textEdit(new TextEditor(editor, editor->getSite(), bounds, initialText))
{
  m_beforeCmdConn = UIContext::instance()->BeforeCommandExecution.connect(
    &WritingTextState::onBeforeCommandExecution,
    this);

  m_fontChangeConn =
    App::instance()->contextBar()->FontChange.connect(&WritingTextState::onFontChange, this);

  m_fgColorConn = Preferences::instance().colorBar.fgColor.AfterChange.connect([this] {
    m_textEdit->invalidateTextCache();
    m_textEdit->invalidate();
    m_editor->invalidate();
  });

  m_textEdit->NewRequiredBounds.connect([this](const gfx::RectF& blobBounds) {
    if (m_bounds.w < blobBounds.w || m_bounds.h < blobBounds.h) {
      m_bounds.w = std::max(m_bounds.w, blobBounds.w);
      m_bounds.h = std::max(m_bounds.h, blobBounds.h);
      m_textEdit->setExtraCelBounds(m_bounds);
      m_textEdit->setBounds(calcEntryBounds());
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
    if (m_hit == Hit::Inside)
      return false;

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

  // Drop if the user just clicked outside the text area (so other text box is created)
  // Don't drop if clicked inside - that's for caret positioning
  if (m_hit != Hit::Inside && m_delayedMouseMove.canInterpretMouseMovementAsJustOneClick()) {
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
  m_textEdit->setExtraCelBounds(m_bounds);
  m_textEdit->setBounds(calcEntryBounds());
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

bool WritingTextState::onKeyDown(Editor*, KeyMessage* msg)
{
  // Cancel loop pressing Esc key
  if (msg->scancode() == ui::kKeyEsc) {
    cancel();
    return true;
  }
  return false;
}

bool WritingTextState::onKeyUp(Editor*, KeyMessage* msg)
{
  // Note: We cannot process kKeyEnter key here to drop the text as it
  // could be received after the Enter key is pressed in the IME
  // dialog to accept the composition (not to accept the text). So we
  // process kKeyEnter in onKeyDown().
  return true;
}

void WritingTextState::onEditorGotFocus(Editor* editor)
{
  // Focus the entry when we focus the editor, it happens when we
  // change the font settings, so we keep the focus in the entry
  // field.
  if (m_textEdit)
    m_textEdit->requestFocus();
}

void WritingTextState::onEditorResize(Editor* editor)
{
  const gfx::PointF scale(editor->projection().scaleX(), editor->projection().scaleY());
  m_textEdit->setScale(scale);
  m_textEdit->setBounds(calcEntryBounds());
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

  editor->addChild(m_textEdit.get());
  m_textEdit->requestFocus();
}

EditorState::LeaveAction WritingTextState::onLeaveState(Editor* editor, EditorState* newState)
{
  if (!newState || !newState->isTemporalState()) {
    if (!m_discarded) {
      // Paints the text in the active layer/sprite creating an
      // undoable transaction.
      Site site = m_editor->getSite();
      ExtraCelRef extraCel = m_textEdit->extraCel(TextEditor::Final);
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
  editor->removeChild(m_textEdit.get());
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
    m_textEdit->setFont(font);
    m_textEdit->invalidate();
    m_editor->invalidate();

    // This is useful to show changes to the anti-alias option
    // immediately.
    auto dummy = m_textEdit->extraCel(TextEditor::Intermediate);

    if (fromField == FontEntry::From::Popup) {
      if (m_textEdit)
        m_textEdit->requestFocus();
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
  if (edges.contains(mouseScreenPos))
    return Hit::Inside;
  if (edges.enlarge(32 * guiscale()).contains(mouseScreenPos))
    return Hit::Edges;
  return Hit::Outside;
}

} // namespace app
