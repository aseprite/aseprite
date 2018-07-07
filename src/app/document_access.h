// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_DOCUMENT_ACCESS_H_INCLUDED
#define APP_DOCUMENT_ACCESS_H_INCLUDED
#pragma once

#include "app/context.h"
#include "app/doc.h"
#include "base/exception.h"

#include <exception>

namespace app {

  // TODO remove exceptions and use "DocumentAccess::operator bool()"
  class LockedDocumentException : public base::Exception {
  public:
    LockedDocumentException(const char* msg) throw()
    : base::Exception(msg) { }
  };

  class CannotReadDocumentException : public LockedDocumentException {
  public:
    CannotReadDocumentException() throw()
    : LockedDocumentException("Cannot read the sprite.\n"
                              "It is being modified by another command.\n"
                              "Try again.") { }
  };

  class CannotWriteDocumentException : public LockedDocumentException {
  public:
    CannotWriteDocumentException() throw()
    : LockedDocumentException("Cannot modify the sprite.\n"
                              "It is being used by another command.\n"
                              "Try again.") { }
  };

  // This class acts like a wrapper for the given document.  It's
  // specialized by DocumentReader/Writer to handle document read/write
  // locks.
  class DocumentAccess {
  public:
    DocumentAccess() : m_document(NULL) { }
    DocumentAccess(const DocumentAccess& copy) : m_document(copy.m_document) { }
    explicit DocumentAccess(Doc* document) : m_document(document) { }
    ~DocumentAccess() { }

    DocumentAccess& operator=(const DocumentAccess& copy)
    {
      m_document = copy.m_document;
      return *this;
    }

    operator Doc* () { return m_document; }
    operator const Doc* () const { return m_document; }

    Doc* operator->()
    {
      ASSERT(m_document != NULL);
      return m_document;
    }

    const Doc* operator->() const
    {
      ASSERT(m_document != NULL);
      return m_document;
    }

  protected:
    Doc* m_document;
  };

  // Class to view the document's state. Its constructor request a
  // reader-lock of the document, or throws an exception in case that
  // the lock cannot be obtained.
  class DocumentReader : public DocumentAccess {
  public:
    DocumentReader() {
    }

    explicit DocumentReader(Doc* document, int timeout)
      : DocumentAccess(document) {
      if (m_document && !m_document->lock(Doc::ReadLock, timeout))
        throw CannotReadDocumentException();
    }

    explicit DocumentReader(const DocumentReader& copy, int timeout)
      : DocumentAccess(copy) {
      if (m_document && !m_document->lock(Doc::ReadLock, timeout))
        throw CannotReadDocumentException();
    }

    ~DocumentReader() {
      unlock();
    }

  protected:
    void unlock() {
      if (m_document) {
        m_document->unlock();
        m_document = nullptr;
      }
    }

  private:
    // Disable operator=
    DocumentReader& operator=(const DocumentReader&);
  };

  // Class to modify the document's state. Its constructor request a
  // writer-lock of the document, or throws an exception in case that
  // the lock cannot be obtained. Also, it contains a special
  // constructor that receives a DocumentReader, to elevate the
  // reader-lock to writer-lock.
  class DocumentWriter : public DocumentAccess {
  public:
    DocumentWriter()
      : m_from_reader(false)
      , m_locked(false) {
    }

    explicit DocumentWriter(Doc* document, int timeout)
      : DocumentAccess(document)
      , m_from_reader(false)
      , m_locked(false) {
      if (m_document) {
        if (!m_document->lock(Doc::WriteLock, timeout))
          throw CannotWriteDocumentException();

        m_locked = true;
      }
    }

    // Constructor that can be used to elevate the given reader-lock to
    // writer permission.
    explicit DocumentWriter(const DocumentReader& document, int timeout)
      : DocumentAccess(document)
      , m_from_reader(true)
      , m_locked(false) {
      if (m_document) {
        if (!m_document->upgradeToWrite(timeout))
          throw CannotWriteDocumentException();

        m_locked = true;
      }
    }

    ~DocumentWriter() {
      unlock();
    }

  protected:
    void unlock() {
      if (m_document && m_locked) {
        if (m_from_reader)
          m_document->downgradeToRead();
        else
          m_document->unlock();

        m_document = nullptr;
        m_locked = false;
      }
    }

  private:
    bool m_from_reader;
    bool m_locked;

    // Non-copyable
    DocumentWriter(const DocumentWriter&);
    DocumentWriter& operator=(const DocumentWriter&);
    DocumentWriter& operator=(const DocumentReader&);
  };

  // Used to destroy the active document in the context.
  class DocumentDestroyer : public DocumentWriter {
  public:
    explicit DocumentDestroyer(Context* context, Doc* document, int timeout)
      : DocumentWriter(document, timeout) {
    }

    void destroyDocument() {
      ASSERT(m_document != NULL);

      m_document->close();
      Doc* doc = m_document;
      unlock();

      delete doc;
      m_document = nullptr;
    }

  };

  class WeakDocumentReader : public DocumentAccess {
  public:
    WeakDocumentReader() {
    }

    explicit WeakDocumentReader(Doc* doc)
      : DocumentAccess(doc)
      , m_weak_lock(base::RWLock::WeakUnlocked) {
      if (m_document)
        m_document->weakLock(&m_weak_lock);
    }

    ~WeakDocumentReader() {
      weakUnlock();
    }

    bool isLocked() const {
      return (m_weak_lock == base::RWLock::WeakLocked);
    }

  protected:
    void weakUnlock() {
      if (m_document && m_weak_lock != base::RWLock::WeakUnlocked) {
        m_document->weakUnlock();
        m_document = nullptr;
      }
    }

  private:
    // Disable operator=
    WeakDocumentReader(const WeakDocumentReader&);
    WeakDocumentReader& operator=(const WeakDocumentReader&);

    base::RWLock::WeakLock m_weak_lock;
  };

} // namespace app

#endif
