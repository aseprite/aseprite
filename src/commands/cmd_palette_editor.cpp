/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2011  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include "app.h"
#include "app/color.h"
#include "app/color_utils.h"
#include "base/bind.h"
#include "base/compiler_specific.h"
#include "commands/command.h"
#include "commands/params.h"
#include "console.h"
#include "dialogs/filesel.h"
#include "document_wrappers.h"
#include "gfx/hsv.h"
#include "gfx/rgb.h"
#include "gfx/size.h"
#include "gui/graphics.h"
#include "gui/gui.h"
#include "ini_file.h"
#include "modules/editors.h"
#include "modules/gui.h"
#include "modules/palettes.h"
#include "raster/image.h"
#include "raster/palette.h"
#include "raster/quantization.h"
#include "raster/sprite.h"
#include "raster/stock.h"
#include "skin/skin_slider_property.h"
#include "ui_context.h"
#include "undo/undo_history.h"
#include "undoers/set_palette_colors.h"
#include "widgets/color_bar.h"
#include "widgets/color_sliders.h"
#include "widgets/editor/editor.h"
#include "widgets/hex_color_entry.h"
#include "widgets/palette_view.h"
#include "widgets/statebar.h"

#include <allegro.h>
#include <stdio.h>
#include <string.h>
#include <vector>

using namespace gfx;

class PaletteEntryEditor : public Frame
{
public:
  PaletteEntryEditor();
  ~PaletteEntryEditor();

  void setColor(const Color& color);

protected:
  bool onProcessMessage(Message* msg) OVERRIDE;

  void onExit();
  void onCloseFrame();
  void onFgBgColorChange(const Color& color);
  void onColorSlidersChange(ColorSlidersChangeEvent& ev);
  void onColorHexEntryChange(const Color& color);
  void onColorTypeButtonClick(Event& ev);
  void onMoreOptionsClick(Event& ev);
  void onLoadCommand(Event& ev);
  void onSaveCommand(Event& ev);
  void onRampCommand(Event& ev);
  void onQuantizeCommand(Event& ev);
    
private:
  void selectColorType(Color::Type type);
  void setPaletteEntry(const Color& color);
  void setPaletteEntryChannel(const Color& color, ColorSliders::Channel channel);
  void setNewPalette(Palette* palette, const char* operationName);
  void updateCurrentSpritePalette(const char* operationName);
  void updateColorBar();

  Box m_vbox;
  Box m_topBox;
  Box m_bottomBox;
  RadioButton m_rgbButton;
  RadioButton m_hsvButton;
  HexColorEntry m_hexColorEntry;
  Label m_entryLabel;
  Button m_moreOptions;
  RgbSliders m_rgbSliders;
  HsvSliders m_hsvSliders;
  Button m_loadButton;
  Button m_saveButton;
  Button m_rampButton;
  Button m_quantizeButton;

  // This variable is used to avoid updating the m_hexColorEntry text
  // when the color change is generated from a
  // HexColorEntry::ColorChange signal. In this way we don't override
  // what the user is writting in the text field.
  bool m_disableHexUpdate;

  int m_redrawTimerId;
  bool m_redrawAll;
};

//////////////////////////////////////////////////////////////////////
// PaletteEditorCommand

static PaletteEntryEditor* g_frame = NULL;

class PaletteEditorCommand : public Command
{
public:
  PaletteEditorCommand();
  Command* clone() { return new PaletteEditorCommand(*this); }

protected:
  void onLoadParams(Params* params);
  void onExecute(Context* context);

private:
  bool m_open;
  bool m_close;
  bool m_switch;
  bool m_background;
};

PaletteEditorCommand::PaletteEditorCommand()
  : Command("PaletteEditor",
	    "PaletteEditor",
	    CmdRecordableFlag)
{
  m_open = true;
  m_close = false;
  m_switch = false;
  m_background = false;
}

void PaletteEditorCommand::onLoadParams(Params* params)
{
  std::string target = params->get("target");
  if (target == "foreground") m_background = false;
  else if (target == "background") m_background = true;

  std::string open_str = params->get("open");
  if (open_str == "true") m_open = true;
  else m_open = false;

  std::string close_str = params->get("close");
  if (close_str == "true") m_close = true;
  else m_close = false;

  std::string switch_str = params->get("switch");
  if (switch_str == "true") m_switch = true;
  else m_switch = false;
}

