/* ASEPRITE
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

#include "config.h"

#include "thumbnail_generator.h"

#include "app.h"
#include "base/bind.h"
#include "base/scoped_lock.h"
#include "base/thread.h"
#include "document.h"
#include "file/file.h"
#include "file_system.h"
#include "raster/image.h"
#include "raster/palette.h"
#include "raster/rotate.h"
#include "raster/sprite.h"

#include <allegro.h>

#define MAX_THUMBNAIL_SIZE              128

class ThumbnailGenerator::Worker
{
public:
  Worker(FileOp* fop, IFileItem* fileitem)
    : m_fop(fop)
    , m_fileitem(fileitem)
    , m_thumbnail(NULL)
    , m_palette(NULL)
    , m_thumbnailBitmap(NULL)
    , m_thread(Bind<void>(&Worker::loadBgThread, this)) {
  }

  ~Worker() {
    fop_stop(m_fop);
    m_thread.join();

    fop_free(m_fop);
  }

  IFileItem* getFileItem() { return m_fileitem; }
  bool isDone() const { return fop_is_done(m_fop); }
  double getProgress() const { return fop_get_progress(m_fop); }

private:
  void loadBgThread() {
    try {
      fop_operate(m_fop, NULL);

      // Post load
      fop_post_load(m_fop);

      // Convert the loaded document into the Allegro bitmap "m_thumbnail".
      const Sprite* sprite = (m_fop->document && m_fop->document->getSprite()) ? m_fop->document->getSprite():
                                                                                 NULL;
      if (!fop_is_stop(m_fop) && sprite) {
        // The palette to convert the Image to a BITMAP
        m_palette.reset(new Palette(*sprite->getPalette(FrameNumber(0))));

        // Render the 'sprite' in one plain 'image'
        UniquePtr<Image> image(Image::create(sprite->getPixelFormat(),
                                             sprite->getWidth(),
                                             sprite->getHeight()));
        sprite->render(image, 0, 0);

        // Calculate the thumbnail size
        int thumb_w = MAX_THUMBNAIL_SIZE * image->w / MAX(image->w, image->h);
        int thumb_h = MAX_THUMBNAIL_SIZE * image->h / MAX(image->w, image->h);
        if (MAX(thumb_w, thumb_h) > MAX(image->w, image->h)) {
          thumb_w = image->w;
          thumb_h = image->h;
        }
        thumb_w = MID(1, thumb_w, MAX_THUMBNAIL_SIZE);
        thumb_h = MID(1, thumb_h, MAX_THUMBNAIL_SIZE);

        // Stretch the 'image'
        m_thumbnail.reset(Image::create(image->getPixelFormat(), thumb_w, thumb_h));
        image_clear(m_thumbnail, 0);
        image_scale(m_thumbnail, image, 0, 0, thumb_w, thumb_h);
      }

      delete m_fop->document;

      // Set the thumbnail of the file-item.
      if (m_thumbnail) {
        BITMAP* bmp = create_bitmap_ex(16, m_thumbnail->w, m_thumbnail->h);
        image_to_allegro(m_thumbnail, bmp, 0, 0, m_palette);
        m_fileitem->setThumbnail(bmp);
      }
    }
    catch (const std::exception& e) {
      fop_error(m_fop, "Error loading file:\n%s", e.what());
    }
    fop_done(m_fop);
  }

  FileOp* m_fop;
  IFileItem* m_fileitem;
  UniquePtr<Image> m_thumbnail;
  BITMAP* m_thumbnailBitmap;
  UniquePtr<Palette> m_palette;
  base::thread m_thread;
};

static void delete_singleton(ThumbnailGenerator* singleton)
{
  delete singleton;
}

ThumbnailGenerator* ThumbnailGenerator::instance()
{
  static ThumbnailGenerator* singleton = NULL;
  if (singleton == NULL) {
    singleton = new ThumbnailGenerator();
    App::instance()->Exit.connect(Bind<void>(&delete_singleton, singleton));
  }
  return singleton;
}

ThumbnailGenerator::WorkerStatus ThumbnailGenerator::getWorkerStatus(IFileItem* fileitem, double& progress)
{
  ScopedLock hold(m_workersAccess);

  for (WorkerList::iterator
         it=m_workers.begin(), end=m_workers.end(); it!=end; ++it) {
    Worker* worker = *it;
    if (worker->getFileItem() == fileitem) {
      if (worker->isDone())
        return ThumbnailIsDone;
      else {
        progress = worker->getProgress();
        return WorkingOnThumbnail;
      }
    }
  }
  return WithoutWorker;
}

bool ThumbnailGenerator::checkWorkers()
{
  ScopedLock hold(m_workersAccess);
  bool doingWork = !m_workers.empty();

  for (WorkerList::iterator
         it=m_workers.begin(); it != m_workers.end(); ) {
    if ((*it)->isDone()) {
      delete *it;
      it = m_workers.erase(it);
    }
    else {
      ++it;
    }
  }

  return doingWork;
}

void ThumbnailGenerator::addWorkerToGenerateThumbnail(IFileItem* fileitem)
{
  double progress;

  if (fileitem->isBrowsable() ||
      fileitem->getThumbnail() != NULL ||
      getWorkerStatus(fileitem, progress) != WithoutWorker)
    return;

  FileOp* fop = fop_to_load_document(fileitem->getFileName().c_str(),
                                     FILE_LOAD_SEQUENCE_NONE |
                                     FILE_LOAD_ONE_FRAME);
  if (!fop)
    return;

  if (fop->has_error()) {
    fop_free(fop);
  }
  else {
    Worker* worker = new Worker(fop, fileitem);
    try {
      ScopedLock hold(m_workersAccess);
      m_workers.push_back(worker);
    }
    catch (...) {
      delete worker;
      throw;
    }
  }
}

void ThumbnailGenerator::stopAllWorkers()
{
  base::thread* ptr = new base::thread(Bind<void>(&ThumbnailGenerator::stopAllWorkersBackground, this));
  m_stopThread.reset(ptr);
}

void ThumbnailGenerator::stopAllWorkersBackground()
{
  WorkerList workersCopy;
  {
    ScopedLock hold(m_workersAccess);
    workersCopy = m_workers;
    m_workers.clear();
  }

  for (WorkerList::iterator
         it=workersCopy.begin(), end=workersCopy.end(); it!=end; ++it) {
    delete *it;
  }
}
