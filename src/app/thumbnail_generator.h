/* Aseprite
 * Copyright (C) 2001-2013  David Capello
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

#ifndef APP_THUMBNAIL_GENERATOR_H_INCLUDED
#define APP_THUMBNAIL_GENERATOR_H_INCLUDED

#include "base/mutex.h"
#include "base/unique_ptr.h"

#include <vector>

namespace base {
  class thread;
}

namespace app {
  class IFileItem;

  class ThumbnailGenerator {
  public:
    enum WorkerStatus { WithoutWorker, WorkingOnThumbnail, ThumbnailIsDone };

    static ThumbnailGenerator* instance();

    // Generate a thumbnail for the given file-item.  It must be called
    // from the GUI thread.
    void addWorkerToGenerateThumbnail(IFileItem* fileitem);

    // Returns the status of the worker that is generating the thumbnail
    // for the given file.
    WorkerStatus getWorkerStatus(IFileItem* fileitem, double& progress);

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
    void stopAllWorkersBackground();

    class Worker;
    typedef std::vector<Worker*> WorkerList;

    WorkerList m_workers;
    base::mutex m_workersAccess;
    base::UniquePtr<base::thread> m_stopThread;
  };
} // namespace app

#endif
