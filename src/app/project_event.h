// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_PROJECT_EVENT_H_INCLUDED
#define APP_PROJECT_EVENT_H_INCLUDED
#pragma once

namespace app {
  class Doc;

  class ProjectEvent {
  public:
    ProjectEvent(Doc* document)
      : m_document(document) {
    }

    Doc* document() const { return m_document; }

  private:
    Doc* m_document;
  };

} // namespace app

#endif
