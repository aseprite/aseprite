// Aseprite
// Copyright (C) 2025  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#include "app/util/open_file_job.h"

namespace app {

void OpenFileJob::showProgressWindow()
{
  startJob();

  if (isCanceled())
    m_fop->stop();

  waitJob();
}

// Thread to do the hard work: load the file from the disk.
void OpenFileJob::onJob()
{
  try {
    m_fop->operate(this);
  }
  catch (const std::exception& e) {
    m_fop->setError("Error loading file:\n%s", e.what());
  }

  if (m_fop->isStop() && m_fop->document())
    delete m_fop->releaseDocument();

  m_fop->done();
}

} // namespace app
