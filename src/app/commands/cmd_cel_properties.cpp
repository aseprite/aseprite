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
#include "app/document_range.h"
#include "app/find_widget.h"
#include "app/load_widget.h"
#include "app/modules/gui.h"
#include "app/transaction.h"
#include "app/ui/main_window.h"
#include "app/ui/timeline.h"
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
    , m_document(nullptr)
    , m_cel(nullptr)
    , m_selfUpdate(false) {
    opacity()->Change.connect(Bind<void>(&CelPropertiesWindow::onStartTimer, this));
    m_timer.Tick.connect(Bind<void>(&CelPropertiesWindow::onCommitChange, this));

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

  int countCels() const {
    if (!m_document)
      return 0;
    else if (m_cel &&
             (!m_range.enabled() ||
              (m_range.frames() == 1 &&
               m_range.layers() == 1))) {
      return 1;
    }
    else if (m_range.enabled()) {
      int count = 0;
      for (Cel* cel : m_document->sprite()->uniqueCels()) {
        if (!cel->layer()->isBackground() &&
            m_range.inRange(cel->sprite()->layerToIndex(cel->layer()),
                            cel->frame())) {
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
        (count == 1 && newOpacity != m_cel->opacity())) {
      try {
        ContextWriter writer(UIContext::instance());
        Transaction transaction(writer.context(), "Cel Opacity Change");

        if (count == 1) {
          if (newOpacity != m_cel->opacity())
            transaction.execute(new cmd::SetCelOpacity(writer.cel(), newOpacity));
        }
        else {
          for (Cel* cel : m_document->sprite()->uniqueCels()) {
            if (!cel->layer()->isBackground() &&
                m_range.inRange(cel->sprite()->layerToIndex(cel->layer()),
                                cel->frame())) {
              transaction.execute(new cmd::SetCelOpacity(cel, newOpacity));
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

    int count = countCels();

    if (count > 0) {
      if (m_cel) {
        opacity()->setValue(m_cel->opacity());
        opacity()->setEnabled(
          (count > 1) ||
          (count == 1 && !m_cel->layer()->isBackground()));
      }
      else                  // Enable slider to change the whole range
        opacity()->setEnabled(true);
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
