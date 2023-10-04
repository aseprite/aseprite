// Aseprite
// Copyright (C) 2018-2023  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/commands/commands.h"
#include "app/commands/params.h"
#include "app/context_access.h"
#include "app/doc_access.h"
#include "app/doc_event.h"
#include "app/doc_range.h"
#include "app/i18n/strings.h"
#include "app/modules/gfx.h"
#include "app/modules/gui.h"
#include "app/modules/palettes.h"
#include "app/pref/preferences.h"
#include "app/tools/active_tool.h"
#include "app/tools/tool.h"
#include "app/ui/button_set.h"
#include "app/ui/color_button.h"
#include "app/ui/editor/editor.h"
#include "app/ui/keyboard_shortcuts.h"
#include "app/ui/main_window.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/status_bar.h"
#include "app/ui/timeline/timeline.h"
#include "app/ui/toolbar.h"
#include "app/ui/zoom_entry.h"
#include "app/ui_context.h"
#include "app/util/range_utils.h"
#include "app/util/tile_flags_utils.h"
#include "base/fs.h"
#include "base/string.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/sprite.h"
#include "fmt/format.h"
#include "gfx/size.h"
#include "os/font.h"
#include "os/surface.h"
#include "ui/ui.h"
#include "ver/info.h"

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <limits>
#include <vector>

namespace app {

using namespace app::skin;
using namespace gfx;
using namespace ui;
using namespace doc;

class StatusBar::AboutStatusBar : public HBox {
public:
  AboutStatusBar()
    : m_label(fmt::format("{} {} by ", get_app_name(), get_app_version()))
    , m_link("", "Igara Studio")
  {
    m_link.Click.connect(
      []{
        Command* cmd = Commands::instance()->byId(CommandId::About());
        UIContext::instance()->executeCommandFromMenuOrShortcut(cmd);
      });

    addChild(new BoxFiller);
    addChild(&m_label);
    addChild(&m_link);
    addChild(new BoxFiller);

    InitTheme.connect(
      [this]{
        auto theme = SkinTheme::get(this);
        ui::Style* style = theme->styles.workspaceLink();
        noBorderNoChildSpacing();
        m_label.setStyle(style);
        m_link.setStyle(style);
        m_label.noBorderNoChildSpacing();
        m_link.noBorderNoChildSpacing();
      });
    initTheme();
  }

  ui::Label m_label;
  ui::LinkLabel m_link;
};

class StatusBar::Indicators : public HBox {

  class Indicator : public Widget {
  public:
    enum IndicatorType {
      kText,
      kIcon,
      kColor,
      kTile
    };
    Indicator(IndicatorType type) : m_type(type) { }
    IndicatorType indicatorType() const { return m_type; }
  private:
    IndicatorType m_type;
  };

  class TextIndicator : public Indicator {
  public:
    TextIndicator(const char* text) : Indicator(kText) {
      updateIndicator(text);
    }

    bool updateIndicator(const char* text) {
      if (this->text() == text)
        return false;

      setText(text);

      if (minSize().w > textSize().w*2)
        setMinSize(textSize());
      else
        setMinSize(minSize().createUnion(textSize()));
      return true;
    }

  private:
    void onPaint(ui::PaintEvent& ev) override {
      auto theme = SkinTheme::get(this);
      gfx::Color textColor = theme->colors.statusBarText();
      Rect rc = clientBounds();
      Graphics* g = ev.graphics();

      g->fillRect(bgColor(), rc);
      if (!text().empty()) {
        g->drawText(text(), textColor, ColorNone,
                    Point(rc.x, rc.y + rc.h/2 - font()->height()/2));
      }
    }
  };

  class IconIndicator : public Indicator {
  public:
    IconIndicator(const skin::SkinPartPtr& part, bool colored)
      : Indicator(kIcon)
      , m_part(part)
      , m_colored(colored) {
      InitTheme.connect(
        [this]{
          updateIndicator();
        });
      initTheme();
    }

    bool updateIndicator(const skin::SkinPartPtr& part, bool colored) {
      if (m_part.get() == part.get() &&
          m_colored == colored)
        return false;

      ASSERT(part);
      m_part = part;
      m_colored = colored;
      updateIndicator();
      return true;
    }

