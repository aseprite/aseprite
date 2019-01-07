// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/cmd/set_cel_opacity.h"
#include "app/cmd/set_user_data.h"
#include "app/commands/command.h"
#include "app/console.h"
#include "app/context_access.h"
#include "app/doc_event.h"
#include "app/doc_range.h"
#include "app/modules/gui.h"
#include "app/tx.h"
#include "app/ui/timeline/timeline.h"
#include "app/ui/user_data_popup.h"
#include "app/ui_context.h"
#include "base/bind.h"
#include "base/mem_utils.h"
#include "base/scoped_value.h"
#include "doc/cel.h"
#include "doc/cels_range.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/sprite.h"
#include "ui/ui.h"

#include "cel_properties.xml.h"

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
    , m_document(nullptr)
    , m_cel(nullptr)
    , m_selfUpdate(false)
    , m_newUserData(false) {
    opacity()->Change.connect(base::Bind<void>(&CelPropertiesWindow::onStartTimer, this));
    userData()->Click.connect(base::Bind<void>(&CelPropertiesWindow::onPopupUserData, this));
    m_timer.Tick.connect(base::Bind<void>(&CelPropertiesWindow::onCommitChange, this));

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

    updateFromCel();
  }

private:

  int opacityValue() const {
    return opacity()->getValue();
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
        if (opacity()->hasFocus()) {
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
        setCel(nullptr, nullptr);
        save_window_pos(this, "CelProperties");

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
  }

  void onCommitChange() {
    base::ScopedValue<bool> switchSelf(m_selfUpdate, true, false);

    m_timer.stop();

    const int newOpacity = opacityValue();
    const int count = countCels();

    if ((count > 1) ||
        (count == 1 && m_cel && (newOpacity != m_cel->opacity() ||
                                 m_userData != m_cel->data()->userData()))) {
      try {
        ContextWriter writer(UIContext::instance());
        Tx tx(writer.context(), "Set Cel Properties");

        DocRange range;
        if (m_range.enabled())
          range = m_range;
        else {
          range.startRange(m_cel->layer(), m_cel->frame(), DocRange::kCels);
          range.endRange(m_cel->layer(), m_cel->frame());
        }

        Sprite* sprite = m_document->sprite();
        for (Cel* cel : sprite->uniqueCels(range.selectedFrames())) {
          if (range.contains(cel->layer())) {
            if (!cel->layer()->isBackground() && newOpacity != cel->opacity()) {
              tx(new cmd::SetCelOpacity(cel, newOpacity));
            }

            if (m_newUserData &&
                m_userData != cel->data()->userData()) {
              tx(new cmd::SetUserData(cel->data(), m_userData));

              // Redraw timeline because the cel's user data/color
              // might have changed.
              App::instance()->timeline()->invalidate();
            }
          }
        }

        tx.commit();
      }
      catch (const std::exception& e) {
        Console::showException(e);
      }

      update_screen_for_document(m_document);
    }
  }

  void onPopupUserData() {
    if (countCels() > 0) {
      m_newUserData = false;
      if (m_cel)
        m_userData = m_cel->data()->userData();
      else
        m_userData = UserData();

      if (show_user_data_popup(userData()->bounds(), m_userData)) {
        m_newUserData = true;
        onCommitChange();
      }
    }
  }

  // ContextObserver impl
  void onActiveSiteChange(const Site& site) override {
    if (isVisible())
      setCel(const_cast<Doc*>(site.document()),
             const_cast<Cel*>(site.cel()));
    else if (m_document)
      setCel(nullptr, nullptr);
  }

  // DocObserver impl
  void onCelOpacityChange(DocEvent& ev) override {
    if (m_cel == ev.cel())
      updateFromCel();
  }

  void updateFromCel() {
    if (m_selfUpdate)
      return;

    m_timer.stop(); // Cancel current editions (just in case)

    base::ScopedValue<bool> switchSelf(m_selfUpdate, true, false);

    int bgCount = 0;
    int count = countCels(&bgCount);

    m_userData = UserData();
    m_newUserData = false;

    if (count > 0) {
      if (m_cel) {
        opacity()->setValue(m_cel->opacity());
        m_userData = m_cel->data()->userData();
      }
      opacity()->setEnabled(bgCount < count);
    }
    else {
      opacity()->setEnabled(false);
    }
  }

  Timer m_timer;
  Doc* m_document;
  Cel* m_cel;
  DocRange m_range;
  bool m_selfUpdate;
  UserData m_userData;
  bool m_newUserData;
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
