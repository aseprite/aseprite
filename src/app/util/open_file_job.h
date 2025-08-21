// Aseprite
// Copyright (C) 2024-2025  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_OPEN_FILE_JOB_H_INCLUDED
#define APP_OPEN_FILE_JOB_H_INCLUDED
#pragma once

#include "app/file/file.h"
#include "app/i18n/strings.h"
#include "app/job.h"

namespace app {

class OpenFileJob : public Job,
                    public IFileOpProgress {
public:
  OpenFileJob(FileOp* fop, const bool showProgress)
    : Job(Strings::open_file_loading(), showProgress)
    , m_fop(fop)
  {
  }

  void showProgressWindow();

private:
  // Thread to do the hard work: load the file from the disk.
  virtual void onJob() override;

  virtual void ackFileOpProgress(double progress) override { jobProgress(progress); }

  FileOp* m_fop;
};

} // namespace app

#endif
