// Aseprite Code Generator
// Copyright (c) 2014 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef GEN_PREF_CLASS_H_INCLUDED
#define GEN_PREF_CLASS_H_INCLUDED
#pragma once

#include <string>
#include "tinyxml.h"

void gen_pref_header(TiXmlDocument* doc, const std::string& inputFn);
void gen_pref_impl(TiXmlDocument* doc, const std::string& inputFn);

#endif
