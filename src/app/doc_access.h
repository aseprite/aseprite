// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_DOC_ACCESS_H_INCLUDED
#define APP_DOC_ACCESS_H_INCLUDED
#pragma once

#include "app/context.h"
#include "app/doc.h"
#include "base/exception.h"

#include <exception>

namespace app {

  // TODO remove exceptions and use "DocAccess::operator bool()"
  class LockedDocException : public base::Exception {
  public:
    LockedDocException(const char* msg) throw()
    : base::Exception(msg) { }
  };

  class CannotReadDocException : public LockedDocException {
  public:
    CannotReadDocException() throw()
    : LockedDocException("Cannot read the sprite.\n"
                         "It is being modified by another command.\n"
                         "Try again.") { }
  };

  class CannotWriteDocException : public LockedDocException {
  public:
    CannotWriteDocException() throw()
    : LockedDocException("Cannot modify the sprite.\n"
                         "It is being used by another command.\n"
                         "Try again.") { }
  };

  // This class acts like a wrapper for the given document.  It's
  // specialized by DocReader/Writer to handle document read/write
  // locks.
  class DocAccess {
  public:
    DocAccess() : m_doc(NULL) { }
    DocAccess(const DocAccess& copy) : m_doc(copy.m_doc) { }
    explicit DocAccess(Doc* doc) : m_doc(doc) { }
    ~DocAccess() { }

    DocAccess& operator=(const DocAccess& copy) {
      m_doc = copy.m_doc;
      return *this;
    }

    operator Doc*() { return m_doc; }
    operator const Doc*() const { return m_doc; }

    Doc* operator->() {
      ASSERT(m_doc);
      return m_doc;
    }

    const Doc* operator->() const {
      ASSERT(m_doc);
      return m_doc;
    }

  protected:
    Doc* m_doc;
  };

  // Class to view the document's state. Its constructor request a
  // reader-lock of the document, or throws an exception in case that
  // the lock cannot be obtained.
  class DocReader : public DocAccess {
  public:
    DocReader() {
    }

    explicit DocReader(Doc* doc, int timeout)
      : DocAccess(doc) {
      if (m_doc && !m_doc->lock(Doc::ReadLock, timeout))
        throw CannotReadDocException();
    }

    explicit DocReader(const DocReader& copy, int timeout)
      : DocAccess(copy) {
      if (m_doc && !m_doc->lock(Doc::ReadLock, timeout))
        throw CannotReadDocException();
    }

    ~DocReader() {
      unlock();
    }

  protected:
    void unlock() {
      if (m_doc) {
        m_doc->unlock();
        m_doc = nullptr;
      }
    }

  private:
    // Disable operator=
    DocReader& operator=(const DocReader&);
  };

  // Class to modify the document's state. Its constructor request a
  // writer-lock of the document, or throws an exception in case that
  // the lock cannot be obtained. Also, it contains a special
  // constructor that receives a DocReader, to elevate the
  // reader-lock to writer-lock.
  class DocWriter : public DocAccess {
  public:
    DocWriter()
      : m_from_reader(false)
      , m_locked(false) {
    }

    explicit DocWriter(Doc* doc, int timeout)
      : DocAccess(doc)
      , m_from_reader(false)
      , m_locked(false) {
      if (m_doc) {
        if (!m_doc->lock(Doc::WriteLock, timeout))
          throw CannotWriteDocException();

        m_locked = true;
      }
    }

    // Constructor that can be used to elevate the given reader-lock to
    // writer permission.
    explicit DocWriter(const DocReader& doc, int timeout)
      : DocAccess(doc)
      , m_from_reader(true)
      , m_locked(false) {
      if (m_doc) {
        if (!m_doc->upgradeToWrite(timeout))
          throw CannotWriteDocException();

        m_locked = true;
      }
    }

    ~DocWriter() {
      unlock();
    }

  protected:
    void unlock() {
      if (m_doc && m_locked) {
        if (m_from_reader)
          m_doc->downgradeToRead();
        else
          m_doc->unlock();

        m_doc = nullptr;
        m_locked = false;
      }
    }

  private:
    bool m_from_reader;
    bool m_locked;

    // Non-copyable
    DocWriter(const DocWriter&);
    DocWriter& operator=(const DocWriter&);
    DocWriter& operator=(const DocReader&);
  };

  // Used to destroy the active document in the context.
  class DocDestroyer : public DocWriter {
  public:
    explicit DocDestroyer(Context* context, Doc* doc, int timeout)
      : DocWriter(doc, timeout) {
    }

    void destroyDocument() {
      ASSERT(m_doc != nullptr);

      // Don't create a backup for destroyed documents (e.g. documents
      // are destroyed when they are used internally by Aseprite or by
      // a script and then closed with Sprite:close())
      if (m_doc->needsBackup())
        m_doc->setInhibitBackup(true);

      m_doc->close();
      Doc* doc = m_doc;
      unlock();

      delete doc;
      m_doc = nullptr;
    }

    void closeDocument() {
      ASSERT(m_doc != nullptr);

      Context* ctx = (Context*)m_doc->context();
      m_doc->close();
      Doc* doc = m_doc;
      unlock();

      ctx->closeDocument(doc);
      m_doc = nullptr;
    }

  };

  class WeakDocReader : public DocAccess {
  public:
    WeakDocReader() {
    }

    explicit WeakDocReader(Doc* doc)
      : DocAccess(doc)
      , m_weak_lock(base::RWLock::WeakUnlocked) {
      if (m_doc)
        m_doc->weakLock(&m_weak_lock);
    }

    ~WeakDocReader() {
      weakUnlock();
    }

    bool isLocked() const {
      return (m_weak_lock == base::RWLock::WeakLocked);
    }

  protected:
    void weakUnlock() {
      if (m_doc && m_weak_lock != base::RWLock::WeakUnlocked) {
        m_doc->weakUnlock();
        m_doc = nullptr;
      }
    }

  private:
    // Disable operator=
    WeakDocReader(const WeakDocReader&);
    WeakDocReader& operator=(const WeakDocReader&);

    base::RWLock::WeakLock m_weak_lock;
  };

} // namespace app

#endif
