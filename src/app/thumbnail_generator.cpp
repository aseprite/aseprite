// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/thumbnail_generator.h"

#include "app/app.h"
#include "app/doc.h"
#include "app/file/file.h"
#include "app/file_system.h"
#include "app/ui/editor/editor_render.h"
#include "base/bind.h"
#include "base/scoped_lock.h"
#include "base/scoped_lock.h"
#include "base/thread.h"
#include "doc/algorithm/rotate.h"
#include "doc/conversion_to_surface.h"
#include "doc/image.h"
#include "doc/palette.h"
#include "doc/primitives.h"
#include "doc/sprite.h"
#include "os/system.h"
#include "render/projection.h"

#include <memory>
#include <thread>

#define MAX_THUMBNAIL_SIZE   128
#define THUMB_TRACE(...)

namespace app {

class ThumbnailGenerator::Worker {
public:
  Worker(base::concurrent_queue<ThumbnailGenerator::Item>& queue)
    : m_queue(queue)
    , m_fop(nullptr)
    , m_isDone(false)
    , m_thread(base::Bind<void>(&Worker::loadBgThread, this)) {
  }

  ~Worker() {
    {
      base::scoped_lock lock(m_mutex);
      if (m_fop)
        m_fop->stop();
    }
    m_thread.join();
  }

  void stop() const {
    base::scoped_lock lock(m_mutex);
    if (m_fop)
      m_fop->stop();
  }

  bool isDone() const {
    return m_isDone;
  }

  void updateProgress() {
    base::scoped_lock lock(m_mutex);
    if (m_item.fileitem && m_item.fop) {
      double progress = m_item.fop->progress();
      if (progress > m_item.fileitem->getThumbnailProgress())
        m_item.fileitem->setThumbnailProgress(progress);
    }
  }

private:
  void loadItem() {
    ASSERT(!m_fop);
    try {
      {
        base::scoped_lock lock(m_mutex);
        m_fop = m_item.fop;
        ASSERT(m_fop);
      }

      THUMB_TRACE("FOP loading thumbnail: %s\n",
                  m_item.fileitem->fileName().c_str());

      // Load the file
      m_fop->operate(nullptr);

      // Post load
      m_fop->postLoad();

      // Convert the loaded document into the os::Surface.
      const Sprite* sprite =
        (m_fop->document() &&
         m_fop->document()->sprite() ?
         m_fop->document()->sprite(): nullptr);

      std::unique_ptr<Image> thumbnailImage;
      std::unique_ptr<Palette> palette;
      if (!m_fop->isStop() && sprite) {
        // The palette to convert the Image
        palette.reset(new Palette(*sprite->palette(frame_t(0))));

        const int w = sprite->width()*sprite->pixelRatio().w;
        const int h = sprite->height()*sprite->pixelRatio().h;

        // Calculate the thumbnail size
        int thumb_w = MAX_THUMBNAIL_SIZE * w / MAX(w, h);
        int thumb_h = MAX_THUMBNAIL_SIZE * h / MAX(w, h);
        if (MAX(thumb_w, thumb_h) > MAX(w, h)) {
          thumb_w = w;
          thumb_h = h;
        }
        thumb_w = MID(1, thumb_w, MAX_THUMBNAIL_SIZE);
        thumb_h = MID(1, thumb_h, MAX_THUMBNAIL_SIZE);

        // Stretch the 'image'
        thumbnailImage.reset(
          Image::create(
            sprite->pixelFormat(), thumb_w, thumb_h));

        render::Projection proj(sprite->pixelRatio(),
                                render::Zoom(thumb_w, w));
        EditorRender render;
        render.setupBackground(NULL, thumbnailImage->pixelFormat());
        render.setProjection(proj);
        render.renderSprite(
          thumbnailImage.get(), sprite, frame_t(0),
          gfx::Clip(0, 0, 0, 0, w, h));
      }

      // Close file
      delete m_fop->releaseDocument();

      // Set the thumbnail of the file-item.
      if (thumbnailImage) {
        os::Surface* thumbnail = os::instance()->createRgbaSurface(
          thumbnailImage->width(),
          thumbnailImage->height());

        convert_image_to_surface(
          thumbnailImage.get(), palette.get(), thumbnail,
          0, 0, 0, 0, thumbnailImage->width(), thumbnailImage->height());

        m_item.fileitem->setThumbnail(thumbnail);
      }

      THUMB_TRACE("FOP done with thumbnail: %s %s\n",
                  m_item.fileitem->fileName().c_str(),
                  (m_fop->isStop() ? " (stop)": ""));

      // Reset the m_item (first the fileitem so this worker is not
      // associated to this fileitem anymore, and then the FileOp).
      m_item.fileitem = nullptr;
    }
    catch (const std::exception& e) {
      m_fop->setError("Error loading file:\n%s", e.what());
    }
    m_fop->done();
    {
      base::scoped_lock lock(m_mutex);
      m_item.fop = nullptr;
      delete m_fop;
      m_fop = nullptr;
    }
    ASSERT(!m_fop);
  }

