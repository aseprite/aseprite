// Aseprite
// Copyright (C) 2018-2023  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_PREF_PREFERENCES_H_INCLUDED
#define APP_PREF_PREFERENCES_H_INCLUDED
#pragma once

#include "app/color.h"
#include "app/commands/filters/cels_target.h"
#include "app/docs_observer.h"
#include "app/pref/option.h"
#include "app/sprite_sheet_data_format.h"
#include "app/sprite_sheet_type.h"
#include "app/tools/dynamics.h"
#include "app/tools/freehand_algorithm.h"
#include "app/tools/ink_type.h"
#include "app/tools/rotation_algorithm.h"
#include "app/ui/color_bar.h"
#include "doc/algorithm/resize_image.h"
#include "doc/anidir.h"
#include "doc/brush_pattern.h"
#include "doc/color_mode.h"
#include "doc/frame.h"
#include "doc/layer_list.h"
#include "doc/rgbmap_algorithm.h"
#include "doc/sprite.h"
#include "filters/hue_saturation_filter.h"
#include "filters/tiled_mode.h"
#include "gfx/rect.h"
#include "render/onionskin_position.h"
#include "render/zoom.h"

#include "pref.xml.h"

#include <map>
#include <string>
#include <vector>

namespace app {
namespace tools {
class Tool;
}

class Doc;

typedef app::gen::ToolPref ToolPreferences;
typedef app::gen::DocPref DocumentPreferences;

class Preferences : public app::gen::GlobalPref,
                    public app::DocsObserver {
public:
  static Preferences& instance();

  Preferences();
  ~Preferences();

  void save() override;

  // Returns true if the given option was set by the user or false
  // if it contains the default value.
  bool isSet(OptionBase& opt) const;

  ToolPreferences& tool(tools::Tool* tool);
  DocumentPreferences& document(const Doc* doc);

  // Used to reset the tool preferences in scripting mode when the
  // UI is not available (so scripts have a common default
  // preferences and a reproducible behavior for automation).
  void resetToolPreferences(tools::Tool* tool);

  // Remove one document explicitly (this can be used if the
  // document used in Preferences::document() function wasn't member
  // of UIContext.
  void removeDocument(Doc* doc);

protected:
  // DocsObserver impl
  void onRemoveDocument(Doc* doc) override;

private:
  void load();
  std::string docConfigFileName(const Doc* doc);

  void serializeDocPref(const Doc* doc, app::DocumentPreferences* docPref, bool save);

  std::map<std::string, app::ToolPreferences*> m_tools;
  std::map<const Doc*, DocumentPreferences*> m_docs;
};

} // namespace app

#endif
