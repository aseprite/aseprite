// Aseprite
// Copyright (C) 2018  Igara Studio S.A.
// Copyright (c) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_DOCS_H_INCLUDED
#define APP_DOCS_H_INCLUDED
#pragma once

#include "app/docs_observer.h"
#include "base/disable_copying.h"
#include "doc/color_mode.h"
#include "doc/object_id.h"
#include "obs/observable.h"

#include <vector>

namespace app {

  class Context;
  class Doc;

  class Docs : public obs::observable<DocsObserver> {
  public:
    typedef std::vector<Doc*>::iterator iterator;
    typedef std::vector<Doc*>::const_iterator const_iterator;

    Docs(Context* ctx);
    ~Docs();

    iterator begin() { return m_docs.begin(); }
    iterator end() { return m_docs.end(); }
    const_iterator begin() const { return m_docs.begin(); }
    const_iterator end() const { return m_docs.end(); }

    Doc* front() const { return m_docs.front(); }
    Doc* back() const { return m_docs.back(); }
    Doc* lastAdded() const { return front(); }

    int size() const { return (int)m_docs.size(); }
    bool empty() const { return m_docs.empty(); }

    // Add a new documents to the list.
    Doc* add(Doc* doc);

    // Easy function for testing
    Doc* add(int width, int height,
             doc::ColorMode colorMode = doc::ColorMode::RGB,
             int ncolors = 256);

    // Removes a document from the list without deleting it. You must
    // to delete the document after removing it.
    void remove(Doc* doc);

    // Moves the document to the given location in the same
    // list. E.g. It is used to reorder documents when they are
    // selected as active.
    void move(Doc* doc, int index);

    Doc* operator[](int index) const { return m_docs[index]; }
    Doc* getById(doc::ObjectId id) const;
    Doc* getByName(const std::string& name) const;
    Doc* getByFileName(const std::string& filename) const;

  private:
    // Deletes all documents in the list (calling "delete" operation).
    void deleteAll();

    Context* m_ctx;
    std::vector<Doc*> m_docs;

    DISABLE_COPYING(Docs);
  };

} // namespace app

#endif
