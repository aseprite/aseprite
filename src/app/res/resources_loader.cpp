/* Aseprite
 * Copyright (C) 2014  David Capello
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
#include "base/path.h"
#include "base/scoped_value.h"

namespace app {

ResourcesLoader::ResourcesLoader(ResourcesLoaderDelegate* delegate)
  : m_delegate(delegate)
  , m_done(false)
  , m_cancel(false)
  , m_thread(Bind<void>(&ResourcesLoader::threadLoadResources, this))
{
  PRINTF("ResourcesLoader::ResourcesLoader()\n");
}

ResourcesLoader::~ResourcesLoader()
{
  m_thread.join();

  PRINTF("ResourcesLoader::~ResourcesLoader()\n");
}

void ResourcesLoader::cancel()
{
  m_cancel = true;
}

bool ResourcesLoader::done()
{
  return m_done;
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
  PRINTF("threadLoadResources()\n");

  base::ScopedValue<bool> scoped(m_done, false, true);

  std::string path = m_delegate->resourcesLocation();
  PRINTF("Loading resources from %s...\n", path.c_str());
  if (path.empty())
    return;

  FileSystemModule* fs = FileSystemModule::instance();
  LockFS lock(fs);

  IFileItem* item = fs->getFileItemFromPath(path);
  if (!item)
    return;

  FileItemList list = item->getChildren();
  for (FileItemList::iterator it=list.begin(), end=list.end();
       it != end; ++it) {
    if (m_cancel)
      break;

    Resource* resource = m_delegate->loadResource((*it)->getFileName());
    if (resource)
      m_queue.push(resource);
  }
}

} // namespace app
