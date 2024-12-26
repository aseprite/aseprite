// Aseprite
// Copyright (C) 2018-2024  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/ui/doc_view.h"

#include "app/app.h"
#include "app/app_menus.h"
#include "app/cmd/clear_mask.h"
#include "app/cmd/deselect_mask.h"
#include "app/cmd/trim_cel.h"
#include "app/commands/commands.h"
#include "app/console.h"
#include "app/context_access.h"
#include "app/doc_access.h"
#include "app/doc_event.h"
#include "app/i18n/strings.h"
#include "app/modules/palettes.h"
#include "app/pref/preferences.h"
#include "app/tx.h"
#include "app/ui/editor/editor.h"
#include "app/ui/editor/editor_customization_delegate.h"
#include "app/ui/editor/editor_view.h"
#include "app/ui/editor/navigate_state.h"
#include "app/ui/keyboard_shortcuts.h"
#include "app/ui/main_window.h"
#include "app/ui/status_bar.h"
#include "app/ui/timeline/timeline.h"
#include "app/ui/workspace.h"
#include "app/ui_context.h"
#include "app/util/clipboard.h"
#include "app/util/range_utils.h"
#include "base/fs.h"
#include "doc/color.h"
#include "doc/layer.h"
#include "doc/sprite.h"
#include "fmt/format.h"
#include "ui/accelerator.h"
#include "ui/alert.h"
#include "ui/menu.h"
#include "ui/message.h"
#include "ui/system.h"
#include "ui/view.h"

#include <typeinfo>

namespace app {

using namespace ui;

namespace {

// Used to show a view temporarily (the one with the file to be
// closed) and restore the previous view. E.g. When we close the
// non-active sprite pressing the cross button in a sprite tab.
class SetRestoreDocView {
public:
  SetRestoreDocView(UIContext* ctx, DocView* newView) : m_ctx(ctx), m_oldView(ctx->activeView())
  {
    if (newView != m_oldView)
      m_ctx->setActiveView(newView);
    else
      m_oldView = nullptr;
  }

