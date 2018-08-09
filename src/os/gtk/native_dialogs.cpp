// LAF OS Library
// Copyright (C) 2017-2018  David Capello
// Copyright (C) 2016  Gabriel Rauter
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "os/gtk/native_dialogs.h"

#include "base/fs.h"
#include "base/string.h"
#include "os/common/file_dialog.h"
#include "os/display.h"
#include "os/error.h"

#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>

namespace os {

class FileDialogGTK : public CommonFileDialog {
public:
  FileDialogGTK() {
  }

  std::string fileName() override {
    return m_filename;
  }

  void getMultipleFileNames(base::paths& output) override {
    output = m_filenames;
  }

  void setFileName(const std::string& filename) override {
    m_filename = base::get_file_name(filename);
    m_initialDir = base::get_file_path(filename);
  }

  bool show(Display* parent) override {
    static std::string s_lastUsedDir;
    if (s_lastUsedDir.empty())
      s_lastUsedDir = g_get_user_special_dir(G_USER_DIRECTORY_DESKTOP);

    const char* okLabel;
    GtkFileChooserAction action;

    switch (m_type) {
      case Type::OpenFile:
      case Type::OpenFiles:
        action = GTK_FILE_CHOOSER_ACTION_OPEN;
        okLabel = "_Open";
        break;
      case Type::OpenFolder:
        action = GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER;
        okLabel = "_Open Folder";
        break;
      case Type::SaveFile:
        action = GTK_FILE_CHOOSER_ACTION_SAVE;
        okLabel = "_Save";
        break;
    }

    // GtkWindow* gtkParent = nullptr;
    GtkWidget* dialog = gtk_file_chooser_dialog_new(
      m_title.c_str(),
      nullptr,
      action,
      "_Cancel", GTK_RESPONSE_CANCEL,
      okLabel, GTK_RESPONSE_ACCEPT,
      nullptr);

    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);

    GtkFileChooser* chooser = GTK_FILE_CHOOSER(dialog);
    m_chooser = chooser;

    if (m_type == Type::SaveFile)
      gtk_file_chooser_set_do_overwrite_confirmation(chooser, TRUE);
    else if (m_type == Type::OpenFiles)
      gtk_file_chooser_set_select_multiple(chooser, true);

    if (m_type != Type::OpenFolder) {
      setupFilters(base::get_file_extension(m_filename));
      setupPreview();
    }

    if (m_initialDir.empty())
      gtk_file_chooser_set_current_folder(chooser, s_lastUsedDir.c_str());
    else
      gtk_file_chooser_set_current_folder(chooser, m_initialDir.c_str());

    if (!m_filename.empty()) {
      std::string fn = m_filename;
      // Add default extension
      if (m_type == Type::SaveFile && base::get_file_extension(fn).empty()) {
        fn.push_back('.');
        fn += m_defExtension;
      }
      gtk_file_chooser_set_current_name(chooser, fn.c_str());
    }

    // Setup the "parent" display as the parent of the dialog (we've
    // to convert a X11 Window into a GdkWindow to do this).
    GdkWindow* gdkParentWindow = nullptr;
    if (parent) {
      GdkWindow* gdkWindow = gtk_widget_get_root_window(dialog);

      gdkParentWindow =
        gdk_x11_window_foreign_new_for_display(
          gdk_window_get_display(gdkWindow),
          (::Window)parent->nativeHandle());

      gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER_ON_PARENT);
      gdk_window_set_transient_for(gdkWindow, gdkParentWindow);
    }
    else {
      gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
    }

    // Show the dialog
    gint res = gtk_dialog_run(GTK_DIALOG(dialog));
    if (res == GTK_RESPONSE_ACCEPT) {
      s_lastUsedDir = gtk_file_chooser_get_current_folder(chooser);
      m_filename = gtk_file_chooser_get_filename(chooser);

      if (m_type == Type::OpenFiles) {
        GSList* list = gtk_file_chooser_get_filenames(chooser);
        g_slist_foreach(
          list,
          [](void* fn, void* userdata){
            auto self = (FileDialogGTK*)userdata;
            self->m_filenames.push_back((char*)fn);
            g_free(fn);
          }, this);
        g_slist_free(list);
      }
    }

