// Aseprite Document IO Library
// Copyright (c) 2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOCIO_DETECT_FORMAT_H_INCLUDED
#define DOCIO_DETECT_FORMAT_H_INCLUDED
#pragma once

#include "docio/file_format.h"

#include <string>

namespace docio {

FileFormat detect_format(const std::string& filename);
FileFormat detect_format_by_file_content(const std::string& filename);
FileFormat detect_format_by_file_extension(const std::string& filename);

} // namespace docio

#endif
