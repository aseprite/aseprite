// SHE library - GTK dialogs
// Copyright (C) 2017  David Capello
// Copyright (C) 2016  Gabriel Rauter
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_GTK_NATIVE_DIALOGS_H_INCLUDED
#define SHE_GTK_NATIVE_DIALOGS_H_INCLUDED
#pragma once

#include "she/native_dialogs.h"

extern "C" struct _GtkApplication;

namespace she {

  class NativeDialogsGTK : public NativeDialogs {
  public:
    NativeDialogsGTK();
    ~NativeDialogsGTK();
    FileDialog* createFileDialog() override;
  private:
    _GtkApplication* m_gtkApp;
  };

} // namespace she

#endif
