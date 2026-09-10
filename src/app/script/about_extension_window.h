// Aseprite
// Copyright (c) 2026-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_SCRIPT_ABOUT_EXTENSION_WINDOW_H_INCLUDED
#define APP_SCRIPT_ABOUT_EXTENSION_WINDOW_H_INCLUDED

#include "base/launcher.h"
#include "ui/alert.h"

#include "about_extension.xml.h"

namespace app {
using namespace ui;
class AboutExtensionWindow : public gen::AboutExtension {
public:
  explicit AboutExtensionWindow(const Extension* ext)
  {
    const auto& about = ext->readAbout();

    if (!ext->canBeUninstalled())
      setText(Strings::about_extension_title_builtin());

    name()->setText(about.name);
    version()->setText(about.version);

    if (about.description.empty())
      description()->setVisible(false);
    else {
      auto* desc = description();
      desc->setText(about.description);
    }

    if (about.url.empty()) {
      urlContainer()->setVisible(false);
    }
    else {
      url()->setText(about.url);
      url()->setUrl(about.url);
    }

    openFolder()->Click.connect([ext] { base::launcher::open_folder(ext->path()); });

    if (about.displayName.empty() || about.displayName == about.name) {
      displayName()->setText(about.name);
      name()->setVisible(false);
      versionSeparator()->setVisible(false);
    }
    else {
      displayName()->setText(about.displayName);
    }

    if (about.author.has_value()) {
      const auto& contributor = about.author.value();
      Widget* label;
      if (contributor.url.empty()) {
        label = new Label(contributor.toString());
      }
      else {
        label = new LinkLabel(contributor.url, contributor.toString());
        tooltipManager()->addTooltipFor(label, contributor.url, BOTTOM);
      }
      authorContainer()->addChild(label);
    }
    else {
      authorContainer()->setVisible(false);
    }

    if (!about.contributors.empty()) {
      for (const auto& contributor : about.contributors) {
        Widget* label;
        if (!contributor.url.empty()) {
          label = new LinkLabel(contributor.url, contributor.toString());
          tooltipManager()->addTooltipFor(label, contributor.url, BOTTOM);
        }
        else {
          label = new Label(contributor.toString());
        }
        contributors()->addChild(label);
      }
    }
    else {
      contributorsContainer()->setVisible(false);
    }

    layout();
  };

  static void show(const Extension* extension)
  {
    ASSERT(extension);
    if (!extension)
      return;

    try {
      AboutExtensionWindow about(extension);
      about.openWindowInForeground();
    }
    catch (const std::exception&) {
      if (Alert::show(Strings::alerts_cannot_read_extension()) == 1)
        base::launcher::open_folder(extension->path());
    }
  }
};
} // namespace app

#endif // APP_SCRIPT_ABOUT_EXTENSION_WINDOW_H_INCLUDED
