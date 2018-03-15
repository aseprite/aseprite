// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/file_selector.h"

#include "app/app.h"
#include "app/pref/preferences.h"
#include "app/ui/file_selector.h"
#include "she/display.h"
#include "she/native_dialogs.h"
#include "she/system.h"

namespace app {

bool show_file_selector(
  const std::string& title,
  const std::string& initialPath,
  const base::paths& extensions,
  FileSelectorType type,
  base::paths& output)
{
  const std::string defExtension =
    Preferences::instance().saveFile.defaultExtension();

  if (Preferences::instance().experimental.useNativeFileDialog() &&
      she::instance()->nativeDialogs()) {
    she::FileDialog* dlg =
      she::instance()->nativeDialogs()->createFileDialog();

    if (dlg) {
      dlg->setTitle(title);
      dlg->setFileName(initialPath);

      she::FileDialog::Type nativeType = she::FileDialog::Type::OpenFile;
      switch (type) {
        case FileSelectorType::Open:
          nativeType = she::FileDialog::Type::OpenFile;
          break;
        case FileSelectorType::OpenMultiple:
          nativeType = she::FileDialog::Type::OpenFiles;
          break;
        case FileSelectorType::Save:
          nativeType = she::FileDialog::Type::SaveFile;
          break;
      }
      dlg->setType(nativeType);

      for (const auto& ext : extensions)
        dlg->addFilter(ext, ext + " files (*." + ext + ")");

      if (!defExtension.empty())
        dlg->setDefaultExtension(defExtension);

      bool res = dlg->show(she::instance()->defaultDisplay());
      if (res) {
        if (type == FileSelectorType::OpenMultiple)
          dlg->getMultipleFileNames(output);
        else
          output.push_back(dlg->fileName());
      }
      dlg->dispose();
      return res;
    }
  }

  FileSelector fileSelector(type);

  if (!defExtension.empty())
    fileSelector.setDefaultExtension(defExtension);

  return fileSelector.show(title, initialPath, extensions, output);
}

} // namespace app
