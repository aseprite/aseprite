/* Aseprite
 * Copyright (C) 2001-2014  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/devconsole_view.h"

#include "ui/entry.h"
#include "ui/message.h"
#include "ui/textbox.h"
#include "ui/view.h"

namespace app {

using namespace ui;

class DevConsoleView::CommmandEntry : public Entry {
public:
  CommmandEntry() : Entry(256, "") {
    setFocusStop(true);
    setFocusMagnet(true);
  }

  Signal1<void, const std::string&> ExecuteCommand;

protected:
  bool onProcessMessage(Message* msg) OVERRIDE {
    switch (msg->type()) {
      case kKeyDownMessage:
        if (hasFocus()) {
          KeyMessage* keymsg = static_cast<KeyMessage*>(msg);
          KeyScancode scancode = keymsg->scancode();

          switch (scancode) {
            case kKeyEnter:
            case kKeyEnterPad: {
              std::string cmd = getText();
              ExecuteCommand(cmd);
              setText("");
              return true;
            }
          }
        }
        break;
    }
    return Entry::onProcessMessage(msg);
  }
};

DevConsoleView::DevConsoleView()
  : Box(JI_VERTICAL)
  , m_textBox("Welcome Aseprite Console\n(experimental)", JI_LEFT)
  , m_label(">")
  , m_entry(new CommmandEntry)
{
  addChild(&m_view);
  addChild(&m_bottomBox);

  m_bottomBox.addChild(&m_label);
  m_bottomBox.addChild(m_entry);

  m_view.attachToView(&m_textBox);
  m_view.setExpansive(true);
  m_entry->setExpansive(true);

  m_entry->ExecuteCommand.connect(&DevConsoleView::onExecuteCommand, this);
}

DevConsoleView::~DevConsoleView()
{
  // m_document->removeObserver(this);
  // delete m_editor;
}

std::string DevConsoleView::getTabText()
{
  return "Console";
}

WorkspaceView* DevConsoleView::cloneWorkspaceView()
{
  return new DevConsoleView();
}

void DevConsoleView::onWorkspaceViewSelected()
{
  m_entry->requestFocus();
}

void DevConsoleView::onClonedFrom(WorkspaceView* from)
{
}

bool DevConsoleView::onProcessMessage(Message* msg)
{
  return Box::onProcessMessage(msg);
}

void DevConsoleView::onExecuteCommand(const std::string& cmd)
{
  m_textBox.setText(m_textBox.getText() + "\n" + cmd);
}

} // namespace app