    gtk_widget_destroy(dialog);
    if (gdkParentWindow)
      g_object_unref(gdkParentWindow);

    // Pump gtk+ events to finally hide the dialog from the screen
    while (gtk_events_pending())
      gtk_main_iteration();

    return (res == GTK_RESPONSE_ACCEPT);
  }

private:
  void setupFilters(const std::string& fnExtension) {
    // Filter for all known formats
    GtkFileFilter* gtkFilter = gtk_file_filter_new();
    gtk_file_filter_set_name(gtkFilter, "All formats");
    for (const auto& filter : m_filters) {
      const std::string& ext = filter.first;
      std::string pat = "*." + ext;
      gtk_file_filter_add_pattern(gtkFilter, pat.c_str());
    }
    gtk_file_chooser_add_filter(m_chooser, gtkFilter);

    // One filter for each format
    for (const auto& filter : m_filters) {
      const std::string& ext = filter.first;
      const std::string& desc = filter.second;
      std::string pat = "*." + ext;

      gtkFilter = gtk_file_filter_new();
      gtk_file_filter_set_name(gtkFilter, desc.c_str());
      gtk_file_filter_add_pattern(gtkFilter, pat.c_str());
      gtk_file_chooser_add_filter(m_chooser, gtkFilter);

      if (base::utf8_icmp(ext, fnExtension) == 0)
        gtk_file_chooser_set_filter(m_chooser, gtkFilter);
    }

    // One filter for all files
    gtkFilter = gtk_file_filter_new();
    gtk_file_filter_set_name(gtkFilter, "All files");
    gtk_file_filter_add_pattern(gtkFilter, "*");
    gtk_file_chooser_add_filter(m_chooser, gtkFilter);
  }

  void setupPreview() {
    m_preview = gtk_image_new();

    gtk_file_chooser_set_use_preview_label(m_chooser, false);
    gtk_file_chooser_set_preview_widget(m_chooser, m_preview);

    g_signal_connect(
      m_chooser, "update-preview",
      G_CALLBACK(&FileDialogGTK::s_onUpdatePreview), this);
  }

  static void s_onUpdatePreview(GtkFileChooser* chooser, gpointer userData) {
    ((FileDialogGTK*)userData)->onUpdatePreview();
  }
  void onUpdatePreview() {
    // Disable preview because we don't know if we will be able to
    // load/generate the preview successfully.
    gtk_file_chooser_set_preview_widget_active(m_chooser, false);

    const char* fn = gtk_file_chooser_get_filename(m_chooser);
    if (fn && base::is_file(fn)) {
      GError* err = nullptr;
      GdkPixbuf* previewPixbuf =
        gdk_pixbuf_new_from_file_at_scale(fn, 256, 256, true, &err);
      if (previewPixbuf) {
        gtk_image_set_from_pixbuf(GTK_IMAGE(m_preview), previewPixbuf);
        g_object_unref(previewPixbuf);

        // Now we can enable the preview panel as the preview was
        // generated.
        gtk_file_chooser_set_preview_widget_active(m_chooser, true);
      }
      if (err)
        g_error_free(err);
    }
  }

  std::string m_filename;
  std::string m_initialDir;
  base::paths m_filenames;
  GtkFileChooser* m_chooser;
  GtkWidget* m_preview;
};

NativeDialogsGTK::NativeDialogsGTK()
  : m_gtkApp(nullptr)
{
}

NativeDialogsGTK::~NativeDialogsGTK()
{
  if (m_gtkApp) {
    g_object_unref(m_gtkApp);
    m_gtkApp = nullptr;
  }
}

FileDialog* NativeDialogsGTK::createFileDialog()
{
  if (!m_gtkApp) {
    int argc = 0;
    char** argv = nullptr;
    gtk_init(&argc, &argv);

    m_gtkApp = gtk_application_new(nullptr, G_APPLICATION_FLAGS_NONE);
  }
  return new FileDialogGTK;
}

} // namespace os
