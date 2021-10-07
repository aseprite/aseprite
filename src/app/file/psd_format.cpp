// Aseprite
// Copyright (C) 2018-2020  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#include "app/file/file.h"
#include "app/file/file_format.h"
#include "base/file_handle.h"

namespace app {

class PsdFormat : public FileFormat {
  const char* onGetName() const override {
    return "psd";
  }
  void onGetExtensions(base::paths& exts) const override {
    exts.push_back("psd");
  }

  dio::FileFormat onGetDioFormat() const override {
    return dio::FileFormat::PSD_IMAGE;
  }

  int onGetFlags() const override {
    return FILE_SUPPORT_LOAD;
  }

  bool onLoad(FileOp* fop) override;
  bool onSave(FileOp* fop) override;
};

FileFormat* CreatePsdFormat(){
  return new PsdFormat();
}

bool PsdFormat::onLoad(FileOp* fop){
  return false;
}

bool PsdFormat::onSave(FileOp* fop){
  return false;
}

}