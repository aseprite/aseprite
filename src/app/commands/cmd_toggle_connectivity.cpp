#include "app/commands/command.h"
#include "app/context.h"
#include "app/pref/preferences.h"
#include "app/tools/tool.h"
#include "app/app.h"

namespace app {

class ToggleConnectivityCommand : public Command {
public:
  ToggleConnectivityCommand()
    : Command("ToggleConnectivity") {}

protected:
  bool onEnabled(Context* /*context*/) override {
    auto* tool = App::instance()->activeTool();
    return (tool && (tool->getId() == "paint_bucket" ||
                     tool->getId() == "magic_wand"));
  }

  void onExecute(Context* /*context*/) override {
    auto* tool = App::instance()->activeTool();
    if (!tool) return;

    auto& toolPref = Preferences::instance().tool(tool);
    using namespace app::gen;

    if (toolPref.floodfill.pixelConnectivity() == PixelConnectivity::FOUR_CONNECTED) {
      toolPref.floodfill.pixelConnectivity(PixelConnectivity::EIGHT_CONNECTED);
    }
    else {
      toolPref.floodfill.pixelConnectivity(PixelConnectivity::FOUR_CONNECTED);
    }
  }
};

Command* CommandFactory::createToggleConnectivityCommand() {
  return new ToggleConnectivityCommand;
}

} // namespace app
