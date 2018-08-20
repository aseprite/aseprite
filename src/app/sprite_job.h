// Aseprite
// Copyright (C) 2017-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_SPRITE_JOB_H_INCLUDED
#define APP_SPRITE_JOB_H_INCLUDED
#pragma once

#include "app/context.h"
#include "app/context_access.h"
#include "app/job.h"
#include "app/tx.h"
#include "render/task_delegate.h"

#include <functional>

namespace app {

class SpriteJob : public Job,
                  public render::TaskDelegate {
public:
  SpriteJob(const ContextReader& reader, const char* jobName);
  ~SpriteJob();

  ContextWriter& writer() { return m_writer; }
  Doc* document() const { return m_document; }
  Sprite* sprite() const { return m_sprite; }
  Tx& tx() { return m_tx; }

  template<typename T>
  void startJobWithCallback(T&& callback) {
    m_callback = std::move(callback);
    Job::startJob();
  }

private:
  // Job impl
  void onJob() override;

  // render::TaskDelegate impl just in case you need to use this
  // Job as a delegate in render::create_palette_from_sprite()
  bool continueTask() override;
  void notifyTaskProgress(double progress) override;

  ContextWriter m_writer;
  Doc* m_document;
  Sprite* m_sprite;
  Tx m_tx;

  // Default implementation calls the given function in
  // startJob(). Anyway you can just extended the SpriteJob and
  // override onJob().
  std::function<void()> m_callback;
};

} // namespace app

#endif // APP_SPRITE_JOB_H_INCLUDED