void PaletteEditorCommand::onExecute(Context* context)
{
  // If this is the first time the command is execute...
  if (!g_frame) {
    // If the command says "Close the palette editor" and it is not
    // created yet, we just do nothing.
    if (m_close)
      return;

    // If this is "open" or "switch", we have to create the frame.
    g_frame = new PaletteEntryEditor();
  }
  // If the frame is already created and it's visible, close it (only in "switch" or "close" modes)
  else if (g_frame->isVisible() && (m_switch || m_close)) {
    // Hide the frame
    g_frame->closeWindow(NULL);
    return;
  }

  if (m_switch || m_open) {
    if (!g_frame->isVisible()) {
      // Default bounds
      g_frame->remap_window();

      int width = MAX(jrect_w(g_frame->rc), JI_SCREEN_W/2);
      g_frame->setBounds(Rect(JI_SCREEN_W - width - jrect_w(app_get_toolbar()->rc),
			      JI_SCREEN_H - jrect_h(g_frame->rc) - jrect_h(app_get_statusbar()->rc),
			      width, jrect_h(g_frame->rc)));

      // Load window configuration
      load_window_pos(g_frame, "PaletteEditor");
    }

    // Run the frame in background.
    g_frame->open_window_bg();
    app_get_colorbar()->setPaletteEditorButtonState(true);
  }

  // Show the specified target color
  {
    Color color =
      (m_background ? context->getSettings()->getBgColor():
  		      context->getSettings()->getFgColor());

    g_frame->setColor(color);
  }
}

//////////////////////////////////////////////////////////////////////
// PaletteEntryEditor implementation
//
// Based on ColorSelector class.

PaletteEntryEditor::PaletteEntryEditor()
  : Frame(false, "Palette Editor (F4)")
  , m_vbox(JI_VERTICAL)
  , m_topBox(JI_HORIZONTAL)
  , m_bottomBox(JI_HORIZONTAL)
  , m_rgbButton("RGB", 1, JI_BUTTON)
  , m_hsvButton("HSV", 1, JI_BUTTON)
  , m_entryLabel("")
  , m_moreOptions("+")
  , m_loadButton("Load")
  , m_saveButton("Save")
  , m_rampButton("Ramp")
  , m_quantizeButton("Quantize")
  , m_disableHexUpdate(false)
  , m_redrawAll(false)
{
  m_redrawTimerId = jmanager_add_timer(this, 250);

  m_topBox.setBorder(gfx::Border(0));
  m_topBox.child_spacing = 0;
  m_bottomBox.setBorder(gfx::Border(0));

  setup_mini_look(&m_rgbButton);
  setup_mini_look(&m_hsvButton);
  setup_mini_look(&m_moreOptions);
  setup_mini_look(&m_loadButton);
  setup_mini_look(&m_saveButton);
  setup_mini_look(&m_rampButton);
  setup_mini_look(&m_quantizeButton);

  // Top box
  m_topBox.addChild(&m_rgbButton);
  m_topBox.addChild(&m_hsvButton);
  m_topBox.addChild(&m_hexColorEntry);
  m_topBox.addChild(&m_entryLabel);
  m_topBox.addChild(new BoxFiller);
  m_topBox.addChild(&m_moreOptions);

  // Bottom box
  {
    Box* box = new Box(JI_HORIZONTAL);
    box->child_spacing = 0;
    box->addChild(&m_loadButton);
    box->addChild(&m_saveButton);
    m_bottomBox.addChild(box);
  }
  m_bottomBox.addChild(&m_rampButton);
  m_bottomBox.addChild(&m_quantizeButton);

  // Main vertical box
  m_vbox.addChild(&m_topBox);
  m_vbox.addChild(&m_rgbSliders);
  m_vbox.addChild(&m_hsvSliders);
  m_vbox.addChild(&m_bottomBox);
  addChild(&m_vbox);

  // Hide (or show) the "More Options" depending the saved value in .cfg file
  m_bottomBox.setVisible(get_config_bool("PaletteEditor", "ShowMoreOptions", false));

  m_rgbButton.Click.connect(&PaletteEntryEditor::onColorTypeButtonClick, this);
  m_hsvButton.Click.connect(&PaletteEntryEditor::onColorTypeButtonClick, this);
  m_moreOptions.Click.connect(&PaletteEntryEditor::onMoreOptionsClick, this);
  m_loadButton.Click.connect(&PaletteEntryEditor::onLoadCommand, this);
  m_saveButton.Click.connect(&PaletteEntryEditor::onSaveCommand, this);
  m_rampButton.Click.connect(&PaletteEntryEditor::onRampCommand, this);
  m_quantizeButton.Click.connect(&PaletteEntryEditor::onQuantizeCommand, this);

  m_rgbSliders.ColorChange.connect(&PaletteEntryEditor::onColorSlidersChange, this);
  m_hsvSliders.ColorChange.connect(&PaletteEntryEditor::onColorSlidersChange, this);
  m_hexColorEntry.ColorChange.connect(&PaletteEntryEditor::onColorHexEntryChange, this);

  selectColorType(Color::RgbType);

  // We hook fg/bg color changes (by eyedropper mainly) to update the selected entry color
  app_get_colorbar()->FgColorChange.connect(&PaletteEntryEditor::onFgBgColorChange, this);
  app_get_colorbar()->BgColorChange.connect(&PaletteEntryEditor::onFgBgColorChange, this);

  // We hook the Frame::Close event to save the frame position before closing it.
  this->Close.connect(Bind<void>(&PaletteEntryEditor::onCloseFrame, this));

  // We hook App::Exit signal to destroy the g_frame singleton at exit.
  App::instance()->Exit.connect(&PaletteEntryEditor::onExit, this);

  initTheme();
}

