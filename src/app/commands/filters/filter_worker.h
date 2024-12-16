// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_COMMANDS_FILTERS_FILTER_BG_H_INCLUDED
#define APP_COMMANDS_FILTERS_FILTER_BG_H_INCLUDED
#pragma once

namespace app {

class FilterManagerImpl;

void start_filter_worker(FilterManagerImpl* filterMgr);

} // namespace app

#endif
