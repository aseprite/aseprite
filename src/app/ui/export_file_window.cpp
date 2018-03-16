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
#include "base/bind.h"
#include "base/convert_to.h"
#include "base/fs.h"

namespace app {

ExportFileWindow::ExportFileWindow(const Document* doc)
  : m_doc(doc)
  , m_docPref(Preferences::instance().document(doc))
{
  // Is a default output filename in the preferences?
  if (!m_docPref.saveCopy.filename().empty()) {
    m_outputFilename = m_docPref.saveCopy.filename();
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
    m_outputFilename = newFn;
  }
  updateOutputFilenameButton();

  // Default export configuration
  resize()->setValue(
    base::convert_to<std::string>(m_docPref.saveCopy.resizeScale()));
  fill_layers_combobox(m_doc->sprite(), layers(), m_docPref.saveCopy.layer());
  fill_frames_combobox(m_doc->sprite(), frames(), m_docPref.saveCopy.frameTag());
  pixelRatio()->setSelected(m_docPref.saveCopy.applyPixelRatio());

  outputFilename()->Click.connect(base::Bind<void>(
    [this]{
      std::string fn = SelectOutputFile();
      if (!fn.empty()) {
        m_outputFilename = fn;
        updateOutputFilenameButton();
      }
    }));
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

bool ExportFileWindow::applyPixelRatio() const
{
  return pixelRatio()->isSelected();
}

void ExportFileWindow::updateOutputFilenameButton()
{
  outputFilename()->setText(base::get_file_name(m_outputFilename));
}

} // namespace app