PaletteEntryEditor::~PaletteEntryEditor()
{
  jmanager_remove_timer(m_redrawTimerId);
  m_redrawTimerId = -1;
}

void PaletteEntryEditor::setColor(const Color& color)
{
  m_rgbSliders.setColor(color);
  m_hsvSliders.setColor(color);
  if (!m_disableHexUpdate)
    m_hexColorEntry.setColor(color);

  PaletteView* palette_editor = app_get_colorbar()->getPaletteView();
  PaletteView::SelectedEntries entries;
  palette_editor->getSelectedEntries(entries);
  int i, j, i2;

  // Find the first selected entry
  for (i=0; i<(int)entries.size(); ++i)
    if (entries[i])
      break;

  // Find the first unselected entry after i
  for (i2=i+1; i2<(int)entries.size(); ++i2)
    if (!entries[i2])
      break;

  // Find the last selected entry
  for (j=entries.size()-1; j>=0; --j)
    if (entries[j])
      break;

  if (i == j) {
    m_entryLabel.setTextf(" Entry: %d", i);
  }
  else if (j-i+1 == i2-i) {
    m_entryLabel.setTextf(" Range: %d-%d", i, j);
  }
  else {
    m_entryLabel.setText(" Multiple Entries");
  }

  m_topBox.layout();
}

bool PaletteEntryEditor::onProcessMessage(Message* msg)
{
  if (msg->type == JM_TIMER &&
      msg->timer.timer_id == m_redrawTimerId) {
    // Redraw all editors
    if (m_redrawAll) {
      m_redrawAll = false;
      jmanager_stop_timer(m_redrawTimerId);

      // Call all listener of PaletteChange event.
      App::instance()->PaletteChange();

      // Redraw all editors
      try {
	const ActiveDocumentReader document(UIContext::instance());
	update_editors_with_document(document);
      }
      catch (...) {
	// Do nothing
      }
    }
    // Redraw just the current editor
    else {
      m_redrawAll = true;
      current_editor->updateEditor();
    }
  }
  return Frame::onProcessMessage(msg);
}

void PaletteEntryEditor::onExit()
{
  delete this;
}

void PaletteEntryEditor::onCloseFrame()
{
  // Save window configuration
  save_window_pos(this, "PaletteEditor");

  // Uncheck the "Edit Palette" button.
  app_get_colorbar()->setPaletteEditorButtonState(false);
}

void PaletteEntryEditor::onFgBgColorChange(const Color& color)
{
  if (color.isValid() && color.getType() == Color::IndexType) {
    setColor(color);
  }
}

void PaletteEntryEditor::onColorSlidersChange(ColorSlidersChangeEvent& ev)
{
  setColor(ev.getColor());
  setPaletteEntryChannel(ev.getColor(), ev.getModifiedChannel());
  updateCurrentSpritePalette("Color Change");
  updateColorBar();
}

void PaletteEntryEditor::onColorHexEntryChange(const Color& color)
{
  // Disable updating the hex entry so we don't override what the user
  // is writting in the text field.
  m_disableHexUpdate = true;

  setColor(color);
  setPaletteEntry(color);
  updateCurrentSpritePalette("Color Change");
  updateColorBar();

  m_disableHexUpdate = false;
}

