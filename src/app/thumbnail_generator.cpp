// Aseprite
// Copyright (C) 2019-2020  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/thumbnail_generator.h"

#include "app/app.h"
#include "app/cmd/convert_color_profile.h"
#include "app/doc.h"
#include "app/file/file.h"
#include "app/file_system.h"
#include "base/bind.h"
#include "base/clamp.h"
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
#include "render/render.h"

#include <algorithm>
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

      // Don't call post-load because postLoad() needs user interaction.
      //m_fop->postLoad();

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

        // Special case for indexed images:
        // If the sprite is transparent -> set the transparent color index alpha = 0
        if (sprite->colorMode() == ColorMode::INDEXED &&
            !sprite->backgroundLayer()) {
          int i = sprite->transparentColor();
          if (i >= 0 && i < int(palette->size()))
            palette->setEntry(i, doc::rgba(0, 0, 0, 0));
        }

        const int w = sprite->width()*sprite->pixelRatio().w;
        const int h = sprite->height()*sprite->pixelRatio().h;

        // Calculate the thumbnail size
        int thumb_w = MAX_THUMBNAIL_SIZE * w / std::max(w, h);
        int thumb_h = MAX_THUMBNAIL_SIZE * h / std::max(w, h);
        if (std::max(thumb_w, thumb_h) > std::max(w, h)) {
          thumb_w = w;
          thumb_h = h;
        }
        thumb_w = base::clamp(thumb_w, 1, MAX_THUMBNAIL_SIZE);
        thumb_h = base::clamp(thumb_h, 1, MAX_THUMBNAIL_SIZE);

        // Stretch the 'image'
        thumbnailImage.reset(
          Image::create(
            sprite->pixelFormat(), thumb_w, thumb_h));

        render::Projection proj(sprite->pixelRatio(),
                                render::Zoom(thumb_w, w));
        render::Render render;
        render.setBgType(render::BgType::TRANSPARENT);
        render.setProjection(proj);
        render.renderSprite(
          thumbnailImage.get(), sprite, frame_t(0),
          gfx::Clip(0, 0, 0, 0, w, h));

        // Convert the image to sRGB color space
        auto cs = sprite->colorSpace();
        if (m_fop->preserveColorProfile() &&
            cs && !cs->nearlyEqual(*gfx::ColorSpace::MakeSRGB())) {
          app::cmd::convert_color_profile(
            thumbnailImage.get(), palette.get(),
            cs, gfx::ColorSpace::MakeSRGB());
        }
      }

      // Close file
      delete m_fop->releaseDocument();

      // Set the thumbnail of the file-item.
      if (thumbnailImage) {
        os::Surface* thumbnail =
          os::instance()->createRgbaSurface(
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

ThumbnailGenerator::ThumbnailGenerator()
{
  int n = std::thread::hardware_concurrency()-1;
  if (n < 1) n = 1;
  m_maxWorkers = n;
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
    if (fileitem->getThumbnailProgress() == 0.00001) {
      m_remainingItems.prioritize(
        [fileitem](const Item& item) {
          return (item.fileitem == fileitem);
        });

      // If there is no more workers running, we have to start a new
      // one to process the m_remainingItems queue. How is it possible
      // that a IFileItem has a thumbnail progress == 0.00001 but
      // there is no workers?  This is an edge case where:
      // 1. The Worker::loadBgThread() asks for the queue of remaining items
      //    and it's empty, so the thread is going to be closed
      // 2. We've just created a FOP for this IFileItem and ask for
      //    available workers and we've already launch the max quantity
      //    of possible workers (m_maxWorkers)
      // 3. All worker threads are just closed so there is no more
      //    worker for the remaining item in the queue.
      if (m_workers.empty())
        startWorker();
    }
    return;
  }

  // Set a starting progress so we don't enqueue the same item two times.
  fileitem->setThumbnailProgress(0.00001);

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

  startWorker();
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

void ThumbnailGenerator::startWorker()
{
  base::scoped_lock hold(m_workersAccess);
  if (m_workers.size() < m_maxWorkers) {
    std::unique_ptr<Worker> worker(new Worker(m_remainingItems));
    m_workers.push_back(worker.get());
    worker.release();
  }
}

} // namespace app