  private:
    void updateIndicator() {
      setMinSize(
        minSize().createUnion(Size(m_part->bitmap(0)->width(),
                                   m_part->bitmap(0)->height())));
    }

    void onPaint(ui::PaintEvent& ev) override {
      auto theme = SkinTheme::get(this);
      gfx::Color textColor = theme->colors.statusBarText();
      Rect rc = clientBounds();
      Graphics* g = ev.graphics();
      os::Surface* icon = m_part->bitmap(0);

      g->fillRect(bgColor(), rc);
      if (m_colored)
        g->drawColoredRgbaSurface(
          icon, textColor,
          rc.x, rc.y + rc.h/2 - icon->height()/2);
      else
        g->drawRgbaSurface(
          icon, rc.x, rc.y + rc.h/2 - icon->height()/2);
    }

    skin::SkinPartPtr m_part;
    bool m_colored;
  };

  class ColorIndicator : public Indicator {
  public:
    ColorIndicator(const app::Color& color)
      : Indicator(kColor)
      , m_color(Color::fromMask()) {
      updateIndicator(color, true);
    }

    bool updateIndicator(const app::Color& color, bool first = false) {
      if (m_color == color && !first)
        return false;

      m_color = color;
      setMinSize(minSize().createUnion(Size(32*guiscale(), 1)));
      return true;
    }

  private:
    void onPaint(ui::PaintEvent& ev) override {
      Rect rc = clientBounds();
      Graphics* g = ev.graphics();

      g->fillRect(bgColor(), rc);
      draw_color_button(
        g, Rect(rc.x, rc.y, 32*guiscale(), rc.h),
        m_color,
        (doc::ColorMode)app_get_current_pixel_format(), false, false);
    }

    app::Color m_color;
  };

  class TileIndicator : public Indicator {
  public:
    TileIndicator(doc::tile_t tile)
      : Indicator(kTile)
      , m_tile(doc::notile) {
      updateIndicator(tile, true);
    }

    bool updateIndicator(doc::tile_t tile, bool first = false) {
      if (m_tile == tile && !first)
        return false;

      m_tile = tile;
      setMinSize(minSize().createUnion(Size(32*guiscale(), 1)));
      return true;
    }

  private:
    void onPaint(ui::PaintEvent& ev) override {
      Rect rc = clientBounds();
      Graphics* g = ev.graphics();

      // TODO could the site came from the Indicators or StatusBar itself
      Site site = UIContext::instance()->activeSite();

      g->fillRect(bgColor(), rc);
      draw_tile_button(
        g, Rect(rc.x, rc.y, 32*guiscale(), rc.h),
        site, m_tile,
        false, false);
    }

    doc::tile_t m_tile;
  };

public:

  Indicators()
    : m_backupIcon(BackupIcon::None)
    , m_redraw(true) {
    m_leftArea.setBorder(gfx::Border(0));
    m_leftArea.setVisible(true);
    m_leftArea.setExpansive(true);

    m_rightArea.setBorder(gfx::Border(0));
    m_rightArea.setVisible(false);

    addChild(&m_leftArea);
    addChild(&m_rightArea);
  }

  void startIndicators() {
    m_iterator = m_indicators.begin();
  }

  void endIndicators() {
    removeAllNextIndicators();
    if (m_redraw) {
      m_redraw = false;
      layout();
    }
  }

  void addTextIndicator(const char* text) {
    // Re-use indicator
    if (m_iterator != m_indicators.end()) {
      if ((*m_iterator)->indicatorType() == Indicator::kText) {
        m_redraw |= static_cast<TextIndicator*>(*m_iterator)
          ->updateIndicator(text);
        ++m_iterator;
        return;
      }
      else {
        removeAllNextIndicators();
      }
    }

    auto indicator = new TextIndicator(text);
    m_indicators.push_back(indicator);
    m_iterator = m_indicators.end();
    m_leftArea.addChild(indicator);
    m_redraw = true;
  }

