// Aseprite
// Copyright (C) 2019-2024  Igara Studio S.A.
// Copyright (C) 2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/ui/export_file_window.h"

#include "app/doc.h"
#include "app/file/file.h"
#include "app/i18n/strings.h"
#include "app/site.h"
#include "app/ui/layer_frame_comboboxes.h"
#include "app/ui_context.h"
#include "base/convert_to.h"
#include "base/fs.h"
#include "base/string.h"
#include "doc/selected_frames.h"
#include "doc/tag.h"
#include "fmt/format.h"
#include "ui/alert.h"

#include <algorithm>

namespace app {

ExportFileWindow::ExportFileWindow(const Doc* doc)
  : m_doc(doc)
  , m_docPref(Preferences::instance().document(doc))
  , m_preferredResize(1)
{
  // Is a default output filename in the preferences?
  outputField()->setDocFilename(doc->filename());
  if (!m_docPref.saveCopy.filename().empty()) {
    outputField()->setFilename(m_docPref.saveCopy.filename());
  }
  else {
    std::string base = doc->filename();
    const std::string basePath = (base::get_file_path(base).empty() ? base::get_current_path() :
                                                                      base::get_file_path(base));
    base = base::join_path(basePath, base::get_file_title(base));

    std::string newFn = base::replace_extension(base, defaultExtension());
    if (newFn == base) {
      newFn = base::join_path(
        base::get_file_path(newFn),
        base::get_file_title(newFn) + "-export." + base::get_file_extension(newFn));
    }
    outputField()->setFilename(newFn);
  }

  // Default export configuration
  setResizeScale(m_docPref.saveCopy.resizeScale());
  fill_area_combobox(m_doc->sprite(), area(), m_docPref.saveCopy.area());
  fill_layers_combobox(m_doc->sprite(),
                       layers(),
                       m_docPref.saveCopy.layer(),
                       m_docPref.saveCopy.layerIndex());
  fill_frames_combobox(m_doc->sprite(), frames(), m_docPref.saveCopy.frameTag());
  fill_anidir_combobox(anidir(), m_docPref.saveCopy.aniDir());

  if (doc->sprite()->hasPixelRatio()) {
    pixelRatio()->setSelected(m_docPref.saveCopy.applyPixelRatio());
  }
  else {
    // Hide "Apply pixel ratio" checkbox when there is no pixel aspect
    // ratio to apply.
    pixelRatio()->setSelected(false);
    pixelRatio()->setVisible(false);
  }

  outputField()->Change.connect([this] { onOutputFieldChange(); });
  forTwitter()->setSelected(m_docPref.saveCopy.forTwitter());
  adjustResize()->setVisible(false);
  playSubtags()->setSelected(m_docPref.saveCopy.playSubtags());
  // Here we don't call updateAniDir() because it's already filled and
  // set by the function fill_anidir_combobox(). So if the user
  // exported a tag with a specific AniDir, we want to keep the option
  // in the preference (instead of the tag's AniDir).
  // updateAniDir();
  updatePlaySubtags();
  updateAdjustResizeButton();

  resize()->Change.connect([this] { updateAdjustResizeButton(); });
  frames()->Change.connect([this] {
    updateAniDir();
    updatePlaySubtags();
  });
  forTwitter()->Click.connect([this] { updateAdjustResizeButton(); });
  adjustResize()->Click.connect([this] { onAdjustResize(); });
  ok()->Click.connect([this] { onOK(); });
}

bool ExportFileWindow::show()
{
  openWindowInForeground();
  return (closer() == ok());
}

void ExportFileWindow::savePref()
{
  m_docPref.saveCopy.filename(outputField()->fullFilename());
  m_docPref.saveCopy.resizeScale(resizeValue());
  m_docPref.saveCopy.area(areaValue());
  m_docPref.saveCopy.layer(layersValue());
  m_docPref.saveCopy.layerIndex(layersIndex());
  m_docPref.saveCopy.aniDir(aniDirValue());
  m_docPref.saveCopy.frameTag(framesValue());
  m_docPref.saveCopy.applyPixelRatio(applyPixelRatio());
  m_docPref.saveCopy.forTwitter(isForTwitter());
  m_docPref.saveCopy.playSubtags(isPlaySubtags());
}

double ExportFileWindow::resizeValue() const
{
  double value = resize()->getEntryWidget()->textDouble() / 100.0;
  return std::clamp(value, 0.001, 100000000.0);
}

std::string ExportFileWindow::areaValue() const
{
  return area()->getValue();
}

std::string ExportFileWindow::layersValue() const
{
  return layers()->getValue();
}

int ExportFileWindow::layersIndex() const
{
  int i = layers()->getSelectedItemIndex() - kLayersComboboxExtraInitialItems;
  return i < 0 ? -1 : i;
}

std::string ExportFileWindow::framesValue() const
{
  return frames()->getValue();
}

doc::AniDir ExportFileWindow::aniDirValue() const
{
  return (doc::AniDir)anidir()->getSelectedItemIndex();
}

bool ExportFileWindow::isPlaySubtags() const
{
  return playSubtags()->isSelected() && framesValue() != kSelectedFrames;
}

bool ExportFileWindow::applyPixelRatio() const
{
  return pixelRatio()->isSelected();
}

bool ExportFileWindow::isForTwitter() const
{
  return forTwitter()->isSelected();
}

void ExportFileWindow::setResizeScale(double scale)
{
  resize()->setValue(fmt::format("{:.2f}", 100.0 * scale));
}

void ExportFileWindow::setArea(const std::string& areaValue)
{
  area()->setValue(areaValue);
}

void ExportFileWindow::setAniDir(const doc::AniDir aniDir)
{
  anidir()->setSelectedItemIndex(int(aniDir));
}

void ExportFileWindow::onOutputFieldChange()
{
  ok()->setEnabled(!outputField()->filename().empty());
}

void ExportFileWindow::updateAniDir()
{
  std::string framesValue = this->framesValue();
  if (!framesValue.empty() && framesValue != kAllFrames && framesValue != kSelectedFrames) {
    SelectedFrames selFrames;
    Tag* tag =
      calculate_selected_frames(UIContext::instance()->activeSite(), framesValue, selFrames);
    if (tag)
      anidir()->setSelectedItemIndex(int(tag->aniDir()));
  }
  else
    anidir()->setSelectedItemIndex(int(doc::AniDir::FORWARD));
}

void ExportFileWindow::updatePlaySubtags()
{
  std::string framesValue = this->framesValue();
  playSubtags()->setVisible(framesValue != kSelectedFrames &&
                            // We hide the option if there is no tag
                            !m_doc->sprite()->tags().empty());
  layout();
}

void ExportFileWindow::updateAdjustResizeButton()
{
  // Calculate a better size for Twitter
  m_preferredResize = 1;
  while (m_preferredResize < 10 &&
         (m_doc->width() * m_preferredResize < 240 || m_doc->height() * m_preferredResize < 240)) {
    ++m_preferredResize;
  }

  const bool newState = forTwitter()->isSelected() && ((int)resizeValue() < m_preferredResize);

  if (adjustResize()->isVisible() != newState) {
    adjustResize()->setVisible(newState);
    if (newState) {
      adjustResize()->setText(Strings::export_file_adjust_resize(100 * m_preferredResize));
    }
    adjustResize()->parent()->layout();
  }
}

void ExportFileWindow::onAdjustResize()
{
  resize()->setValue(fmt::format("{:.2f}", 100.0 * m_preferredResize));

  adjustResize()->setVisible(false);
  adjustResize()->parent()->layout();
}

void ExportFileWindow::onOK()
{
  base::paths exts = get_writable_extensions();
  std::string ext = base::string_to_lower(base::get_file_extension(outputField()->filename()));

  // Add default extension to output filename
  if (std::find(exts.begin(), exts.end(), ext) == exts.end()) {
    if (ext.empty()) {
      outputField()->setFilenameQuiet(
        base::replace_extension(outputField()->filename(), defaultExtension()));
    }
    else {
      ui::Alert::show(Strings::alerts_unknown_output_file_format_error(ext));
      return;
    }
  }

  closeWindow(ok());
}

std::string ExportFileWindow::defaultExtension() const
{
  auto& pref = Preferences::instance();
  if (m_doc->sprite()->totalFrames() > 1)
    return pref.exportFile.animationDefaultExtension();
  else
    return pref.exportFile.imageDefaultExtension();
}

} // namespace app
