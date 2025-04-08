// Aseprite
// Copyright (C) 2025  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_TIMELINE_DOC_PROVIDERS_H_INCLUDED
#define APP_UI_TIMELINE_DOC_PROVIDERS_H_INCLUDED
#pragma once

#include "app/doc_api.h"
#include "base/paths.h"

namespace app {

using namespace docapi;

class Context;

class DocProviderFromPaths : public DocProvider {
public:
  DocProviderFromPaths(Context* context, const base::paths& paths)
    : m_context(context)
    , m_paths(paths)
  {
  }

  virtual ~DocProviderFromPaths() = default;

  std::unique_ptr<Doc> nextDoc() override;

  int pendingDocs() override { return m_paths.size(); }

private:
  Context* m_context;
  base::paths m_paths;
};

class DocProviderFromImage : public DocProvider {
public:
  DocProviderFromImage(const doc::ImageRef& image) : m_image(image) {}

  virtual ~DocProviderFromImage() = default;

  std::unique_ptr<Doc> nextDoc() override;

  int pendingDocs() override { return m_image ? 1 : 0; }

private:
  doc::ImageRef m_image = nullptr;
};

} // namespace app

#endif