void PaletteEntryEditor::onColorTypeButtonClick(Event& ev)
{
  RadioButton* source = static_cast<RadioButton*>(ev.getSource());

  if (source == &m_rgbButton) selectColorType(Color::RgbType);
  else if (source == &m_hsvButton) selectColorType(Color::HsvType);
}

void PaletteEntryEditor::onMoreOptionsClick(Event& ev)
{
  Size reqSize;

  if (m_bottomBox.isVisible()) {
    set_config_bool("PaletteEditor", "ShowMoreOptions", false);
    m_bottomBox.setVisible(false);

    // Get the required size of the "More options" panel
    reqSize = m_bottomBox.getPreferredSize();
    reqSize.h += 4;

    // Remove the space occupied by the "More options" panel
    {
      JRect rect = jrect_new(rc->x1, rc->y1,
			     rc->x2, rc->y2 - reqSize.h);
      move_window(rect);
      jrect_free(rect);
    }
  }
  else {
    set_config_bool("PaletteEditor", "ShowMoreOptions", true);
    m_bottomBox.setVisible(true);

    // Get the required size of the whole window
    reqSize = getPreferredSize();

    // Add space for the "more_options" panel
    if (jrect_h(rc) < reqSize.h) {
      JRect rect = jrect_new(rc->x1, rc->y1,
			     rc->x2, rc->y1 + reqSize.h);

      // Show the expanded area inside the screen
      if (rect->y2 > JI_SCREEN_H)
	jrect_displace(rect, 0, JI_SCREEN_H - rect->y2);
      
      move_window(rect);
      jrect_free(rect);
    }
    else
      setBounds(getBounds()); // TODO layout() method is missing
  }

  // Redraw the window
  invalidate();
}

void PaletteEntryEditor::onLoadCommand(Event& ev)
{
  Palette *palette;
  base::string filename = ase_file_selector("Load Palette", "", "png,pcx,bmp,tga,lbm,col");
  if (!filename.empty()) {
    palette = Palette::load(filename.c_str());
    if (!palette) {
      Alert::show("Error<<Loading palette file||&Close");
    }
    else {
      setNewPalette(palette, "Load Palette");
      delete palette;
    }
  }
}

void PaletteEntryEditor::onSaveCommand(Event& ev)
{
  base::string filename;
  int ret;

 again:
  filename = ase_file_selector("Save Palette", "", "png,pcx,bmp,tga,col");
  if (!filename.empty()) {
    if (exists(filename.c_str())) {
      ret = Alert::show("Warning<<File exists, overwrite it?<<%s||&Yes||&No||&Cancel",
			get_filename(filename.c_str()));

      if (ret == 2)
	goto again;
      else if (ret != 1)
	return;
    }

    Palette* palette = get_current_palette();
    if (!palette->save(filename.c_str())) {
      Alert::show("Error<<Saving palette file||&Close");
    }
  }
}

void PaletteEntryEditor::onRampCommand(Event& ev)
{
  PaletteView* palette_editor = app_get_colorbar()->getPaletteView();
  int index1, index2;

  if (!palette_editor->getSelectedRange(index1, index2))
    return;

  Palette* src_palette = get_current_palette();
  Palette* dst_palette = new Palette(0, 256);

  src_palette->copyColorsTo(dst_palette);
  dst_palette->makeHorzRamp(index1, index2);

  setNewPalette(dst_palette, "Color Ramp");
  delete dst_palette;
}

void PaletteEntryEditor::onQuantizeCommand(Event& ev)
{
  Palette* palette = NULL;

  {
    const ActiveDocumentReader document(UIContext::instance());
    const Sprite* sprite(document ? document->getSprite(): NULL);

    if (sprite == NULL) {
      Alert::show("Error<<There is no sprite selected to quantize.||&OK");
      return;
    }

    if (sprite->getImgType() != IMAGE_RGB) {
      Alert::show("Error<<You can use this command only for RGB sprites||&OK");
      return;
    }

    palette = quantization::create_palette_from_rgb(sprite);
  }

  setNewPalette(palette, "Quantize Palette");
  delete palette;
}

void PaletteEntryEditor::setPaletteEntry(const Color& color)
{
  PaletteView* palView = app_get_colorbar()->getPaletteView();
  PaletteView::SelectedEntries entries;
  palView->getSelectedEntries(entries);

  uint32_t new_pal_color = _rgba(color.getRed(),
				 color.getGreen(),
				 color.getBlue(), 255);

  Palette* palette = get_current_palette();
  for (int c=0; c<palette->size(); c++) {
    if (entries[c])
      palette->setEntry(c, new_pal_color);
  }
}

