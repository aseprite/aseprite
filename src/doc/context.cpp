// Aseprite Document Library
// Copyright (c) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/context.h"

#include "base/debug.h"
#include "doc/site.h"

namespace doc {

Context::Context()
  : m_docs(this)
  , m_activeDoc(NULL)
{
  m_docs.add_observer(this);
}

Context::~Context()
{
  m_docs.remove_observer(this);
}

Site Context::activeSite() const
{
  Site site;
  onGetActiveSite(&site);
  return site;
}

Document* Context::activeDocument() const
{
  Site site;
  onGetActiveSite(&site);
  return site.document();
}

void Context::notifyActiveSiteChanged()
{
  Site site = activeSite();
  notify_observers<const Site&>(&ContextObserver::onActiveSiteChange, site);
}

void Context::onGetActiveSite(Site* site) const
{
  ASSERT(false);
}

void Context::onAddDocument(Document* doc)
{
  // Do nothing
}

void Context::onRemoveDocument(Document* doc)
{
  // Do nothing
}

} // namespace doc
