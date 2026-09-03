// Aseprite
// Copyright (C) 2019-present  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_SET_TAG_ANIDIR_H_INCLUDED
#define APP_CMD_SET_TAG_ANIDIR_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_tag.h"
#include "doc/anidir.h"

namespace app { namespace cmd {
using namespace doc;

class SetTagAniDir : public Cmd,
                     public WithTag {
public:
  CMDTYPE('d', 'i', 'T', 'g');

  SetTagAniDir(Tag* tag, doc::AniDir anidir);

protected:
  void onExecute(Context* ctx) override;
  void onUndo(Context* ctx) override;
  size_t onMemSize() const override { return sizeof(*this); }

private:
  doc::AniDir m_oldAniDir;
  doc::AniDir m_newAniDir;
};

}} // namespace app::cmd

#endif
