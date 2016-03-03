// GTK Component of SHE library
// Copyright (C) 2016  Gabriel Rauter
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

//disable EMPTY_STRING macro already set in allegro, enabling it at the end of file
#pragma push_macro("EMPTY_STRING")
#undef EMPTY_STRING
#ifndef SHE_GTK_NATIVE_DIALOGS_H_INCLUDED
#define SHE_GTK_NATIVE_DIALOGS_H_INCLUDED
#pragma once

#include "she/native_dialogs.h"

#include <gtkmm/application.h>
#include <glibmm/refptr.h>

namespace she {

  class NativeDialogsGTK3 : public NativeDialogs {
  public:
    NativeDialogsGTK3();
    FileDialog* createFileDialog() override;
  private:
    Glib::RefPtr<Gtk::Application> m_app;
  };

} // namespace she

#endif
#pragma pop_macro("EMPTY_STRING")
