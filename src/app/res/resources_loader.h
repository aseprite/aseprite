// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_RES_RESOURCES_LOADER_H_INCLUDED
#define APP_RES_RESOURCES_LOADER_H_INCLUDED
#pragma once

#include "base/concurrent_queue.h"
#include "base/thread.h"

#include <memory>

namespace app {

  class Resource;
  class ResourcesLoaderDelegate;

  class ResourcesLoader {
  public:
    ResourcesLoader(ResourcesLoaderDelegate* delegate);
    ~ResourcesLoader();

    void cancel();
    bool isDone() const { return m_done; }
    bool next(std::unique_ptr<Resource>& resource);
    void reload();

  private:
    void threadLoadResources();
    base::thread* createThread();

    typedef base::concurrent_queue<Resource*> Queue;

    ResourcesLoaderDelegate* m_delegate;
    bool m_done;
    bool m_cancel;
    Queue m_queue;
    std::unique_ptr<base::thread> m_thread;
  };

} // namespace app

#endif