  void loadBgThread() {
    while (!m_queue.empty()) {
      while (m_queue.try_pop(m_item)) {
        loadItem();
      }
      base::this_thread::yield();
    }
    m_isDone = true;
  }

  base::concurrent_queue<Item>& m_queue;
  app::ThumbnailGenerator::Item m_item;
  FileOp* m_fop;
  mutable base::mutex m_mutex;
  bool m_isDone;
  base::thread m_thread;
};

static void delete_singleton(ThumbnailGenerator* singleton)
{
  delete singleton;
}

ThumbnailGenerator* ThumbnailGenerator::instance()
{
  static ThumbnailGenerator* singleton = nullptr;
  if (singleton == NULL) {
    singleton = new ThumbnailGenerator();
    App::instance()->Exit.connect(base::Bind<void>(&delete_singleton, singleton));
  }
  return singleton;
}

bool ThumbnailGenerator::checkWorkers()
{
  base::scoped_lock hold(m_workersAccess);
  bool doingWork = (!m_workers.empty());

  for (WorkerList::iterator
         it=m_workers.begin(); it != m_workers.end(); ) {
    Worker* worker = *it;
    worker->updateProgress();
    if (worker->isDone()) {
      delete worker;
      it = m_workers.erase(it);
    }
    else {
      ++it;
    }
  }

  return doingWork;
}

void ThumbnailGenerator::generateThumbnail(IFileItem* fileitem)
{
  if (fileitem->isBrowsable() ||
      fileitem->getThumbnail())
    return;

  if (fileitem->getThumbnailProgress() > 0.0) {
    if (fileitem->getThumbnailProgress() < 0.0002) {
      m_remainingItems.prioritize(
        [fileitem](const Item& item) {
          return (item.fileitem == fileitem);
        });
    }
    return;
  }

  // Set a starting progress so we don't enqueue the same item two times.
  fileitem->setThumbnailProgress(0.0001);

  THUMB_TRACE("Queue FOP thumbnail for %s\n",
              fileitem->fileName().c_str());

  std::unique_ptr<FileOp> fop(
    FileOp::createLoadDocumentOperation(
      nullptr,
      fileitem->fileName().c_str(),
      FILE_LOAD_SEQUENCE_NONE |
      FILE_LOAD_ONE_FRAME));
  if (!fop)
    return;

  if (fop->hasError())
    return;

  m_remainingItems.push(Item(fileitem, fop.get()));
  fop.release();

  int n = std::thread::hardware_concurrency()-1;
  if (n < 1) n = 1;
  if (m_workers.size() < n) {
    Worker* worker = new Worker(m_remainingItems);
    try {
      base::scoped_lock hold(m_workersAccess);
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
  Item item;
  while (!m_remainingItems.empty()) {
    while (m_remainingItems.try_pop(item)) {
      if (!item.fileitem->getThumbnail()) {
        // Reset progress to 0.0 because the FileOp wasn't used and we
        // will need to create it again if we require this FileItem
        // thumbnail again.
        item.fileitem->setThumbnailProgress(0.0);
      }
      delete item.fop;
    }
  }

  base::scoped_lock hold(m_workersAccess);
  for (auto worker : m_workers)
    worker->stop();
}

} // namespace app
