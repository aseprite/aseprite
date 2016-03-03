// GTK Component of SHE library
// Copyright (C) 2016  Gabriel Rauter
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

//disable EMPTY_STRING macro already set in allegro, enabling it at the end of file
#pragma push_macro("EMPTY_STRING")
#undef EMPTY_STRING
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "she/gtk/native_dialogs.h"

#include "base/string.h"
#include "she/display.h"
#include "she/error.h"

#include <gtkmm/application.h>
#include <gtkmm/filechooserdialog.h>
#include <gtkmm/button.h>
#include <gtkmm/filefilter.h>
#include <glibmm/refptr.h>

#include <string>
#include <map>

namespace she {
class FileDialogGTK3 : public FileDialog, public Gtk::FileChooserDialog {
public:
  FileDialogGTK3(Glib::RefPtr<Gtk::Application> app) :
   Gtk::FileChooserDialog(""),
   app(app), cancel(true) {
    this->add_button("_Cancel", Gtk::RESPONSE_CANCEL);
    ok_button = this->add_button("_Open", Gtk::RESPONSE_OK);
    this->set_default_response(Gtk::RESPONSE_OK);
    filter_all = Gtk::FileFilter::create();
    filter_all->set_name("All formats");
    this->set_do_overwrite_confirmation();
  }

  void on_show() override {
    //setting the filename only works properly when the dialog is shown
    this->set_filename(file_name);
    //TODO set position centered to parent window, need she::screen to provide info
    this->set_position(Gtk::WIN_POS_CENTER);
    Gtk::FileChooserDialog::on_show();
    this->raise();
  }

  void on_response(int response_id) override {
    switch(response_id) {
      case(Gtk::RESPONSE_OK): {
        cancel = false;
        this->file_name = this->get_filename();
        break;
      }
    }
    this->hide();
  }

  void dispose() override {
    for (auto& window  : app->get_windows()) {
      window->close();
    }
    app->quit();
    delete this;
  }

  void toOpenFile() override {
    this->set_action(Gtk::FILE_CHOOSER_ACTION_OPEN);
    this->ok_button->set_label("_Open");
  }

  void toSaveFile() override {
    this->set_action(Gtk::FILE_CHOOSER_ACTION_SAVE);
    this->ok_button->set_label("_Save");
  }

  void setTitle(const std::string& title) override {
    this->set_title(title);
  }

  void setDefaultExtension(const std::string& extension) override {
    this->default_extension = extension;
  }

  void addFilter(const std::string& extension, const std::string& description) override {
    auto filter = Gtk::FileFilter::create();
    filter->set_name(description);
    filter->add_pattern("*." + extension);
    this->filter_all->add_pattern("*." + extension);
    this->filters[extension] = filter;
  }

  std::string fileName() override {
    return this->file_name;
  }

  void setFileName(const std::string& filename) override {
    this->file_name = filename;
  }

  bool show(Display* parent) override {
    //keep pointer on parent display to get information later
  	display = parent;

  	//add filters in order they will appear
    this->add_filter(filter_all);

    for (const auto& filter : filters) {
      this->add_filter(filter.second)	;
      if (filter.first.compare(default_extension) == 0) {
        this->set_filter(filter.second);
      }
    }

    auto filter_any = Gtk::FileFilter::create();
    filter_any->set_name("Any files");
    filter_any->add_pattern("*");
    this->add_filter(filter_any);

    //Run dialog in context of a gtk application so it can be destroys properly
    int result = app->run(*this);
    return !cancel;
  }

private:
  std::string file_name;
  std::string default_extension;
  Glib::RefPtr<Gtk::FileFilter> filter_all;
  std::map<std::string, Glib::RefPtr<Gtk::FileFilter>> filters;
  Gtk::Button* ok_button;
  Glib::RefPtr<Gtk::Application> app;
  Display* display;
  bool cancel;
};

NativeDialogsGTK3::NativeDialogsGTK3()
{
}

FileDialog* NativeDialogsGTK3::createFileDialog()
{
  app = Gtk::Application::create();
  FileDialog* dialog = new FileDialogGTK3(app);
  return dialog;
}

} // namespace she
#pragma pop_macro("EMPTY_STRING")
