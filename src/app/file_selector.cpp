// Aseprite
// Copyright (C) 2020-2024  Igara Studio S.A.
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
#include "app/ui/app_menuitem.h"
#include "app/ui/file_selector.h"
#include "base/fs.h"
#include "dlgs/file_dialog.h"
#include "os/system.h"
#include "os/window.h"

#if LAF_LINUX
  #include "os/x11/x11.h"
#endif

namespace app {

bool show_file_selector(const std::string& title,
                        const std::string& initialPath,
                        const base::paths& extensions,
                        FileSelectorType type,
                        base::paths& output)
{
  const os::SystemRef system = os::System::instance();
  const std::string defExtension = Preferences::instance().saveFile.defaultExtension();

  if (Preferences::instance().experimental.useNativeFileDialog()) {
    dlgs::FileDialog::Spec spec;

#if LAF_MACOS
    // Setup the standard "Edit" item for macOS
    auto* editItem = AppMenuItem::GetStandardEditMenu();
    if (editItem && editItem->native() && editItem->native()->menuItem) {
      spec.editNSMenuItem = editItem->native()->menuItem->nativeHandle();
    }
#elif LAF_LINUX
    // Connect laf-os <-> laf-dlgs information about the X11 server
    // connection.
    spec.x11display = os::X11::instance()->display();
#endif

    dlgs::FileDialogRef dlg = dlgs::FileDialog::make(spec);
    if (dlg) {
      dlg->setTitle(title);

      // Must be set before setFileName() as the Linux impl might
      // require the default extension to fix the initial file name
      // with the default extension.
      if (!defExtension.empty()) {
        dlg->setDefaultExtension(defExtension);
      }

#if LAF_LINUX
      // As the X11 version doesn't store the default path to start
      // navigating, we use our own
      // get_initial_path_to_select_filename()
      dlg->setFileName(get_initial_path_to_select_filename(initialPath));
#else // !LAF_LINUX
      dlg->setFileName(initialPath);
#endif

      dlgs::FileDialog::Type nativeType = dlgs::FileDialog::Type::OpenFile;
      switch (type) {
        case FileSelectorType::Open:         nativeType = dlgs::FileDialog::Type::OpenFile; break;
        case FileSelectorType::OpenMultiple: nativeType = dlgs::FileDialog::Type::OpenFiles; break;
        case FileSelectorType::Save:         nativeType = dlgs::FileDialog::Type::SaveFile; break;
      }
      dlg->setType(nativeType);

      for (const auto& ext : extensions)
        dlg->addFilter(ext, ext + " files (*." + ext + ")");

      auto res = dlg->show(system->defaultWindow()->nativeHandle());
      if (res != dlgs::FileDialog::Result::Error) {
        if (res == dlgs::FileDialog::Result::OK) {
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
  // "<empty>" is the default value for this property, to start from
  // the user docs folder by default.
  if (path == "<empty>") {
    path = base::get_user_docs_folder();
  }
  return path;
}

void set_current_dir_for_file_selector(const std::string& dirPath)
{
  Preferences::instance().fileSelector.currentFolder(dirPath);
}

} // namespace app
