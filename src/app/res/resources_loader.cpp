// Aseprite
// Copyright (C) 2020-2023  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/res/resources_loader.h"

#include "app/file_system.h"
#include "app/res/resource.h"
#include "app/res/resources_loader_delegate.h"
#include "app/resource_finder.h"
#include "base/fs.h"
#include "base/scoped_value.h"

namespace app {

ResourcesLoader::ResourcesLoader(std::unique_ptr<ResourcesLoaderDelegate>&& delegate)
  : m_delegate(std::move(delegate))
  , m_done(false)
  , m_cancel(false)
  , m_thread(new std::thread([this]{ threadLoadResources(); }))
{
}

ResourcesLoader::~ResourcesLoader()
{
  if (m_thread)
    m_thread->join();
}

void ResourcesLoader::cancel()
{
  m_cancel = true;
}

bool ResourcesLoader::next(std::unique_ptr<Resource>& resource)
{
  Resource* rawResource;
  if (m_queue.try_pop(rawResource)) {
    resource.reset(rawResource);
    return true;
  }
  else
    return false;
}

void ResourcesLoader::reload()
{
  if (m_thread) {
    m_thread->join();
    m_thread.reset(nullptr);
  }

  m_thread.reset(createThread());
}

void ResourcesLoader::threadLoadResources()
{
  base::ScopedValue scoped(m_done, false, true);

  // Load resources from extensions
  std::map<std::string, std::string> idAndPaths;
  m_delegate->getResourcesPaths(idAndPaths);
  for (const auto& idAndPath : idAndPaths) {
    if (m_cancel)
      break;

    TRACE("RESLOAD: Loading resource '%s' from '%s'...\n",
          idAndPath.first.c_str(),
          idAndPath.second.c_str());

    Resource* resource =
      m_delegate->loadResource(idAndPath.first,
                               idAndPath.second);
    if (resource)
      m_queue.push(resource);
  }
}

std::thread* ResourcesLoader::createThread()
{
  return new std::thread([this]{ threadLoadResources(); });
}

} // namespace app
