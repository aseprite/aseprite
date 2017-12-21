// Aseprite
// Copyright (C) 2001-2017  David Capello
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
#include "base/split_string.h"
#include "she/display.h"
#include "she/native_dialogs.h"
#include "she/system.h"

namespace app {

bool show_file_selector(
  const std::string& title,
  const std::string& initialPath,
  const std::string& showExtensions,
  FileSelectorType type,
  FileSelectorFiles& output,
  FileSelectorDelegate* delegate)
{
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

      std::vector<std::string> tokens;
      base::split_string(showExtensions, tokens, ",");
      for (const auto& tok : tokens)
        dlg->addFilter(tok, tok + " files (*." + tok + ")");

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

  FileSelector fileSelector(type, delegate);
  return fileSelector.show(title, initialPath, showExtensions, output);
}

} // namespace app
