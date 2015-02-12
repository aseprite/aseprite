// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_PROJECT_EVENT_H_INCLUDED
#define APP_PROJECT_EVENT_H_INCLUDED
#pragma once

namespace app {
  class Document;

  class ProjectEvent {
  public:
    ProjectEvent(Document* document)
      : m_document(document) {
    }

    Document* document() const { return m_document; }

  private:
    Document* m_document;
  };

} // namespace app

#endif
