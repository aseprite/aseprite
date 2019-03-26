// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_THUMBNAIL_GENERATOR_H_INCLUDED
#define APP_THUMBNAIL_GENERATOR_H_INCLUDED
#pragma once

#include "base/concurrent_queue.h"
#include "base/mutex.h"

#include <memory>
#include <vector>

namespace base {
  class thread;
}

namespace app {
  class FileOp;
  class IFileItem;

  class ThumbnailGenerator {
    ThumbnailGenerator();
  public:
    static ThumbnailGenerator* instance();

    // Generate a thumbnail for the given file-item.  It must be called
    // from the GUI thread.
    void generateThumbnail(IFileItem* fileitem);

    // Checks the status of workers. If there are workers that already
    // done its job, we've to destroy them. This function must be called
    // from the GUI thread (because a thread is joint to it).
    // Returns true if there are workers generating thumbnails.
    bool checkWorkers();

    // Stops all workers generating thumbnails. This is an non-blocking
    // operation. The cancelation of all workers is done in a background
    // thread.
    void stopAllWorkers();

  private:
    void startWorker();

    class Worker;
    typedef std::vector<Worker*> WorkerList;

    struct Item {
      IFileItem* fileitem;
      FileOp* fop;
      Item() : fileitem(nullptr), fop(nullptr) { }
      Item(const Item& item) : fileitem(item.fileitem), fop(item.fop) { }
      Item(IFileItem* fileitem, FileOp* fop)
        : fileitem(fileitem), fop(fop) {
      }
    };

    int m_maxWorkers;
    WorkerList m_workers;
    base::mutex m_workersAccess;
    std::unique_ptr<base::thread> m_stopThread;
    base::concurrent_queue<Item> m_remainingItems;
  };

} // namespace app

#endif
