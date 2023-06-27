// Aseprite
// Copyright (C) 2020-2023  Igara Studio S.A.
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
#include "base/fs.h"
#include "os/native_dialogs.h"
#include "os/system.h"
#include "os/window.h"

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
      os::instance()->nativeDialogs()) {
    os::FileDialogRef dlg =
      os::instance()->nativeDialogs()->makeFileDialog();

    if (dlg) {
      dlg->setTitle(title);

      // Must be set before setFileName() as the Linux impl might
      // require the default extension to fix the initial file name
      // with the default extension.
      if (!defExtension.empty()) {
        dlg->setDefaultExtension(defExtension);
      }

#if LAF_LINUX // As the X11 version doesn't store the default path to
              // start navigating, we use our own
              // get_initial_path_to_select_filename()
      dlg->setFileName(get_initial_path_to_select_filename(initialPath));
#else  // !LAF_LINUX
      dlg->setFileName(initialPath);
#endif

      os::FileDialog::Type nativeType = os::FileDialog::Type::OpenFile;
      switch (type) {
        case FileSelectorType::Open:
          nativeType = os::FileDialog::Type::OpenFile;
          break;
        case FileSelectorType::OpenMultiple:
          nativeType = os::FileDialog::Type::OpenFiles;
          break;
        case FileSelectorType::Save:
          nativeType = os::FileDialog::Type::SaveFile;
          break;
      }
      dlg->setType(nativeType);

      for (const auto& ext : extensions)
        dlg->addFilter(ext, ext + " files (*." + ext + ")");

      auto res = dlg->show(os::instance()->defaultWindow());
      if (res != os::FileDialog::Result::Error) {
        if (res == os::FileDialog::Result::OK) {
          if (type == FileSelectorType::OpenMultiple)
            dlg->getMultipleFileNames(output);
          else
            output.push_back(dlg->fileName());

#if LAF_LINUX // Save the path in the configuration file
          if (!output.empty()) {
            set_current_dir_for_file_selector(base::get_file_path(output[0]));
          }
#endif

          return true;
        }
        else {
          return false;
        }
      }
      else {
        // Fallback to default file selector if we weren't able to
        // open the native dialog...
      }
    }
  }

  FileSelector fileSelector(type);

  if (!defExtension.empty())
    fileSelector.setDefaultExtension(defExtension);

  return fileSelector.show(title, initialPath, extensions, output);
}

std::string get_initial_path_to_select_filename(const std::string& initialFilename)
{
  std::string path = base::get_file_path(initialFilename);
  // If initialFilename doesn't contain a path/directory.
  if (path.empty()) {
    // Put the saved 'path' from the configuration file.
    path = base::join_path(get_current_dir_for_file_selector(),
                           base::get_file_name(initialFilename));
  }
  else {
    path = initialFilename;
  }
  path = base::fix_path_separators(path);
  return path;
}

std::string get_current_dir_for_file_selector()
{
  std::string path = Preferences::instance().fileSelector.currentFolder();
  // If it's empty or the folder doesn't exist anymore, starts from
  // the docs folder by default.
  if (path.empty() ||
      path == "<empty>" ||
      !base::is_directory(path)) {
    path = base::get_user_docs_folder();
  }
  return path;
}

void set_current_dir_for_file_selector(const std::string& dirPath)
{
  Preferences::instance().fileSelector.currentFolder(dirPath);
}

} // namespace app