  ~SetRestoreDocView()
  {
    if (m_oldView)
      m_ctx->setActiveView(m_oldView);
  }

private:
  UIContext* m_ctx;
  DocView* m_oldView;
};

class AppEditor : public Editor,
                  public EditorObserver,
                  public EditorCustomizationDelegate {
public:
  AppEditor(Doc* document, DocViewPreviewDelegate* previewDelegate)
    : Editor(document)
    , m_previewDelegate(previewDelegate)
  {
    add_observer(this);
    setCustomizationDelegate(this);
  }

  ~AppEditor()
  {
    remove_observer(this);
    setCustomizationDelegate(NULL);
  }

  // EditorObserver implementation
  void dispose() override { m_previewDelegate->onDisposeOtherEditor(this); }

  void onScrollChanged(Editor* editor) override
  {
    m_previewDelegate->onScrollOtherEditor(this);

    if (isActive())
      StatusBar::instance()->updateFromEditor(this);
  }

  void onAfterFrameChanged(Editor* editor) override
  {
    m_previewDelegate->onPreviewOtherEditor(this);

    if (isActive())
      set_current_palette(editor->sprite()->palette(editor->frame()), false);
  }

  void onAfterLayerChanged(Editor* editor) override
  {
    m_previewDelegate->onPreviewOtherEditor(this);
  }

  // EditorCustomizationDelegate implementation
  tools::Tool* getQuickTool(tools::Tool* currentTool) override
  {
    return KeyboardShortcuts::instance()->getCurrentQuicktool(currentTool);
  }

  KeyAction getPressedKeyAction(KeyContext context) override
  {
    return KeyboardShortcuts::instance()->getCurrentActionModifiers(context);
  }

  TagProvider* getTagProvider() override { return App::instance()->mainWindow()->getTimeline(); }

protected:
  bool onProcessMessage(Message* msg) override
  {
    switch (msg->type()) {
      case kKeyDownMessage:
      case kKeyUpMessage:
        if (static_cast<KeyMessage*>(msg)->repeat() == 0) {
          KeyboardShortcuts* keys = KeyboardShortcuts::instance();
          KeyPtr lmb = keys->action(KeyAction::LeftMouseButton, KeyContext::Any);
          KeyPtr rmb = keys->action(KeyAction::RightMouseButton, KeyContext::Any);

          // Convert action keys into mouse messages.
          if (lmb->isPressed(msg, *keys) || rmb->isPressed(msg, *keys)) {
            MouseMessage mouseMsg(
              (msg->type() == kKeyDownMessage ? kMouseDownMessage : kMouseUpMessage),
              PointerType::Unknown,
              (lmb->isPressed(msg, *keys) ? kButtonLeft : kButtonRight),
              msg->modifiers(),
              mousePosInDisplay());

            sendMessage(&mouseMsg);
            return true;
          }
        }
        break;
    }

    try {
      return Editor::onProcessMessage(msg);
    }
    catch (const std::exception& ex) {
      showUnhandledException(ex, msg);
      return false;
    }
  }

private:
  DocViewPreviewDelegate* m_previewDelegate;
};

class PreviewEditor : public Editor,
                      public EditorCustomizationDelegate {
public:
  PreviewEditor(Doc* document)
    : Editor(document,
             Editor::kShowOutside, // Don't show grid/mask in preview preview
             std::make_shared<NavigateState>())
  {
    setCustomizationDelegate(this);
  }

  ~PreviewEditor()
  {
    // As we are destroying this instance, we have to remove it as the
    // customization delegate. Editor::~Editor() will call
    // setCustomizationDelegate(nullptr) too which triggers a
    // EditorCustomizationDelegate::dispose() if the customization
    // isn't nullptr.
    setCustomizationDelegate(nullptr);
  }

  // EditorCustomizationDelegate implementation
  void dispose() override
  {
    // Do nothing
  }

  tools::Tool* getQuickTool(tools::Tool* currentTool) override { return nullptr; }

  KeyAction getPressedKeyAction(KeyContext context) override { return KeyAction::None; }

  TagProvider* getTagProvider() override { return App::instance()->mainWindow()->getTimeline(); }
};

} // anonymous namespace

DocView::DocView(Doc* document, Type type, DocViewPreviewDelegate* previewDelegate)
  : Box(VERTICAL)
  , m_type(type)
  , m_document(document)
  , m_view(
      new EditorView(type == Normal ? EditorView::CurrentEditorMode : EditorView::AlwaysSelected))
  , m_previewDelegate(previewDelegate)
  , m_editor((type == Normal ? (Editor*)new AppEditor(document, previewDelegate) :
                               (Editor*)new PreviewEditor(document)))
{
  addChild(m_view);

  m_view->attachToView(m_editor);
  m_view->setExpansive(true);

  m_editor->setDocView(this);
  m_document->add_observer(this);
}

DocView::~DocView()
{
  m_document->remove_observer(this);
  delete m_editor;
}

void DocView::getSite(Site* site) const
{
  m_editor->getSite(site);
}

std::string DocView::getTabText()
{
  return m_document->name();
}

TabIcon DocView::getTabIcon()
{
  return TabIcon::NONE;
}

gfx::Color DocView::getTabColor()
{
  color_t c = m_editor->sprite()->userData().color();
  return gfx::rgba(doc::rgba_getr(c), doc::rgba_getg(c), doc::rgba_getb(c), doc::rgba_geta(c));
}

WorkspaceView* DocView::cloneWorkspaceView()
{
  return new DocView(m_document, Normal, m_previewDelegate);
}

void DocView::onWorkspaceViewSelected()
{
  if (auto statusBar = StatusBar::instance())
    statusBar->showDefaultText(m_document);
}

void DocView::onClonedFrom(WorkspaceView* from)
{
  Editor* newEditor = this->editor();
  Editor* srcEditor = static_cast<DocView*>(from)->editor();

  newEditor->setLayer(srcEditor->layer());
  newEditor->setFrame(srcEditor->frame());
  newEditor->setZoom(srcEditor->zoom());

  View::getView(newEditor)->setViewScroll(View::getView(srcEditor)->viewScroll());
}

bool DocView::onCloseView(Workspace* workspace, bool quitting)
{
  if (m_editor->isMovingPixels())
    m_editor->dropMovingPixels();

  // If there is another view for this document, just close the view.
  for (auto view : *workspace) {
    DocView* docView = dynamic_cast<DocView*>(view);
    if (docView && docView != this && docView->document() == document()) {
      workspace->removeView(this);
      delete this;
      return true;
    }
  }

  UIContext* ctx = UIContext::instance();
  SetRestoreDocView restoreView(ctx, this);
  bool save_it;
  bool try_again = true;

  while (try_again) {
    // This flag indicates if we have to sabe the sprite before to destroy it
    save_it = false;

    // See if the sprite has changes
    while (m_document->isModified()) {
      // ask what want to do the user with the changes in the sprite
      int ret = Alert::show(Strings::alerts_save_sprite_changes(
        m_document->name(),
        (quitting ? Strings::alerts_save_sprite_changes_quitting() :
                    Strings::alerts_save_sprite_changes_closing())));

      if (ret == 1) {
        // "save": save the changes
        save_it = true;
        break;
      }
      else if (ret != 2) {
        // "cancel" or "ESC" */
        return false; // we back doing nothing
      }
      else {
        // "discard"
        break;
      }
    }

    // Does we need to save the sprite?
    if (save_it) {
      ctx->updateFlags();

      Command* save_command = Commands::instance()->byId(CommandId::SaveFile());
      ctx->executeCommand(save_command);

      try_again = true;
    }
    else
      try_again = false;
  }

  try {
    // Destroy the sprite (locking it as writer)
    DocDestroyer destroyer(static_cast<app::Context*>(m_document->context()), m_document, 500);

    StatusBar::instance()->setStatusText(0, fmt::format("Sprite '{}' closed.", m_document->name()));

    // Just close the document (so we can reopen it with
    // ReopenClosedFile command).
    destroyer.closeDocument();

    // At this point the view is already destroyed
    return true;
  }
  catch (const LockedDocException& ex) {
    Console::showException(ex);
    return false;
  }
}

void DocView::onTabPopup(Workspace* workspace)
{
  Menu* menu = AppMenus::instance()->getDocumentTabPopupMenu();
  if (!menu)
    return;

  UIContext* ctx = UIContext::instance();
  ctx->setActiveView(this);
  ctx->updateFlags();

  menu->showPopup(mousePosInDisplay(), display());
}

bool DocView::onProcessMessage(Message* msg)
{
  switch (msg->type()) {
    case kFocusEnterMessage:
      if (msg->recipient() != m_editor)
        m_editor->requestFocus();
      break;
  }
  return Box::onProcessMessage(msg);
}

void DocView::onGeneralUpdate(DocEvent& ev)
{
  if (m_editor->isVisible())
    m_editor->updateEditor(true);
}

void DocView::onSpritePixelsModified(DocEvent& ev)
{
  if (m_editor->isVisible() && m_editor->frame() == ev.frame())
    m_editor->drawSpriteClipped(ev.region());
}

void DocView::onLayerMergedDown(DocEvent& ev)
{
  m_editor->setLayer(ev.targetLayer());
}

void DocView::onAddLayer(DocEvent& ev)
{
  if (m_editor->isActive()) {
    ASSERT(ev.layer() != NULL);
    m_editor->setLayer(ev.layer());
  }
}

void DocView::onAddFrame(DocEvent& ev)
{
  if (m_editor->isActive())
    m_editor->setFrame(ev.frame());
  else if (m_editor->frame() > ev.frame())
    m_editor->setFrame(m_editor->frame() + 1);
}

void DocView::onRemoveFrame(DocEvent& ev)
{
  // Adjust current frame of all editors that are in a frame more
  // advanced that the removed one.
  if (m_editor->frame() > ev.frame()) {
    m_editor->setFrame(m_editor->frame() - 1);
  }
  // If the editor was in the previous "last frame" (current value of
  // totalFrames()), we've to adjust it to the new last frame
  // (lastFrame())
  else if (m_editor->frame() >= m_editor->sprite()->totalFrames()) {
    m_editor->setFrame(m_editor->sprite()->lastFrame());
  }
}

void DocView::onTagChange(DocEvent& ev)
{
  if (m_previewDelegate)
    m_previewDelegate->onTagChangeEditor(m_editor, ev);
}

void DocView::onAddCel(DocEvent& ev)
{
  UIContext::instance()->notifyActiveSiteChanged();
}

void DocView::onAfterRemoveCel(DocEvent& ev)
{
  // This can happen when we apply a filter that clear the whole cel
  // and then the cel is removed in a background/job
  // thread. (e.g. applying a convolution matrix)
  if (!ui::is_ui_thread())
    return;

  UIContext::instance()->notifyActiveSiteChanged();
}

void DocView::onTotalFramesChanged(DocEvent& ev)
{
  if (m_editor->frame() >= m_editor->sprite()->totalFrames()) {
    m_editor->setFrame(m_editor->sprite()->lastFrame());
  }
}

void DocView::onLayerRestacked(DocEvent& ev)
{
  if (hasContentInActiveFrame(ev.layer()))
    m_editor->invalidate();
}

void DocView::onAfterLayerVisibilityChange(DocEvent& ev)
{
  // If there is no cel for this layer in the current frame, there is
  // no need to redraw the editor
  if (hasContentInActiveFrame(ev.layer()))
    m_editor->invalidate();
}

void DocView::onTilesetChanged(DocEvent& ev)
{
  // This can happen when a filter is applied to each tile in a
  // background thread.
  if (!ui::is_ui_thread())
    return;

  m_editor->invalidate();
}

void DocView::onNewInputPriority(InputChainElement* element, const ui::Message* msg)
{
  // Do nothing
}

bool DocView::onCanCut(Context* ctx)
{
  if (ctx->checkFlags(ContextFlags::ActiveDocumentIsWritable | ContextFlags::ActiveLayerIsVisible |
                      ContextFlags::ActiveLayerIsEditable | ContextFlags::HasVisibleMask |
                      ContextFlags::HasActiveImage) &&
      !ctx->checkFlags(ContextFlags::ActiveLayerIsReference))
    return true;
  else if (m_editor->isMovingPixels())
    return true;
  else
    return false;
}

bool DocView::onCanCopy(Context* ctx)
{
  if (ctx->checkFlags(ContextFlags::ActiveDocumentIsWritable | ContextFlags::ActiveLayerIsVisible |
                      ContextFlags::HasVisibleMask | ContextFlags::HasActiveImage) &&
      !ctx->checkFlags(ContextFlags::ActiveLayerIsReference))
    return true;
  else if (m_editor->isMovingPixels())
    return true;
  else
    return false;
}

bool DocView::onCanPaste(Context* ctx)
{
  if (ctx->checkFlags(ContextFlags::ActiveDocumentIsWritable | ContextFlags::ActiveLayerIsVisible |
                      ContextFlags::ActiveLayerIsEditable | ContextFlags::ActiveLayerIsImage) &&
      !ctx->checkFlags(ContextFlags::ActiveLayerIsReference)) {
    auto format = ctx->clipboard()->format();
    if (format == ClipboardFormat::Image) {
      return true;
    }
    else if (format == ClipboardFormat::Tilemap &&
             ctx->checkFlags(ContextFlags::ActiveLayerIsTilemap)) {
      return true;
    }
  }
  return false;
}

bool DocView::onCanClear(Context* ctx)
{
  if (ctx->checkFlags(ContextFlags::ActiveDocumentIsWritable | ContextFlags::ActiveLayerIsVisible |
                      ContextFlags::ActiveLayerIsEditable | ContextFlags::ActiveLayerIsImage) &&
      !ctx->checkFlags(ContextFlags::ActiveLayerIsReference)) {
    return true;
  }
  else if (m_editor->isMovingPixels()) {
    return true;
  }
  else
    return false;
}

bool DocView::onCut(Context* ctx)
{
  ContextWriter writer(ctx);
  ctx->clipboard()->cut(writer);
  return true;
}

bool DocView::onCopy(Context* ctx)
{
  const ContextReader reader(ctx);
  if (reader.site()->document() &&
      static_cast<const Doc*>(reader.site()->document())->isMaskVisible() &&
      reader.site()->image()) {
    ctx->clipboard()->copy(reader);
    return true;
  }
  else
    return false;
}

bool DocView::onPaste(Context* ctx, const gfx::Point* position)
{
  auto clipboard = ctx->clipboard();
  if (clipboard->format() == ClipboardFormat::Image ||
      clipboard->format() == ClipboardFormat::Tilemap) {
    clipboard->paste(ctx, true, position);
    return true;
  }
  else
    return false;
}

bool DocView::onClear(Context* ctx)
{
  // First we check if there is a selected slice, so we'll delete
  // those slices.
  Site site = ctx->activeSite();
  if (!site.selectedSlices().empty()) {
    Command* removeSlices = Commands::instance()->byId(CommandId::RemoveSlice());
    ctx->executeCommand(removeSlices);
    return true;
  }

  // In other case we delete the mask or the cel.
  ContextWriter writer(ctx);
  Doc* document = site.document();
  bool visibleMask = document->isMaskVisible();

  CelList cels;
  if (site.range().enabled()) {
    cels = get_unique_cels_to_edit_pixels(site.sprite(), site.range());
  }
  else if (site.cel()) {
    cels.push_back(site.cel());
  }

  if (cels.empty()) // No cels to modify
    return false;

  // TODO This code is similar to clipboard::cut()
  {
    Tx tx(writer, "Clear");
    const bool deselectMask = (visibleMask &&
                               !Preferences::instance().selection.keepSelectionAfterClear());

    ctx->clipboard()->clearMaskFromCels(tx, document, site, cels, deselectMask);

    tx.commit();
  }

  if (visibleMask)
    document->generateMaskBoundaries();

  document->notifyGeneralUpdate();
  return true;
}

void DocView::onCancel(Context* ctx)
{
  if (m_editor)
    m_editor->cancelSelections();

  // Deselect mask
  if (ctx->checkFlags(ContextFlags::ActiveDocumentIsWritable | ContextFlags::HasVisibleMask)) {
    Command* deselectMask = Commands::instance()->byId(CommandId::DeselectMask());
    ctx->executeCommand(deselectMask);
  }
}

bool DocView::hasContentInActiveFrame(const doc::Layer* layer) const
{
  if (!layer)
    return false;
  else if (layer->cel(m_editor->frame()))
    return true;
  else if (layer->isGroup()) {
    for (const doc::Layer* child : static_cast<const doc::LayerGroup*>(layer)->layers()) {
      if (hasContentInActiveFrame(child))
        return true;
    }
  }
  return false;
}

} // namespace app
