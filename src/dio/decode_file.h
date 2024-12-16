// Aseprite Document IO Library
// Copyright (c) 2017 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DIO_DECODE_FILE_H_INCLUDED
#define DIO_DECODE_FILE_H_INCLUDED
#pragma once

namespace dio {

class DecodeDelegate;
class FileInterface;

bool decode_file(DecodeDelegate* delegate, FileInterface* f);

} // namespace dio

#endif
