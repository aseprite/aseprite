// Aseprite
// Copyright (C) 2025  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/ui/timeline/doc_providers.h"

#include "app/console.h"
#include "app/util/open_file_job.h"
#include "doc/layer.h"

namespace app {

std::unique_ptr<Doc> DocProviderFromPaths::nextDoc()
{
  if (m_paths.empty())
    return nullptr;

  std::string path = m_paths.front();
  // Remove the path that is currently being processed
  m_paths.erase(m_paths.begin());

  Console console;
  int flags = FILE_LOAD_DATA_FILE | FILE_LOAD_AVOID_BACKGROUND_LAYER | FILE_LOAD_CREATE_PALETTE |
              FILE_LOAD_SEQUENCE_YES;

  std::unique_ptr<FileOp> fop(FileOp::createLoadDocumentOperation(m_context, path, flags));

  // Do nothing (the user cancelled or something like that)
  if (!fop)
    return nullptr;

  if (fop->hasError()) {
    console.printf(fop->error().c_str());
    return nullptr;
  }

  base::paths fopFilenames;
  fop->getFilenameList(fopFilenames);
  // Remove paths that will be loaded by the current file operation.
  for (const auto& filename : fopFilenames) {
    auto it = std::find(m_paths.begin(), m_paths.end(), filename);
    if (it != m_paths.end())
      m_paths.erase(it);
  }

  OpenFileJob task(fop.get(), true);
  task.showProgressWindow();

  // Post-load processing, it is called from the GUI because may require user intervention.
  fop->postLoad();

  // Show any error
  if (fop->hasError() && !fop->isStop())
    console.printf(fop->error().c_str());

  return std::unique_ptr<Doc>(fop->releaseDocument());
}

std::unique_ptr<Doc> DocProviderFromImage::nextDoc()
{
  if (!m_image)
    return nullptr;

  Sprite* sprite = new Sprite(m_image->spec(), 256);
  LayerImage* layer = new LayerImage(sprite);
  sprite->root()->addLayer(layer);
  Cel* cel = new Cel(0, m_image);
  layer->addCel(cel);
  m_image = nullptr;
  return std::make_unique<Doc>(sprite);
}

} // namespace app
