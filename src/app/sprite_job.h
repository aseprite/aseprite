// Aseprite
// Copyright (C) 2024  Igara Studio S.A.
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
#include <memory>
#include <string>

namespace app {

class Context;

// Creates a Job to run a task in a background thread. At the same
// time it creates a new Tx in the main thread (to group all sub-Txs)
// without locking the sprite. You have to lock the sprite with a sub
// Tx (the onSpriteJob(Tx) already has the sprite locked for write
// access).
//
// This class takes care to lock the sprite in the background thread
// for write access, or to avoid re-locking the sprite in case it's
// already locked from the main thread (where SpriteJob was created,
// generally true when we're running a script).
class SpriteJob : public Job,
                  public render::TaskDelegate {
public:
  SpriteJob(Context* ctx, Doc* doc,
            const std::string& jobName);
  ~SpriteJob();

  Doc* document() const { return m_doc; }
  Sprite* sprite() const { return m_sprite; }

  template<typename T>
  void startJobWithCallback(T&& callback) {
    m_callback = std::move(callback);
    Job::startJob();
  }

private:
  virtual void onSpriteJob(Tx& tx);

  // Job impl
  void onJob() override;

  // render::TaskDelegate impl just in case you need to use this
  // Job as a delegate in render::create_palette_from_sprite()
  bool continueTask() override;
  void notifyTaskProgress(double progress) override;

  Context* m_ctx;
  Doc* m_doc;
  Sprite* m_sprite;
  Tx m_tx;

  // What action to do with the sub-Tx inside the background thread.
  // This is required to check if the sprite is already locked for
  // write access in the main thread, in that case we don't need to
  // lock it again from the background thread.
  Tx::LockAction m_lockAction;

  // Default implementation calls the given function in
  // startJob(). Anyway you can just extended the SpriteJob and
  // override onJob().
  std::function<void(Tx&)> m_callback;
};

} // namespace app

#endif // APP_SPRITE_JOB_H_INCLUDED
