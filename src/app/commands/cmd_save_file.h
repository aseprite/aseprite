// Aseprite
// Copyright (C) 2021-2022  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_COMMANDS_CMD_SAVE_FILE_H_INCLUDED
#define APP_COMMANDS_CMD_SAVE_FILE_H_INCLUDED
#pragma once

#include "app/commands/command.h"
#include "app/commands/new_params.h"
#include "doc/anidir.h"
#include "doc/frames_sequence.h"
#include "gfx/point.h"
#include "gfx/rect.h"

#include <string>

namespace app {
  class Doc;

  struct SaveFileParams : public NewParams {
    Param<bool> ui { this, true, { "ui", "useUI" } };
    Param<std::string> filename { this, std::string(), "filename" };
    Param<std::string> filenameFormat { this, std::string(), { "filenameFormat", "filename-format" } };
    Param<std::string> tag { this, std::string(), { "tag", "frame-tag" } };
    Param<doc::AniDir> aniDir { this, doc::AniDir::FORWARD, { "aniDir", "ani-dir" } };
    Param<std::string> slice { this, std::string(), "slice" };
    Param<doc::frame_t> fromFrame { this, 0, { "fromFrame", "from-frame" } };
    Param<doc::frame_t> toFrame { this, 0, { "toFrame", "to-frame" } };
    Param<bool> ignoreEmpty { this, false, "ignoreEmpty" };
    Param<double> scale { this, 1.0, "scale" };
    Param<gfx::Rect> bounds { this, gfx::Rect(), "bounds" };
    Param<bool> playSubtags { this, false, "playSubtags" };
  };

  class SaveFileBaseCommand : public CommandWithNewParams<SaveFileParams> {
  public:
    enum class MarkAsSaved { Off, On };
    enum class SaveInBackground { Off, On };
    enum class ResizeOnTheFly { Off, On };

    SaveFileBaseCommand(const char* id, CommandFlags flags);

  protected:
    void onLoadParams(const Params& params) override;
    bool onEnabled(Context* context) override;

    std::string saveAsDialog(
      Context* context,
      const std::string& dlgTitle,
      const std::string& filename,
      const MarkAsSaved markAsSaved,
      const SaveInBackground saveInBackground = SaveInBackground::On,
      const std::string& forbiddenFilename = std::string());
    void saveDocumentInBackground(
      const Context* context,
      Doc* document,
      const std::string& filename,
      const MarkAsSaved markAsSaved,
      const ResizeOnTheFly resizeOnTheFly = ResizeOnTheFly::Off,
      const gfx::PointF& scale = gfx::PointF(1.0, 1.0));

    doc::FramesSequence m_framesSeq;
    bool m_adjustFramesByTag;
  };

} // namespace app

#endif