  void addIconIndicator(const skin::SkinPartPtr& part, bool colored) {
    if (m_iterator != m_indicators.end()) {
      if ((*m_iterator)->indicatorType() == Indicator::kIcon) {
        m_redraw |= static_cast<IconIndicator*>(*m_iterator)
          ->updateIndicator(part, colored);
        ++m_iterator;
        return;
      }
      else
        removeAllNextIndicators();
    }

    auto indicator = new IconIndicator(part, colored);
    m_indicators.push_back(indicator);
    m_iterator = m_indicators.end();
    m_leftArea.addChild(indicator);
    m_redraw = true;
  }

  void addColorIndicator(const app::Color& color) {
    if (m_iterator != m_indicators.end()) {
      if ((*m_iterator)->indicatorType() == Indicator::kColor) {
        m_redraw |= static_cast<ColorIndicator*>(*m_iterator)
          ->updateIndicator(color);
        ++m_iterator;
        return;
      }
      else
        removeAllNextIndicators();
    }

    auto indicator = new ColorIndicator(color);
    m_indicators.push_back(indicator);
    m_iterator = m_indicators.end();
    m_leftArea.addChild(indicator);
    m_redraw = true;
  }

  void addTileIndicator(doc::tile_t tile) {
    if (m_iterator != m_indicators.end()) {
      if ((*m_iterator)->indicatorType() == Indicator::kTile) {
        m_redraw |= static_cast<TileIndicator*>(*m_iterator)
          ->updateIndicator(tile);
        ++m_iterator;
        return;
      }
      else
        removeAllNextIndicators();
    }

    auto indicator = new TileIndicator(tile);
    m_indicators.push_back(indicator);
    m_iterator = m_indicators.end();
    m_leftArea.addChild(indicator);
    m_redraw = true;
  }

  void showBackupIcon(BackupIcon icon) {
    m_backupIcon = icon;
    if (m_backupIcon != BackupIcon::None) {
      auto theme = SkinTheme::get(this);
      SkinPartPtr part =
        (m_backupIcon == BackupIcon::Normal ?
         theme->parts.iconSave():
         theme->parts.iconSaveSmall());

      m_rightArea.setVisible(true);
      if (m_rightArea.children().empty()) {
        m_rightArea.addChild(new IconIndicator(part, true));
      }
      else {
        ((IconIndicator*)m_rightArea.lastChild())->updateIndicator(part, true);
      }
    }
    else {
      m_rightArea.setVisible(false);
    }
    layout();
  }

private:
  void removeAllNextIndicators() {
    auto it = m_iterator;
    auto end = m_indicators.end();
    if (m_iterator != end) {
      for (; it != end; ++it) {
        auto indicator = *it;
        m_leftArea.removeChild(indicator);
        delete indicator;
      }
      m_indicators.erase(m_iterator, end);
      m_redraw = true;
    }
  }

  std::vector<Indicator*> m_indicators;
  std::vector<Indicator*>::iterator m_iterator;
  BackupIcon m_backupIcon;
  HBox m_leftArea;
  HBox m_rightArea;
  bool m_redraw;
};

class StatusBar::IndicatorsGeneration {
public:
  IndicatorsGeneration(StatusBar::Indicators* indicators)
    : m_indicators(indicators) {
    m_indicators->startIndicators();
  }

  ~IndicatorsGeneration() {
    m_indicators->endIndicators();
  }

  IndicatorsGeneration& add(const char* text) {
    auto theme = SkinTheme::get(m_indicators);

    for (auto i = text; *i; ) {
      // Icon
      if (*i == ':' && (i == text || *(i-1) == ' ')) {
        const char* j = i+1;
        for (; *j; ++j) {
          if (*j == ':')
            break;
        }

        if (*j == ':' && (*(j+1) == 0 || *(j+1) == ' ')) {
          if (i != text) {
            // Here i is ':' and i-1 is a whitespace ' '
            m_indicators->addTextIndicator(std::string(text, i-1).c_str());
          }

          auto part = theme->getPartById("icon_" + std::string(i+1, j));
          if (part)
            add(part, true);

          text = i = (*(j+1) == ' ' ? j+2: j+1);
        }
        else
          i = j;
      }
      else
        ++i;
    }

    if (*text != 0)
      m_indicators->addTextIndicator(text);

    return *this;
  }

