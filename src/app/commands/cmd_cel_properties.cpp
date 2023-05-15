// Aseprite
// Copyright (C) 2020-2023  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/cmd/set_cel_opacity.h"
#include "app/cmd/set_cel_zindex.h"
#include "app/cmd/set_user_data.h"
#include "app/commands/command.h"
#include "app/console.h"
#include "app/context_access.h"
#include "app/doc_event.h"
#include "app/doc_range.h"
#include "app/modules/gui.h"
#include "app/tx.h"
#include "app/ui/timeline/timeline.h"
#include "app/ui/user_data_view.h"
#include "app/ui_context.h"
#include "base/mem_utils.h"
#include "base/scoped_value.h"
#include "doc/cel.h"
#include "doc/cels_range.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/sprite.h"
#include "ui/ui.h"

#include "cel_properties.xml.h"

#include <algorithm>
#include <limits>

namespace app {

using namespace ui;

class CelPropertiesWindow;
static CelPropertiesWindow* g_window = nullptr;

class CelPropertiesWindow : public app::gen::CelProperties,
                            public ContextObserver,
                            public DocObserver {
public:
  CelPropertiesWindow()
    : m_timer(250, this)
    , m_userDataView(Preferences::instance().cels.userDataVisibility)
  {
    opacity()->Change.connect([this]{ onStartTimer(); });
    zindex()->Change.connect([this]{ onStartTimer(); });
    userData()->Click.connect([this]{ onToggleUserData(); });
    m_timer.Tick.connect([this]{ onCommitChange(); });

    m_userDataView.UserDataChange.connect([this]{ onStartTimer(); });

    // TODO add to Expr widget spin flag to include these widgets in
    //      the same Expr
    zindexSpin()->ItemChange.connect([this]{
      int dz = (zindexSpin()->selectedItem() == 0 ? +1: -1);
      zindex()->setTextf("%d", zindex()->textInt() + dz);
      onStartTimer();
    });

    remapWindow();
    centerWindow();
    load_window_pos(this, "CelProperties");

    UIContext::instance()->add_observer(this);
  }

  ~CelPropertiesWindow() {
    UIContext::instance()->remove_observer(this);
  }

  void setCel(Doc* doc, Cel* cel) {
    if (m_document) {
      m_document->remove_observer(this);
      m_document = nullptr;
      m_cel = nullptr;
    }

    m_timer.stop();
    m_document = doc;
    m_cel = cel;
    m_range = App::instance()->timeline()->range();

    if (m_document)
      m_document->add_observer(this);

    if (countCels() > 0) {
      m_userDataView.configureAndSet(
        (m_cel ? m_cel->data()->userData(): UserData()),
        g_window->propertiesGrid());
    }
    else if (!m_cel)
      m_userDataView.setVisible(false, false);

    g_window->expandWindow(gfx::Size(g_window->bounds().w,
                                     g_window->sizeHint().h));
    updateFromCel();
  }

private:

  int opacityValue() const {
    return opacity()->getValue();
  }

  int zindexValue() const {
    return zindex()->textInt();
  }

  int countCels(int* backgroundCount = nullptr) const {
    if (backgroundCount)
      *backgroundCount = 0;

    if (!m_document)
      return 0;
    else if (m_cel &&
             (!m_range.enabled() ||
              (m_range.frames() == 1 &&
               m_range.layers() == 1))) {
      if (backgroundCount && m_cel->layer()->isBackground())
        *backgroundCount = 1;
      return 1;
    }
    else if (m_range.enabled()) {
      Sprite* sprite = m_document->sprite();
      int count = 0;
      for (Cel* cel : sprite->uniqueCels(m_range.selectedFrames())) {
        if (m_range.contains(cel->layer())) {
          if (backgroundCount && cel->layer()->isBackground())
            ++(*backgroundCount);
          ++count;
        }
      }
      return count;
    }
    else
      return 0;
  }

  bool onProcessMessage(ui::Message* msg) override {
    switch (msg->type()) {

      case kKeyDownMessage:
        if (opacity()->hasFocus() ||
            zindex()->hasFocus()) {
          KeyScancode scancode = static_cast<KeyMessage*>(msg)->scancode();
          if (scancode == kKeyEnter ||
              scancode == kKeyEsc) {
            onCommitChange();
            closeWindow(this);
            return true;
          }
        }
        break;

      case kCloseMessage:
        // Save changes before we close the window
        if (m_cel)
          save_window_pos(this, "CelProperties");
        setCel(nullptr, nullptr);
        deferDelete();
        g_window = nullptr;
        break;

    }
    return Window::onProcessMessage(msg);
  }

  void onStartTimer() {
    if (m_selfUpdate)
      return;

    m_timer.start();
    m_pendingChanges = true;
  }

  void onCommitChange() {
    // Nothing to change
    if (!m_pendingChanges)
      return;

    base::ScopedValue switchSelf(m_selfUpdate, true);

    m_timer.stop();

    const int newOpacity = opacityValue();
    // Clamp z-index to the limits of the .aseprite specs
    const int newZIndex = std::clamp<int>(zindexValue(),
                                          std::numeric_limits<int16_t>::min(),
                                          std::numeric_limits<int16_t>::max());
    const UserData newUserData= m_userDataView.userData();
    const int count = countCels();

    if ((count > 1) ||
        (count == 1 && m_cel && (newOpacity != m_cel->opacity() ||
                                 newZIndex != m_cel->zIndex() ||
                                 newUserData != m_cel->data()->userData()))) {
      try {
        ContextWriter writer(UIContext::instance());
        Tx tx(writer.context(), "Set Cel Properties");

        DocRange range;
        if (m_range.enabled()) {
          range = m_range;
          range.convertToCels(m_document->sprite());
        }
        else {
          range.startRange(m_cel->layer(), m_cel->frame(), DocRange::kCels);
          range.endRange(m_cel->layer(), m_cel->frame());
        }

        Sprite* sprite = m_document->sprite();
        bool redrawTimeline = false;

        // For each unique cel (don't repeat on links)
        for (Cel* cel : sprite->uniqueCels(range.selectedFrames())) {
          if (range.contains(cel->layer())) {
            if (!cel->layer()->isBackground() && newOpacity != cel->opacity()) {
              tx(new cmd::SetCelOpacity(cel, newOpacity));
            }

            if (newUserData != cel->data()->userData()) {
              tx(new cmd::SetUserData(cel->data(), newUserData, m_document));

              // Redraw timeline because the cel's user data/color
              // might have changed.
              if (newUserData.color() != cel->data()->userData().color()) {
                redrawTimeline = true;
              }
            }
          }
        }

        // For all cels (repeat links)
        for (Cel* cel : sprite->cels(range.selectedFrames())) {
          if (range.contains(cel->layer())) {
            if (newZIndex != cel->zIndex()) {
              tx(new cmd::SetCelZIndex(cel, newZIndex));
              redrawTimeline = true;
            }
          }
        }

        if (redrawTimeline)
          App::instance()->timeline()->invalidate();

        tx.commit();
      }
      catch (const std::exception& e) {
        Console::showException(e);
      }

      update_screen_for_document(m_document);
    }

    m_pendingChanges = false;
  }

  void onToggleUserData() {
    if (countCels() > 0) {
      m_userDataView.toggleVisibility();
      g_window->expandWindow(gfx::Size(g_window->bounds().w,
                                       g_window->sizeHint().h));
    }
  }

  // ContextObserver impl
  void onActiveSiteChange(const Site& site) override {
    onCommitChange();
    if (isVisible())
      setCel(const_cast<Doc*>(site.document()),
             const_cast<Cel*>(site.cel()));
    else if (m_document)
      setCel(nullptr, nullptr);
  }

  // DocObserver impl
  void onBeforeRemoveCel(DocEvent& ev) override {
    if (m_cel == ev.cel())
      setCel(m_document, nullptr);
  }

  void onCelOpacityChange(DocEvent& ev) override {
    if (m_cel == ev.cel())
      updateFromCel();
  }

  void onCelZIndexChange(DocEvent& ev) override {
    if (m_cel == ev.cel())
      updateFromCel();
  }

  void onUserDataChange(DocEvent& ev) override {
     if (m_cel && m_cel->data() == ev.withUserData())
      updateFromCel();
  }

  void updateFromCel() {
    if (m_selfUpdate)
      return;

    m_timer.stop(); // Cancel current editions (just in case)

    base::ScopedValue switchSelf(m_selfUpdate, true);

    int bgCount = 0;
    int count = countCels(&bgCount);

    if (count > 0) {
      if (m_cel) {
        opacity()->setValue(m_cel->opacity());
        zindex()->setTextf("%d", m_cel->zIndex());
        color_t c = m_cel->data()->userData().color();
        m_userDataView.color()->setColor(Color::fromRgb(rgba_getr(c), rgba_getg(c), rgba_getb(c), rgba_geta(c)));
        m_userDataView.entry()->setText(m_cel->data()->userData().text());
      }
      opacity()->setEnabled(bgCount < count);
    }
    else {
      opacity()->setEnabled(false);
      m_userDataView.setVisible(false, false);
    }
  }

  Timer m_timer;
  bool m_pendingChanges = false;
  Doc* m_document = nullptr;
  Cel* m_cel = nullptr;
  DocRange m_range;
  bool m_selfUpdate = false;
  UserDataView m_userDataView;
};

class CelPropertiesCommand : public Command {
public:
  CelPropertiesCommand();

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

CelPropertiesCommand::CelPropertiesCommand()
  : Command(CommandId::CelProperties(), CmdUIOnlyFlag)
{
}

bool CelPropertiesCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::ActiveLayerIsImage);
}

void CelPropertiesCommand::onExecute(Context* context)
{
  ContextReader reader(context);

  if (!g_window)
    g_window = new CelPropertiesWindow;

  g_window->setCel(reader.document(), reader.cel());
  g_window->openWindow();

  // Focus layer name
  g_window->opacity()->requestFocus();
}

Command* CommandFactory::createCelPropertiesCommand()
{
  return new CelPropertiesCommand;
}

} // namespace app
