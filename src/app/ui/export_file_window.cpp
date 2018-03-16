// Aseprite
// Copyright (C) 2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/export_file_window.h"

#include "app/document.h"
#include "app/ui/layer_frame_comboboxes.h"
#include "app/ui_context.h"
#include "base/bind.h"
#include "base/convert_to.h"
#include "base/fs.h"
#include "doc/frame_tag.h"
#include "doc/selected_frames.h"
#include "doc/site.h"

namespace app {

ExportFileWindow::ExportFileWindow(const Document* doc)
  : m_doc(doc)
  , m_docPref(Preferences::instance().document(doc))
{
  // Is a default output filename in the preferences?
  if (!m_docPref.saveCopy.filename().empty()) {
    setOutputFilename(m_docPref.saveCopy.filename());
  }
  else {
    std::string newFn = base::replace_extension(
      doc->filename(),
      doc->sprite()->totalFrames() > 1 ? "gif": "png");
    if (newFn == doc->filename()) {
      newFn = base::join_path(
        base::get_file_path(newFn),
        base::get_file_title(newFn) + "-export." + base::get_file_extension(newFn));
    }
    setOutputFilename(newFn);
  }

  // Default export configuration
  resize()->setValue(
    base::convert_to<std::string>(m_docPref.saveCopy.resizeScale()));
  fill_layers_combobox(m_doc->sprite(), layers(), m_docPref.saveCopy.layer());
  fill_frames_combobox(m_doc->sprite(), frames(), m_docPref.saveCopy.frameTag());
  fill_anidir_combobox(anidir(), m_docPref.saveCopy.aniDir());
  pixelRatio()->setSelected(m_docPref.saveCopy.applyPixelRatio());

  updateAniDir();

  outputFilename()->Change.connect(
    base::Bind<void>(
      [this]{
        m_outputFilename = outputFilename()->text();
        onOutputFilenameEntryChange();
      }));
  outputFilenameBrowse()->Click.connect(
    base::Bind<void>(
      [this]{
        std::string fn = SelectOutputFile();
        if (!fn.empty()) {
          setOutputFilename(fn);
        }
      }));

  frames()->Change.connect(base::Bind<void>(&ExportFileWindow::updateAniDir, this));
}

bool ExportFileWindow::show()
{
  openWindowInForeground();
  return (closer() == ok());
}

void ExportFileWindow::savePref()
{
  m_docPref.saveCopy.filename(outputFilenameValue());
  m_docPref.saveCopy.resizeScale(resizeValue());
  m_docPref.saveCopy.layer(layersValue());
  m_docPref.saveCopy.frameTag(framesValue());
  m_docPref.saveCopy.applyPixelRatio(applyPixelRatio());
}

std::string ExportFileWindow::outputFilenameValue() const
{
  return base::join_path(m_outputPath,
                         m_outputFilename);
}

double ExportFileWindow::resizeValue() const
{
  return base::convert_to<double>(resize()->getValue());
}

std::string ExportFileWindow::layersValue() const
{
  return layers()->getValue();
}

std::string ExportFileWindow::framesValue() const
{
  return frames()->getValue();
}

doc::AniDir ExportFileWindow::aniDirValue() const
{
  return (doc::AniDir)anidir()->getSelectedItemIndex();
}

bool ExportFileWindow::applyPixelRatio() const
{
  return pixelRatio()->isSelected();
}

void ExportFileWindow::setOutputFilename(const std::string& pathAndFilename)
{
  m_outputPath = base::get_file_path(pathAndFilename);
  m_outputFilename = base::get_file_name(pathAndFilename);

  updateOutputFilenameEntry();
}

void ExportFileWindow::updateOutputFilenameEntry()
{
  outputFilename()->setText(m_outputFilename);
  onOutputFilenameEntryChange();
}

void ExportFileWindow::onOutputFilenameEntryChange()
{
  ok()->setEnabled(!m_outputFilename.empty());
}

void ExportFileWindow::updateAniDir()
{
  std::string framesValue = this->framesValue();
  if (!framesValue.empty() &&
      framesValue != kAllFrames &&
      framesValue != kSelectedFrames) {
    SelectedFrames selFrames;
    FrameTag* frameTag = calculate_selected_frames(
      UIContext::instance()->activeSite(), framesValue, selFrames);
    if (frameTag)
      anidir()->setSelectedItemIndex(int(frameTag->aniDir()));
  }
  else
    anidir()->setSelectedItemIndex(int(doc::AniDir::FORWARD));
}

} // namespace app