  IndicatorsGeneration& add(const skin::SkinPartPtr& part, bool colored) {
    if (part.get())
      m_indicators->addIconIndicator(part, colored);
    return *this;
  }

  IndicatorsGeneration& add(const app::Color& color) {
    // For Color::TileType, use the tile version
    if (color.getType() == app::Color::TileType) {
      return add(color.getTile());
    }

    auto theme = SkinTheme::get(m_indicators);

    // Eyedropper icon
    add(theme->getToolPart("eyedropper"), false);

    // Color
    m_indicators->addColorIndicator(color);

    // Color description
    std::string str = color.toHumanReadableString(
      app_get_current_pixel_format(),
      app::Color::LongHumanReadableString);
    if (color.getAlpha() < 255)
      str += fmt::format(" A{}", color.getAlpha());
    m_indicators->addTextIndicator(str.c_str());

    return *this;
  }

  IndicatorsGeneration& add(doc::tile_t tile) {
    auto theme = SkinTheme::get(m_indicators);

    // Eyedropper icon
    add(theme->getToolPart("eyedropper"), false);

    // Color
    m_indicators->addTileIndicator(tile);

    // Color description
    std::string str;
    if (tile == doc::notile) {
      str += "Empty";
    }
    else {
      // TODO could the site came from the Indicators or StatusBar itself
      int baseIndex = 1;
      Site site = UIContext::instance()->activeSite();
      if (site.tileset())
        baseIndex = site.tileset()->baseIndex();

      doc::tile_index ti = doc::tile_geti(tile);
      doc::tile_flags tf = doc::tile_getf(tile);
      if (baseIndex < 0)
        str += fmt::format("{}", ((int)ti) + baseIndex - 1);
      else
        str += fmt::format("{}", ti + baseIndex - 1);
      if (tf) {
        str += " Flip ";
        build_tile_flags_string(tf, str);
      }
    }
    m_indicators->addTextIndicator(str.c_str());

    return *this;
  }

  IndicatorsGeneration& add(tools::Tool* tool) {
    auto theme = SkinTheme::get(m_indicators);

    // Tool icon + text
    add(theme->getToolPart(tool->getId().c_str()), false);
    m_indicators->addTextIndicator(tool->getText().c_str());

    // Tool shortcut
    KeyPtr key = KeyboardShortcuts::instance()->tool(tool);
    if (key && !key->accels().empty()) {
      add(theme->parts.iconKey(), true);
      m_indicators->addTextIndicator(key->accels().front().toString().c_str());
    }
    return *this;
  }

private:
  StatusBar::Indicators* m_indicators;
};

class StatusBar::CustomizedTipWindow : public ui::TipWindow {
public:
  CustomizedTipWindow(const std::string& text)
    : ui::TipWindow(text) {
  }

  void setInterval(int msecs) {
    if (!m_timer)
      m_timer.reset(new ui::Timer(msecs, this));
    else
      m_timer->setInterval(msecs);
  }

  void startTimer() {
    m_timer->start();
  }

protected:
  bool onProcessMessage(Message* msg) override {
    switch (msg->type()) {
      case kTimerMessage:
        closeWindow(nullptr);
        m_timer->stop();
        break;
    }
    return ui::TipWindow::onProcessMessage(msg);
  }

private:
  std::unique_ptr<ui::Timer> m_timer;
};

// TODO Use a ui::TipWindow with rounded borders, when we add support
//      to invalidate transparent windows.
class StatusBar::SnapToGridWindow : public ui::PopupWindow {
public:
  SnapToGridWindow()
    : ui::PopupWindow("", ClickBehavior::DoNothingOnClick)
    , m_button(Strings::statusbar_tips_disable_snap_grid()) {
    InitTheme.connect(
      [this]{
        setBorder(gfx::Border(2 * guiscale()));
        setBgColor(gfx::rgba(255, 255, 200));
      });
    initTheme();
    makeFloating();

    addChild(&m_button);
    m_button.Click.connect([this]{ onDisableSnapToGrid(); });
  }