void PaletteEntryEditor::setPaletteEntryChannel(const Color& color, ColorSliders::Channel channel)
{
  PaletteView* palView = app_get_colorbar()->getPaletteView();
  PaletteView::SelectedEntries entries;
  palView->getSelectedEntries(entries);

  uint32_t src_color;
  int r, g, b;

  Palette* palette = get_current_palette();
  for (int c=0; c<palette->size(); c++) {
    if (entries[c]) {
      // Get the current RGB values of the palette entry
      src_color = palette->getEntry(c);
      r = _rgba_getr(src_color);
      g = _rgba_getg(src_color);
      b = _rgba_getb(src_color);

      switch (color.getType()) {

        case Color::RgbType:
	  // Setup the new RGB values depending of the modified channel.
	  switch (channel) {
	    case ColorSliders::Red:
	      r = color.getRed();
	    case ColorSliders::Green:
	      g = color.getGreen();
	      break;
	    case ColorSliders::Blue:
	      b = color.getBlue();
	      break;
	  }
	  break;

        case Color::HsvType:
	  {
	    // Convert RGB to HSV
	    Hsv hsv(Rgb(r, g, b));

	    // Only modify the desired HSV channel
	    switch (channel) {
	      case ColorSliders::Hue:
		hsv.hue(color.getHue());
		break;
	      case ColorSliders::Saturation:
		hsv.saturation(double(color.getSaturation()) / 100.0);
		break;
	      case ColorSliders::Value:
		hsv.value(double(color.getValue()) / 100.0);
		break;
	    }

	    // Convert HSV back to RGB
	    Rgb rgb(hsv);
	    r = rgb.red();
	    g = rgb.green();
	    b = rgb.blue();
	  }
	  break;
      }

      palette->setEntry(c, _rgba(r, g, b, 255));
    }
  }
}

void PaletteEntryEditor::selectColorType(Color::Type type)
{
  m_rgbSliders.setVisible(type == Color::RgbType);
  m_hsvSliders.setVisible(type == Color::HsvType);

  switch (type) {
    case Color::RgbType: m_rgbButton.setSelected(true); break;
    case Color::HsvType: m_hsvButton.setSelected(true); break;
  }
  
  m_vbox.layout();
  m_vbox.invalidate();
}

void PaletteEntryEditor::setNewPalette(Palette* palette, const char* operationName)
{
  // Copy the palette
  palette->copyColorsTo(get_current_palette());

  // Set the palette calling the hooks
  set_current_palette(palette, false);

  // Update the sprite palette
  updateCurrentSpritePalette(operationName);

  // Redraw the entire screen
  jmanager_refresh_screen();
}

void PaletteEntryEditor::updateCurrentSpritePalette(const char* operationName)
{
  if (UIContext::instance()->getActiveDocument() &&
      UIContext::instance()->getActiveDocument()->getSprite()) {
    try {
      ActiveDocumentWriter document(UIContext::instance());
      Sprite* sprite(document->getSprite());
      undo::UndoHistory* undo = document->getUndoHistory();
      Palette* newPalette = get_current_palette(); // System current pal
      Palette* currentSpritePalette = sprite->getPalette(sprite->getCurrentFrame()); // Sprite current pal
      int from, to;

      // Check differences between current sprite palette and current system palette
      from = to = -1;
      currentSpritePalette->countDiff(newPalette, &from, &to);

      if (from >= 0 && to >= from) {
	// Add undo information to save the range of pal entries that will be modified.
	if (undo->isEnabled()) {
	  undo->setLabel(operationName);
	  undo->setModification(undo::ModifyDocument);
	  undo->pushUndoer(new undoers::SetPaletteColors(undo->getObjects(),
	      sprite, currentSpritePalette, from, to));
	}

	// Change the sprite palette
	sprite->setPalette(newPalette, false);
      }
    }
    catch (base::Exception& e) {
      Console::showException(e);
    }
  }

  PaletteView* palette_editor = app_get_colorbar()->getPaletteView();
  palette_editor->invalidate();

  if (!jmanager_timer_is_running(m_redrawTimerId))
    jmanager_start_timer(m_redrawTimerId);

  m_redrawAll = false;
}

void PaletteEntryEditor::updateColorBar()
{
  app_get_colorbar()->invalidate();
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::createPaletteEditorCommand()
{
  return new PaletteEditorCommand;
}
