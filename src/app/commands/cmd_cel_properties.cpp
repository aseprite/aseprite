// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/cmd/set_cel_opacity.h"
#include "app/cmd/set_user_data.h"
#include "app/commands/command.h"
#include "app/console.h"
#include "app/context_access.h"
#include "app/document_api.h"
#include "app/document_range.h"
#include "app/modules/gui.h"
#include "app/transaction.h"
#include "app/ui/main_window.h"
#include "app/ui/timeline.h"
#include "app/ui/user_data_popup.h"
#include "app/ui_context.h"
#include "base/bind.h"
#include "base/mem_utils.h"
#include "base/scoped_value.h"
#include "doc/cel.h"
#include "doc/cels_range.h"
#include "doc/document_event.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/sprite.h"
#include "ui/ui.h"

#include "cel_properties.xml.h"

namespace app {

using namespace ui;

class CelPropertiesWindow;
static CelPropertiesWindow* g_window = nullptr;

class CelPropertiesWindow : public app::gen::CelProperties
                          , public doc::ContextObserver
                          , public doc::DocumentObserver {
public:
  CelPropertiesWindow()
    : m_timer(250, this)
    , m_document(nullptr)
    , m_cel(nullptr)
    , m_selfUpdate(false) {
    opacity()->Change.connect(base::Bind<void>(&CelPropertiesWindow::onStartTimer, this));
    userData()->Click.connect(base::Bind<void>(&CelPropertiesWindow::onPopupUserData, this));
    m_timer.Tick.connect(base::Bind<void>(&CelPropertiesWindow::onCommitChange, this));

    remapWindow();
    centerWindow();
    load_window_pos(this, "CelProperties");

    UIContext::instance()->addObserver(this);
  }

  ~CelPropertiesWindow() {
    UIContext::instance()->removeObserver(this);
  }

  void setCel(Document* doc, Cel* cel) {
    if (m_document) {
      m_document->removeObserver(this);
      m_document = nullptr;
      m_cel = nullptr;
    }

    m_timer.stop();
    m_document = doc;
    m_cel = cel;
    m_range = App::instance()->getMainWindow()->getTimeline()->range();

    if (m_document)
      m_document->addObserver(this);

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
      for (Cel* cel : sprite->uniqueCels(m_range.frameBegin(),
                                         m_range.frameEnd())) {
        if (m_range.inRange(sprite->layerToIndex(cel->layer()))) {
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
          if (static_cast<KeyMessage*>(msg)->scancode() == kKeyEnter) {
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

    int newOpacity = opacityValue();
    int count = countCels();

    if ((count > 1) ||
        (count == 1 && m_cel && (newOpacity != m_cel->opacity() ||
                                 m_userData != m_cel->data()->userData()))) {
      try {
        ContextWriter writer(UIContext::instance());
        Transaction transaction(writer.context(), "Set Cel Properties");

        if (count == 1 && m_cel) {
          if (!m_cel->layer()->isBackground() &&
              newOpacity != m_cel->opacity()) {
            transaction.execute(new cmd::SetCelOpacity(writer.cel(), newOpacity));
          }

          if (m_userData != m_cel->data()->userData()) {
            transaction.execute(new cmd::SetUserData(writer.cel()->data(), m_userData));

            // Redraw timeline because the cel's user data/color
            // might have changed.
            App::instance()->getMainWindow()->getTimeline()->invalidate();
          }
        }
        else if (m_range.enabled()) {
          Sprite* sprite = m_document->sprite();
          for (Cel* cel : sprite->uniqueCels(m_range.frameBegin(),
                                             m_range.frameEnd())) {
            if (m_range.inRange(sprite->layerToIndex(cel->layer()))) {
              if (!cel->layer()->isBackground() && newOpacity != cel->opacity()) {
                transaction.execute(new cmd::SetCelOpacity(cel, newOpacity));
              }

              if (m_userData != cel->data()->userData()) {
                transaction.execute(new cmd::SetUserData(cel->data(), m_userData));

                // Redraw timeline because the cel's user data/color
                // might have changed.
                App::instance()->getMainWindow()->getTimeline()->invalidate();
              }
            }
          }
        }

        transaction.commit();
      }
      catch (const std::exception& e) {
        Console::showException(e);
      }

      update_screen_for_document(m_document);
    }
  }

  void onPopupUserData() {
    if (countCels() > 0) {
      if (m_cel)
        m_userData = m_cel->data()->userData();
      else
        m_userData = UserData();

      if (show_user_data_popup(userData()->bounds(), m_userData)) {
        onCommitChange();
      }
    }
  }

  // ContextObserver impl
  void onActiveSiteChange(const Site& site) override {
    if (isVisible())
      setCel(static_cast<app::Document*>(const_cast<doc::Document*>(site.document())),
             const_cast<Cel*>(site.cel()));
    else if (m_document)
      setCel(nullptr, nullptr);
  }

  // DocumentObserver impl
  void onCelOpacityChange(DocumentEvent& ev) override {
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
  Document* m_document;
  Cel* m_cel;
  DocumentRange m_range;
  bool m_selfUpdate;
  UserData m_userData;
};

class CelPropertiesCommand : public Command {
public:
  CelPropertiesCommand();
  Command* clone() const override { return new CelPropertiesCommand(*this); }

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

CelPropertiesCommand::CelPropertiesCommand()
  : Command("CelProperties",
            "Cel Properties",
            CmdUIOnlyFlag)
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