  void setDocument(Doc* doc) {
    m_doc = doc;
  }

private:
  void onDisableSnapToGrid() {
    Preferences::instance().document(m_doc).grid.snap(false);
    closeWindow(nullptr);
  }

  Doc* m_doc;
  ui::Button m_button;
};

// This widget is used to show the current frame.
class GotoFrameEntry : public Entry {
public:
  GotoFrameEntry() : Entry(4, "") {
  }

  bool onProcessMessage(Message* msg) override {
    switch (msg->type()) {

      // When the mouse enter in this entry, it got the focus and the
      // text is automatically selected.
      case kMouseEnterMessage:
        if (Preferences::instance().statusBar.focusFrameFieldOnMouseover()) {
          requestFocus();
          selectAllText();
        }
        break;

      case kKeyDownMessage: {
        KeyMessage* keymsg = static_cast<KeyMessage*>(msg);
        KeyScancode scancode = keymsg->scancode();

        if (hasFocus() &&
            (scancode == kKeyEnter || // TODO customizable keys
             scancode == kKeyEnterPad)) {
          Command* cmd = Commands::instance()->byId(CommandId::GotoFrame());
          Params params;
          params.set("frame", text().c_str());
          UIContext::instance()->executeCommandFromMenuOrShortcut(cmd, params);

          // Select the text again
          selectAllText();
          releaseFocus();
          return true;          // Key used.
        }
        break;
      }
    }

    bool result = Entry::onProcessMessage(msg);

    if (msg->type() == kMouseDownMessage)
      selectText(0, -1);

    return result;
  }

};

StatusBar* StatusBar::m_instance = NULL;

StatusBar::StatusBar(TooltipManager* tooltipManager)
  : m_timeout(0)
  , m_about(new AboutStatusBar)
  , m_indicators(new Indicators)
  , m_docControls(new HBox)
  , m_tipwindow(nullptr)
  , m_snapToGridWindow(nullptr)
{
  m_instance = this;

  setDoubleBuffered(true);
  setFocusStop(true);

  m_about->setExpansive(true);
  m_about->setVisible(true);
  m_indicators->setExpansive(true);
  m_indicators->setVisible(false);
  m_docControls->setVisible(false);
  addChild(m_about);
  addChild(m_indicators);
  addChild(m_docControls);

  // Construct the commands box
  {
    Box* box1 = new Box(HORIZONTAL);
    Box* box4 = new Box(HORIZONTAL);

    m_frameLabel = new Label(Strings::statusbar_tips_frame());
    m_currentFrame = new GotoFrameEntry();
    m_newFrame = new Button("+");
    m_newFrame->Click.connect([this]{ newFrame(); });
    m_zoomEntry = new ZoomEntry;
    m_zoomEntry->ZoomChange.connect(&StatusBar::onChangeZoom, this);

    setup_mini_look(m_currentFrame);

    box4->addChild(m_currentFrame);
    box4->addChild(m_newFrame);

    box1->addChild(m_frameLabel);
    box1->addChild(box4);
    box1->addChild(m_zoomEntry);

    m_docControls->addChild(box1);
    m_commandsBox = box1;
  }

  // Tooltips
  tooltipManager->addTooltipFor(
    m_currentFrame, Strings::statusbar_tips_current_frame(), BOTTOM);
  tooltipManager->addTooltipFor(
    m_zoomEntry, Strings::statusbar_tips_zoom_level(), BOTTOM);
  tooltipManager->addTooltipFor(
    m_newFrame, Strings::statusbar_tips_new_frame(), BOTTOM);

  App::instance()->activeToolManager()->add_observer(this);

  initTheme();
}

StatusBar::~StatusBar()
{
  App::instance()->activeToolManager()->remove_observer(this);

  delete m_tipwindow;           // widget
  delete m_snapToGridWindow;
}

void StatusBar::onSelectedToolChange(tools::Tool* tool)
{
  if (isVisible() && tool)
    showTool(500, tool);
}

void StatusBar::clearText()
{
  showIndicators();
  setStatusText(1, std::string());
}

// TODO Workspace views should have a method to set the default status
//      bar text, because here the StatusBar is depending on too many
//      details of the main window/docs/etc.
void StatusBar::showDefaultText()
{
  auto mainWindow = (App::instance() ? App::instance()->mainWindow(): nullptr);
  if (mainWindow)
    mainWindow->showDefaultStatusBar();
  else
    clearText();
}

void StatusBar::showDefaultText(Doc* doc)
{
  clearText();
  if (doc) {
    std::string buf =
      fmt::format("{}  :size: {} {}",
                  doc->name(), doc->width(), doc->height());
    if (doc->getTransformation().bounds().w != 0) {
      buf += fmt::format(" :selsize: {} {}",
                         int(doc->getTransformation().bounds().w),
                         int(doc->getTransformation().bounds().h));
    }
    if (Preferences::instance().general.showFullPath()) {
      std::string path = base::get_file_path(doc->filename());
      if (!path.empty())
        buf += fmt::format("  ({})", path);
    }

    setStatusText(1, buf);
  }
}

void StatusBar::updateFromEditor(Editor* editor)
{
  if (editor) {
    showIndicators();
    m_zoomEntry->setZoom(editor->zoom());
  }
}

void StatusBar::showBackupIcon(BackupIcon icon)
{
  showIndicators();
  m_indicators->showBackupIcon(icon);
}

bool StatusBar::setStatusText(int msecs, const std::string& msg)
{
  if ((base::current_tick() > m_timeout) || (msecs > 0)) {
    showIndicators();
    IndicatorsGeneration(m_indicators).add(msg.c_str());
    m_timeout = base::current_tick() + msecs;
    return true;
  }
  else
    return false;
}

void StatusBar::showTip(int msecs, const std::string& msg)
{
  ASSERT(msecs > 0);

  if (m_tipwindow == NULL) {
    m_tipwindow = new CustomizedTipWindow(msg);
  }
  else {
    m_tipwindow->setText(msg);
  }

  m_tipwindow->setInterval(msecs);

  if (m_tipwindow->isVisible())
    m_tipwindow->closeWindow(nullptr);

  m_tipwindow->remapWindow();

  gfx::Rect rc = m_tipwindow->bounds();
  rc.x = bounds().x2() - rc.w;
  rc.y = bounds().y - rc.h;
  ui::fit_bounds(display(), m_tipwindow, rc);

  m_tipwindow->openWindow();
  m_tipwindow->startTimer();

  // Set the text in status-bar (with inmediate timeout)
  IndicatorsGeneration(m_indicators).add(msg.c_str());
  m_timeout = base::current_tick();
}

void StatusBar::showColor(int msecs, const app::Color& color,
                          const std::string& text)
{
  if ((base::current_tick() > m_timeout) || (msecs > 0)) {
    showIndicators();
    IndicatorsGeneration gen(m_indicators);
    gen.add(color);
    if (!text.empty())
      gen.add(text.c_str());

    m_timeout = base::current_tick() + msecs;
  }
}

void StatusBar::showTile(int msecs, doc::tile_t tile,
                         const std::string& text)
{
  if ((base::current_tick() > m_timeout) || (msecs > 0)) {
    IndicatorsGeneration gen(m_indicators);
    gen.add(tile);
    if (!text.empty())
      gen.add(text.c_str());

    m_timeout = base::current_tick() + msecs;
  }
}

void StatusBar::showTool(int msecs, tools::Tool* tool)
{
  showIndicators();

  ASSERT(tool != nullptr);
  IndicatorsGeneration(m_indicators).add(tool);

  m_timeout = base::current_tick() + msecs;
}

void StatusBar::showSnapToGridWarning(bool state)
{
  if (state) {
    // this->doc() can be nullptr if "snap to grid" command is pressed
    // without an opened document. (E.g. to change the default
    // setting)
    if (!doc())
      return;

    if (!m_snapToGridWindow)
      m_snapToGridWindow = new SnapToGridWindow;

    m_snapToGridWindow->setDisplay(display(), false);

    if (!m_snapToGridWindow->isVisible()) {
      m_snapToGridWindow->openWindow();
      m_snapToGridWindow->remapWindow();
      updateSnapToGridWindowPosition();
    }

    m_snapToGridWindow->setDocument(doc());
  }
  else {
    if (m_snapToGridWindow)
      m_snapToGridWindow->closeWindow(nullptr);
  }
}

//////////////////////////////////////////////////////////////////////
// StatusBar message handler

void StatusBar::onInitTheme(ui::InitThemeEvent& ev)
{
  HBox::onInitTheme(ev);

  auto theme = SkinTheme::get(this);
  setBgColor(theme->colors.statusBarFace());
  setBorder(gfx::Border(6*guiscale(), 0, 6*guiscale(), 0));
  setMinMaxSize(
    Size(0, textHeight()+8*guiscale()),
    Size(std::numeric_limits<int>::max(),
         textHeight()+8*guiscale()));

  m_newFrame->setStyle(theme->styles.newFrameButton());
  m_commandsBox->setBorder(gfx::Border(2, 1, 2, 2)*guiscale());

  if (m_snapToGridWindow) {
    m_snapToGridWindow->initTheme();
    if (m_snapToGridWindow->isVisible())
      updateSnapToGridWindowPosition();
  }
}

void StatusBar::onResize(ResizeEvent& ev)
{
  Rect rc = ev.bounds();
  m_docControls->setVisible(doc() && rc.w > 300*ui::guiscale());

  HBox::onResize(ev);

  if (m_snapToGridWindow &&
      m_snapToGridWindow->isVisible())
    updateSnapToGridWindowPosition();
}

void StatusBar::onActiveSiteChange(const Site& site)
{
  DocObserverWidget<ui::HBox>::onActiveSiteChange(site);

  const bool controlsWereVisible = m_docControls->isVisible();

  if (doc()) {
    auto& docPref = Preferences::instance().document(doc());

    m_docControls->setVisible(true);
    showSnapToGridWarning(docPref.grid.snap());

    // Current frame
    {
      std::string newText =
        fmt::format("{}", site.frame()+docPref.timeline.firstFrame());
      if (m_currentFrame->text() != newText)
        m_currentFrame->setText(newText);
    }

    // Zoom level
    if (auto editor = Editor::activeEditor())
      updateFromEditor(editor);
  }
  else {
    m_docControls->setVisible(false);
    showSnapToGridWarning(false);
  }

  // Relayout the StatusBar so we can put the m_docControls widget in
  // the right place now that it's visibility changed.
  if (m_docControls->isVisible() != controlsWereVisible)
    layout();
}

void StatusBar::onPixelFormatChanged(DocEvent& ev)
{
  // If this is called from the non-UI thread it means that the pixel
  // format change was made in the background,
  // i.e. ChangePixelFormatCommand uses a background thread to change
  // the sprite format.
  if (!ui::is_ui_thread())
    return;

  onActiveSiteChange(UIContext::instance()->activeSite());
}

void StatusBar::newFrame()
{
  Command* cmd = Commands::instance()->byId(CommandId::NewFrame());
  UIContext::instance()->executeCommandFromMenuOrShortcut(cmd);
}

void StatusBar::onChangeZoom(const render::Zoom& zoom)
{
  if (auto editor = Editor::activeEditor())
    editor->setEditorZoom(zoom);
}

void StatusBar::updateSnapToGridWindowPosition()
{
  ASSERT(m_snapToGridWindow);

  Rect rc = bounds();
  int toolBarWidth = ToolBar::instance()->sizeHint().w;

  gfx::Rect snapRc = m_snapToGridWindow->bounds();
  snapRc.x = rc.x+rc.w-toolBarWidth-snapRc.w;
  snapRc.y = rc.y-snapRc.h;
  m_snapToGridWindow->setBounds(snapRc);
}

void StatusBar::showAbout()
{
  if (!m_about->isVisible()) {
    m_indicators->setVisible(false);
    m_about->setVisible(true);
    m_about->layout();
  }
}

void StatusBar::showIndicators()
{
  if (!m_indicators->isVisible()) {
    m_about->setVisible(false);
    m_indicators->setVisible(true);
    m_indicators->layout();
  }
}

} // namespace app
