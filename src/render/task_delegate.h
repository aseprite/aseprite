// Aseprite Render Library
// Copyright (c) 2017 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef RENDER_TASK_DELEGATE_H_INCLUDED
#define RENDER_TASK_DELEGATE_H_INCLUDED
#pragma once

namespace render {

class TaskDelegate {
public:
  virtual ~TaskDelegate() {}
  virtual void notifyTaskProgress(double progress) = 0;
  virtual bool continueTask() = 0;
};

} // namespace render

#endif // RENDER_TASK_H_INCLUDED
