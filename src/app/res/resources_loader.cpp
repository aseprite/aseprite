// Aseprite
// Copyright (C) 2001-2016  David Capello
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
#include "base/bind.h"
#include "base/fs.h"
#include "base/scoped_value.h"

namespace app {

ResourcesLoader::ResourcesLoader(ResourcesLoaderDelegate* delegate)
  : m_delegate(delegate)
  , m_done(false)
  , m_cancel(false)
  , m_thread(base::Bind<void>(&ResourcesLoader::threadLoadResources, this))
{
}

ResourcesLoader::~ResourcesLoader()
{
  m_thread.join();
}

void ResourcesLoader::cancel()
{
  m_cancel = true;
}

bool ResourcesLoader::next(base::UniquePtr<Resource>& resource)
{
  Resource* rawResource;
  if (m_queue.try_pop(rawResource)) {
    resource.reset(rawResource);
    return true;
  }
  else
    return false;
}

void ResourcesLoader::threadLoadResources()
{
  base::ScopedValue<bool> scoped(m_done, false, true);

  std::string path = m_delegate->resourcesLocation();
  TRACE("RESLOAD: Loading resources from %s...\n", path.c_str());
  if (path.empty())
    return;

  FileSystemModule* fs = FileSystemModule::instance();
  LockFS lock(fs);

  IFileItem* item = fs->getFileItemFromPath(path);
  if (!item)
    return;

  FileItemList list = item->children();
  for (auto child : list) {
    if (m_cancel)
      break;

    Resource* resource = m_delegate->loadResource((child)->fileName());
    if (resource)
      m_queue.push(resource);
  }
}

} // namespace app
