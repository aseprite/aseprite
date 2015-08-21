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
#include "app/cmd/set_cel_opacity.h"
#include "app/commands/command.h"
#include "app/console.h"
#include "app/context_access.h"
#include "app/document_api.h"
#include "app/find_widget.h"
#include "app/load_widget.h"
#include "app/modules/gui.h"
#include "app/transaction.h"
#include "app/ui_context.h"
#include "base/bind.h"
#include "base/mem_utils.h"
#include "base/scoped_value.h"
#include "doc/cel.h"
#include "doc/document_event.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/sprite.h"
#include "ui/ui.h"

#include "generated_cel_properties.h"

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
    , m_cel(nullptr)
    , m_selfUpdate(false) {
    opacity()->Change.connect(Bind<void>(&CelPropertiesWindow::onShowChange, this));
    m_timer.Tick.connect(Bind<void>(&CelPropertiesWindow::onCommitChange, this));

    remapWindow();
    centerWindow();
    load_window_pos(this, "CelProperties");

    UIContext::instance()->addObserver(this);
  }

  ~CelPropertiesWindow() {
    UIContext::instance()->removeObserver(this);
  }

  void setCel(Cel* cel) {
    // Save uncommited changes
    if (m_cel) {
      if (m_timer.isRunning())
        onCommitChange();

      document()->removeObserver(this);
      m_cel = nullptr;
    }

    m_timer.stop();
    m_cel = const_cast<Cel*>(cel);

    base::ScopedValue<bool> switchSelf(m_selfUpdate, true, false);

    if (m_cel) {
      document()->addObserver(this);

      m_oldOpacity = cel->opacity();

      opacity()->setValue(cel->opacity());
      opacity()->setEnabled(!cel->layer()->isBackground());
    }
    else {
      opacity()->setEnabled(false);
    }
  }

private:

  app::Document* document() {
    ASSERT(m_cel);
    if (m_cel)
      return static_cast<app::Document*>(m_cel->document());
    else
      return nullptr;
  }

  int opacityValue() const {
    return opacity()->getValue();
  }

  bool onProcessMessage(ui::Message* msg) override {
    switch (msg->type()) {

      case kCloseMessage:
        // Save changes before we close the window
        setCel(nullptr);
        save_window_pos(this, "CelProperties");

        deferDelete();
        g_window = nullptr;
        break;

    }
    return Window::onProcessMessage(msg);
  }

  void onShowChange() {
    if (m_selfUpdate)
      return;

    m_cel->setOpacity(opacityValue());
    m_timer.start();

    update_screen_for_document(document());
  }

  void onCommitChange() {
    base::ScopedValue<bool> switchSelf(m_selfUpdate, true, false);

    m_timer.stop();

    int newOpacity = opacityValue();

    m_cel->setOpacity(m_oldOpacity);

    if (newOpacity != m_oldOpacity) {
      try {
        ContextWriter writer(UIContext::instance());
        Transaction transaction(writer.context(), "Cel Opacity Change");

        if (newOpacity != m_oldOpacity)
          transaction.execute(new cmd::SetCelOpacity(writer.cel(), newOpacity));

        transaction.commit();
      }
      catch (const std::exception& e) {
        Console::showException(e);
      }

      update_screen_for_document(document());
    }
  }

  // ContextObserver impl
  void onActiveSiteChange(const Site& site) override {
    if (isVisible())
      setCel(const_cast<Cel*>(site.cel()));
    else if (m_cel)
      setCel(nullptr);
  }

  // DocumentObserver impl
  void onCelOpacityChange(DocumentEvent& ev) override {
    updateFromCel(ev.cel());
  }

  void updateFromCel(Cel* cel) {
    if (m_selfUpdate)
      return;

    // Cancel current editions (just in case)
    m_timer.stop();
    setCel(cel);
  }

  Timer m_timer;
  Cel* m_cel;
  int m_oldOpacity;
  bool m_selfUpdate;
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
  Cel* cel = reader.cel();

  if (!g_window)
    g_window = new CelPropertiesWindow;

  g_window->setCel(cel);
  g_window->openWindow();

  // Focus layer name
  g_window->opacity()->requestFocus();
}

Command* CommandFactory::createCelPropertiesCommand()
{
  return new CelPropertiesCommand;
}

} // namespace app
