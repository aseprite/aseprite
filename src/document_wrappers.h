/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2011  David Capello
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

#ifndef DOCUMENT_WRAPPERS_H_INCLUDED
#define DOCUMENT_WRAPPERS_H_INCLUDED

#include "base/exception.h"
#include "context.h"
#include "document.h"

#include <exception>

class LockedDocumentException : public base::Exception
{
public:
  LockedDocumentException() throw()
  : base::Exception("Cannot read or write the active document.\n"
		    "It is locked by a background task.\n"
		    "Try again later.") { }
};

//////////////////////////////////////////////////////////////////////

// Wraps a document access.
class DocumentWrapper
{
public:
  DocumentWrapper() : m_document(NULL) { }
  DocumentWrapper(const DocumentWrapper& copy) : m_document(copy.m_document) { }
  explicit DocumentWrapper(Document* document) : m_document(document) { }
  ~DocumentWrapper() { }

  DocumentWrapper& operator=(const DocumentWrapper& copy) 
  {
    m_document = copy.m_document;
    return *this;
  }

  operator Document* () { return m_document; }
  operator const Document* () const { return m_document; }

  Document* operator->() 
  {
    ASSERT(m_document != NULL);
    return m_document;
  }

  const Document* operator->() const 
  {
    ASSERT(m_document != NULL);
    return m_document;
  }

protected:
  Document* m_document;
};

// Class to view the document's state.
class DocumentReader : public DocumentWrapper
{
public:

  DocumentReader()
  {
  }

  explicit DocumentReader(Document* document)
    : DocumentWrapper(document)
  {
    if (m_document && !m_document->lock(Document::ReadLock))
      throw LockedDocumentException();
  }

  explicit DocumentReader(const DocumentReader& copy)
    : DocumentWrapper(copy)
  {
    if (m_document && !m_document->lock(Document::ReadLock))
      throw LockedDocumentException();
  }

  DocumentReader& operator=(const DocumentReader& copy)
  {
    // unlock old document
    if (m_document)
      m_document->unlock();

    DocumentWrapper::operator=(copy);

    // relock the document
    if (m_document && !m_document->lock(Document::ReadLock))
      throw LockedDocumentException();

    return *this;
  }

  ~DocumentReader()
  {
    // unlock the document
    if (m_document)
      m_document->unlock();
  }

};

// Class to modify the document's state.
class DocumentWriter : public DocumentWrapper
{
public:

  DocumentWriter()
    : m_from_reader(false)
    , m_locked(false)
  {
  }

  explicit DocumentWriter(Document* document)
    : DocumentWrapper(document)
    , m_from_reader(false)
    , m_locked(false)
  {
    if (m_document) {
      if (!m_document->lock(Document::WriteLock))
	throw LockedDocumentException();

      m_locked = true;
    }
  }

  explicit DocumentWriter(const DocumentReader& document)
    : DocumentWrapper(document)
    , m_from_reader(true)
    , m_locked(false)
  {
    if (m_document) {
      if (!m_document->lockToWrite())
	throw LockedDocumentException();

      m_locked = true;
    }
  }

  ~DocumentWriter() 
  {
    unlockWriter();
  }

  DocumentWriter& operator=(const DocumentReader& copy)
  {
    unlockWriter();

    DocumentWrapper::operator=(copy);

    if (m_document) {
      m_from_reader = true;

      if (!m_document->lockToWrite())
	throw LockedDocumentException();

      m_locked = true;
    }

    return *this;
  }

protected:

  void unlockWriter()
  {
    if (m_document && m_locked) {
      if (m_from_reader)
	m_document->unlockToRead();
      else
	m_document->unlock();
      m_locked = false;
    }
  }

private:
  bool m_from_reader;
  bool m_locked;

  // Non-copyable
  DocumentWriter(const DocumentWriter&);
  DocumentWriter& operator=(const DocumentWriter&);
};

class ActiveDocumentReader : public DocumentReader
{
public:

  explicit ActiveDocumentReader(Context* context)
    : DocumentReader(context->getActiveDocument())
  {
  }

  ~ActiveDocumentReader() 
  {
  }

};

class ActiveDocumentWriter : public DocumentWriter
{
public:

  explicit ActiveDocumentWriter(Context* context)
    : DocumentWriter(context->getActiveDocument())
    , m_context(context)
  {
  }

  ~ActiveDocumentWriter() 
  {
  }

  void deleteDocument()
  {
    ASSERT(m_document != NULL);

    m_context->removeDocument(m_document);
    unlockWriter();

    delete m_document;
    m_document = NULL;
  }

private:
  Context* m_context;
};

#endif
