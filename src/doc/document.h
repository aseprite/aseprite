// Aseprite Document Library
// Copyright (c) 2014 David Capello
//
// This source file is distributed under the terms of the MIT license,
// please read LICENSE.txt for more information.

#ifndef DOC_DOCUMENT_H_INCLUDED
#define DOC_DOCUMENT_H_INCLUDED

#include <string>

#include "doc/object.h"

namespace doc {

  class Document : public Object {
  public:
    Document();

    const std::string& filename() const { return m_filename; }

    void setFilename(const std::string& filename);

  private:
    // Document's file name. From where it was loaded, where it is
    // saved.
    std::string m_filename;
  };

} // namespace doc

#endif
