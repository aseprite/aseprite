// Aseprite Document Library
// Copyright (c) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_CANCEL_IO_H_INCLUDED
#define DOC_CANCEL_IO_H_INCLUDED
#pragma once

namespace doc {

class CancelIO {
public:
  virtual ~CancelIO() {}
  virtual bool isCanceled() = 0;
};

} // namespace doc

#endif
