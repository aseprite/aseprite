// SHE library
// Copyright (C) 2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "she/win/window_dde.h"

#include "base/string.h"
#include "she/event.h"
#include "she/event_queue.h"

#include <string>
#include <vector>

namespace she {

namespace {

std::string get_atom_string(ATOM atom, bool isUnicode)
{
  std::string result;
  if (!atom)
    return result;

  if (isUnicode) {
    std::vector<WCHAR> buf(256, 0);
    GlobalGetAtomNameW(atom, &buf[0], buf.size());
    result = base::to_utf8(std::wstring(&buf[0]));
  }
  else {
    std::vector<char> buf(256, 0);
    UINT chars = GlobalGetAtomNameA(atom, &buf[0], buf.size());
    result = &buf[0];
  }

  return result;
}

// Converts a DDE command string (e.g. "[open(filename)]") into she events.
// https://msdn.microsoft.com/en-us/library/windows/desktop/ms648995.aspx
bool parse_dde_command(const std::string& cmd)
{
  bool result = false;

  for (size_t i=0; i<cmd.size(); ) {
    if (cmd[i] == '[') {
      size_t j = ++i;

      for (; j<cmd.size(); ++j) {
        if (cmd[j] == '(')
          break;
      }

      std::string cmdName = cmd.substr(i, j-i);
      std::vector<std::string> cmdParams;

      for (i=j+1; i<cmd.size(); ) {
        if (cmd[i] == ')') {
          j = i+1;
          break;
        }
        else if (cmd[i] == '"') {
          std::string cmdParam;
          for (j=++i; j<cmd.size(); ++j) {
            if (cmd[j] == '"')
              break;
            else
              cmdParam.push_back(cmd[j]);
          }
          cmdParams.push_back(cmdParam);
          i = j+1;
        }
        else {
          std::string cmdParam;
          for (j=i; j<cmd.size(); ++j) {
            if (cmd[j] == ',' || cmd[j] == ')')
              break;
            else
              cmdParam.push_back(cmd[j]);
          }
          cmdParams.push_back(cmdParam);
          i = j;
        }
      }

      if (cmdName == "open" && !cmdParams.empty()) {
        // Send a message to open the file as if it were dropped into the window.
        Event ev;
        ev.setType(Event::DropFiles);
        ev.setFiles(cmdParams);
        she::queue_event(ev);

        result = true;
      }

      i = j;
    }
    else
      ++i;
  }

  return result;
}

union DdeackContainer {
  WORD value;
  DDEACK ddeack;
};

} // anonymous namespace

bool handle_dde_messages(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, LRESULT& result)
{
  switch (msg) {

    case WM_DDE_INITIATE: {
      HWND clienthwnd = (HWND)wparam;
      UINT appAtom = 0, topicAtom = 0;
      if (!UnpackDDElParam(msg, lparam,
                           &appAtom,
                           &topicAtom))
        return false;

      bool useUnicode = (IsWindowUnicode(clienthwnd) ? true: false);
      std::string app = get_atom_string((ATOM)appAtom, useUnicode);
      std::string topic = get_atom_string((ATOM)topicAtom, useUnicode);
      FreeDDElParam(msg, lparam);

#ifndef PACKAGE
#define PACKAGE ""
#pragma message("warning: Define PACKAGE macro with your app name")
#endif

      if (app != PACKAGE) {
        return false;
      }

      // We have to create new atoms for this WM_DDE_ACK, we cannot
      // reuse them according to MSDN:
      //
      //   https://msdn.microsoft.com/en-us/library/windows/desktop/ms648996(v=vs.85).aspx
      //
      // Anyway it looks like Windows returns the same ATOM number.
      ATOM newApp = (useUnicode ? GlobalAddAtomW(base::from_utf8(app).c_str()):
                                  GlobalAddAtomA(app.c_str()));
      ATOM newTopic = (useUnicode ? GlobalAddAtomW(base::from_utf8(topic).c_str()):
                                    GlobalAddAtomA(topic.c_str()));

      if (newApp && newTopic) {
        SendMessage(clienthwnd,
                    WM_DDE_ACK,
                    (WPARAM)hwnd,
                    PackDDElParam(WM_DDE_ACK, newApp, newTopic));
      }

      if (newApp) GlobalDeleteAtom(newApp);
      if (newTopic) GlobalDeleteAtom(newTopic);

      result = 0;
      return true;
    }

    case WM_DDE_EXECUTE: {
      HWND clienthwnd = (HWND)wparam;
      bool useUnicode = (IsWindowUnicode(clienthwnd) ? true: false);
      LPCVOID cmdPtr = (LPCVOID)GlobalLock((HGLOBAL)lparam);
      std::string cmd;

      if (useUnicode)
        cmd = base::to_utf8((const WCHAR*)cmdPtr);
      else
        cmd = (const CHAR*)cmdPtr;

      GlobalUnlock((HGLOBAL)lparam);

      DdeackContainer ddeack;
      ddeack.value = 0;
      ddeack.ddeack.fAck = 1;

      PostMessage(clienthwnd, WM_DDE_ACK,
                  (WPARAM)hwnd,
                  ReuseDDElParam(lparam, msg, WM_DDE_ACK,
                                 (UINT_PTR)ddeack.value,
                                 (UINT_PTR)lparam));

      if (parse_dde_command(cmd))
        SetForegroundWindow(hwnd);

      result = 0;
      return true;
    }

    case WM_DDE_TERMINATE: {
      HWND clienthwnd = (HWND)wparam;
      PostMessage(clienthwnd, WM_DDE_TERMINATE, (WPARAM)hwnd, lparam);
      result = 0;
      return true;
    }

  }

  return false;
}

} // namespace she
