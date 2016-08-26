// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/thumbnail_generator.h"

#include "app/app.h"
#include "app/app_render.h"
#include "app/document.h"
#include "app/file/file.h"
#include "app/file_system.h"
#include "base/bind.h"
#include "base/scoped_lock.h"
#include "base/thread.h"
#include "doc/algorithm/rotate.h"
#include "doc/conversion_she.h"
#include "doc/image.h"
#include "doc/palette.h"
#include "doc/primitives.h"
#include "doc/sprite.h"
#include "she/system.h"

#define MAX_THUMBNAIL_SIZE              128

namespace app {

class ThumbnailGenerator::Worker {
public:
  Worker(FileOp* fop, IFileItem* fileitem)
    : m_fop(fop)
    , m_fileitem(fileitem)
    , m_thumbnail(NULL)
    , m_palette(NULL)
    , m_thread(base::Bind<void>(&Worker::loadBgThread, this)) {
  }

  ~Worker() {
    m_fop->stop();
    m_thread.join();
  }

  IFileItem* getFileItem() { return m_fileitem; }
  bool isDone() const { return m_fop->isDone(); }
  double getProgress() const { return m_fop->progress(); }

private:
  void loadBgThread() {
    try {
      m_fop->operate(nullptr);

      // Post load
      m_fop->postLoad();

      // Convert the loaded document into the she::Surface.
      const Sprite* sprite =
        (m_fop->document() &&
         m_fop->document()->sprite() ?
         m_fop->document()->sprite(): nullptr);

      if (!m_fop->isStop() && sprite) {
        // The palette to convert the Image
        m_palette.reset(new Palette(*sprite->palette(frame_t(0))));

        // Render first frame of the sprite in 'image'
        base::UniquePtr<Image> image(Image::create(
            IMAGE_RGB, sprite->width(), sprite->height()));

        AppRender render;
        render.setupBackground(NULL, image->pixelFormat());
        render.setBgType(render::BgType::CHECKED);
        render.renderSprite(image, sprite, frame_t(0));

        // Calculate the thumbnail size
        int thumb_w = MAX_THUMBNAIL_SIZE * image->width() / MAX(image->width(), image->height());
        int thumb_h = MAX_THUMBNAIL_SIZE * image->height() / MAX(image->width(), image->height());
        if (MAX(thumb_w, thumb_h) > MAX(image->width(), image->height())) {
          thumb_w = image->width();
          thumb_h = image->height();
        }
        thumb_w = MID(1, thumb_w, MAX_THUMBNAIL_SIZE);
        thumb_h = MID(1, thumb_h, MAX_THUMBNAIL_SIZE);

        // Stretch the 'image'
        m_thumbnail.reset(Image::create(image->pixelFormat(), thumb_w, thumb_h));
        clear_image(m_thumbnail, 0);
        algorithm::scale_image(m_thumbnail, image,
                               0, 0, thumb_w, thumb_h,
                               0, 0, image->width(), image->height());
      }

      // Close file
      delete m_fop->releaseDocument();

      // Set the thumbnail of the file-item.
      if (m_thumbnail) {
        she::Surface* thumbnail = she::instance()->createRgbaSurface(
          m_thumbnail->width(),
          m_thumbnail->height());

        convert_image_to_surface(m_thumbnail, m_palette, thumbnail,
          0, 0, 0, 0, m_thumbnail->width(), m_thumbnail->height());

        m_fileitem->setThumbnail(thumbnail);
      }
    }
    catch (const std::exception& e) {
      m_fop->setError("Error loading file:\n%s", e.what());
    }
    m_fop->done();
  }

  base::UniquePtr<FileOp> m_fop;
  IFileItem* m_fileitem;
  base::UniquePtr<Image> m_thumbnail;
  base::UniquePtr<Palette> m_palette;
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
    App::instance()->Exit.connect(base::Bind<void>(&delete_singleton, singleton));
  }
  return singleton;
}

ThumbnailGenerator::WorkerStatus ThumbnailGenerator::getWorkerStatus(IFileItem* fileitem, double& progress)
{
  base::scoped_lock hold(m_workersAccess);

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
  base::scoped_lock hold(m_workersAccess);
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

  base::UniquePtr<FileOp> fop(
    FileOp::createLoadDocumentOperation(
      nullptr,
      fileitem->fileName().c_str(),
      FILE_LOAD_SEQUENCE_NONE |
      FILE_LOAD_ONE_FRAME));
  if (!fop)
    return;

  if (fop->hasError())
    return;

  Worker* worker = new Worker(fop.release(), fileitem);
  try {
    base::scoped_lock hold(m_workersAccess);
    m_workers.push_back(worker);
  }
  catch (...) {
    delete worker;
    throw;
  }
}

void ThumbnailGenerator::stopAllWorkers()
{
  base::thread* ptr = new base::thread(base::Bind<void>(&ThumbnailGenerator::stopAllWorkersBackground, this));
  m_stopThread.reset(ptr);
}

void ThumbnailGenerator::stopAllWorkersBackground()
{
  WorkerList workersCopy;
  {
    base::scoped_lock hold(m_workersAccess);
    workersCopy = m_workers;
    m_workers.clear();
  }

  for (auto it=workersCopy.begin(), end=workersCopy.end(); it!=end; ++it)
    delete *it;
}

} // namespace app
