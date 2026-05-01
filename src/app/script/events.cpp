// Aseprite
// Copyright (C) 2021-2024  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/context.h"
#include "app/context_observer.h"
#include "app/doc.h"
#include "app/doc_event.h"
#include "app/doc_undo.h"
#include "app/doc_undo_observer.h"
#include "app/pref/preferences.h"
#include "app/script/docobj.h"
#include "app/script/engine.h"
#include "app/script/events.h"
#include "app/script/luacpp.h"
#include "app/script/values.h"
#include "app/site.h"
#include "app/ui/main_window.h"
#include "doc/sprite.h"
#include "ui/resize_event.h"

#include <any>
#include <initializer_list>
#include <map>

namespace app { namespace script {

using namespace doc;

namespace {
// Used in BeforeCommand
bool g_stopPropagationFlag = false;
} // namespace

bool Events::hasListener(EventListener callbackRef) const
{
  for (const auto& listeners : m_listeners) {
    for (const EventListener listener : listeners) {
      if (listener == callbackRef)
        return true;
    }
  }
  return false;
}

void Events::add(const EventType eventType, const EventListener callbackRef)
{
  if (eventType >= m_listeners.size())
    m_listeners.resize(eventType + 1);

  auto& listeners = m_listeners[eventType];
  listeners.push_back(callbackRef);
  if (listeners.size() == 1)
    onAddFirstListener(eventType);
}

void Events::remove(const EventListener callbackRef)
{
  for (int i = 0; i < int(m_listeners.size()); ++i) {
    EventListeners& listeners = m_listeners[i];
    auto it = listeners.begin();
    auto end = listeners.end();
    bool removed = false;
    for (; it != end;) {
      if (*it == callbackRef) {
        removed = true;
        it = listeners.erase(it);
        end = listeners.end();
      }
      else
        ++it;
    }
    if (removed && listeners.empty())
      onRemoveLastListener(i);
  }
}

void Events::call(const EventType eventType,
                  const std::initializer_list<std::pair<const std::string, std::any>>& args) const
{
  if (eventType >= m_listeners.size())
    return;

  auto* engine = get_engine(L);
  try {
    for (const EventListener callbackRef : m_listeners[eventType]) {
      // Get user-defined callback function
      lua_rawgeti(L, LUA_REGISTRYINDEX, callbackRef);

      int callbackArgs = 0;
      if (args.size() > 0) {
        ++callbackArgs;
        lua_newtable(L); // Create "ev" argument with fields about the event
        for (const auto& kv : args) {
          push_value_to_lua(L, kv.second);
          lua_setfield(L, -2, kv.first.c_str());
        }
      }

      if (lua_pcall(L, callbackArgs, 0, 0)) {
        if (const char* s = lua_tostring(L, -1))
          engine->ConsolePrint(s);
      }
    }
  }
  catch (const std::exception& ex) {
    engine->ConsolePrint(ex.what());
  }
}

Events::EventType AppEvents::eventType(const char* eventName) const
{
  if (std::strcmp(eventName, "sitechange") == 0)
    return SiteChange;
  if (std::strcmp(eventName, "beforesitechange") == 0)
    return BeforeSiteChange;
  if (std::strcmp(eventName, "fgcolorchange") == 0)
    return FgColorChange;
  if (std::strcmp(eventName, "bgcolorchange") == 0)
    return BgColorChange;
  if (std::strcmp(eventName, "beforecommand") == 0)
    return BeforeCommand;
  if (std::strcmp(eventName, "aftercommand") == 0)
    return AfterCommand;
  return Unknown;
}

bool Events::empty() const
{
  if (m_listeners.empty())
    return true;

  for (const auto& listener : m_listeners)
    if (!listener.empty())
      return false;

  return true;
}

void AppEvents::onAddFirstListener(EventType eventType)
{
  auto* app = App::instance();
  auto ctx = app->context();
  auto& pref = Preferences::instance();
  switch (eventType) {
    case BeforeSiteChange: [[fallthrough]];
    case SiteChange:       {
      if (m_addedObserver == 0)
        ctx->add_observer(this);

      ++m_addedObserver;
    } break;
    case FgColorChange:
      m_fgConn = pref.colorBar.fgColor.AfterChange.connect([this] { onFgColorChange(); });
      break;
    case BgColorChange:
      m_bgConn = pref.colorBar.bgColor.AfterChange.connect([this] { onBgColorChange(); });
      break;
    case BeforeCommand:
      m_beforeCmdConn = ctx->BeforeCommandExecution.connect(&AppEvents::onBeforeCommand, this);
      break;
    case AfterCommand:
      m_afterCmdConn = ctx->AfterCommandExecution.connect(&AppEvents::onAfterCommand, this);
      break;
  }
}

void AppEvents::onRemoveLastListener(EventType eventType)
{
  switch (eventType) {
    case BeforeSiteChange: [[fallthrough]];
    case SiteChange:
      --m_addedObserver;

      if (m_addedObserver == 0)
        App::instance()->context()->remove_observer(this);
      break;
    case FgColorChange: m_fgConn.disconnect(); break;
    case BgColorChange: m_bgConn.disconnect(); break;
    case BeforeCommand: m_beforeCmdConn.disconnect(); break;
    case AfterCommand:  m_afterCmdConn.disconnect(); break;
  }
}

void AppEvents::onFgColorChange()
{
  call(FgColorChange);
}

void AppEvents::onBgColorChange()
{
  call(BgColorChange);
}

void AppEvents::onBeforeCommand(CommandExecutionEvent& ev)
{
  g_stopPropagationFlag = false;

  auto stopPropagation = (lua_CFunction)[](lua_State*)->int
  {
    g_stopPropagationFlag = true;
    return 0;
  };

  call(BeforeCommand,
       {
         { "name",            ev.command()->id() },
         { "params",          ev.params()        },
         { "stopPropagation", stopPropagation    }
  });
  if (g_stopPropagationFlag)
    ev.cancel();
}

void AppEvents::onAfterCommand(CommandExecutionEvent& ev)
{
  call(AfterCommand,
       {
         { "name",   ev.command()->id() },
         { "params", ev.params()        }
  });
}

void AppEvents::onActiveSiteChange(const Site& site)
{
  if (m_lastActiveSite.has_value() && *m_lastActiveSite == site) {
    // Avoid multiple events that can happen when closing since
    // we're changing views at the same time we're removing
    // documents
    return;
  }

  const bool fromUndo = (site.document() && site.document()->isUndoing());
  call(SiteChange,
       {
         { "fromUndo", fromUndo }
  });
  m_lastBeforeActiveSite = std::nullopt;
  m_lastActiveSite = site;
}

void AppEvents::onBeforeActiveSiteChange(const Site& fromSite)
{
  if (m_lastBeforeActiveSite.has_value() && *m_lastBeforeActiveSite == fromSite)
    return;

  const bool fromUndo = (fromSite.document() && fromSite.document()->isUndoing());
  call(BeforeSiteChange,
       {
         { "fromUndo", fromUndo }
  });
  m_lastBeforeActiveSite = fromSite;
}

Events::EventType WindowEvents::eventType(const char* eventName) const
{
  if (std::strcmp(eventName, "resize") == 0)
    return Resize;

  return Unknown;
}

void WindowEvents::onAddFirstListener(EventType eventType)
{
  switch (eventType) {
    case Resize: m_resizeConn = m_window->Resize.connect(&WindowEvents::onResize, this); break;
  }
}

void WindowEvents::onRemoveLastListener(EventType eventType)
{
  switch (eventType) {
    case Resize: m_resizeConn.disconnect(); break;
  }
}

void WindowEvents::onResize(ui::ResizeEvent& ev)
{
  call(Resize,
       {
         { "width",  ev.bounds().w },
         { "height", ev.bounds().h }
  });
}

SpriteEvents::SpriteEvents(lua_State* L, const Sprite* sprite) : Events(L), m_spriteId(sprite->id())
{
  doc()->add_observer(this);
}

SpriteEvents::~SpriteEvents()
{
  auto doc = this->doc();
  // The document can be nullptr in some cases like:
  // - When closing the App with an exception
  //   (ui::get_app_state() == ui::AppState::kClosingWithException)
  // - When Sprite.events property was accessed in a app
  //   "sitechange" event just when this same sprite was closed
  //   (so the SpriteEvents is created/destroyed for second time)
  if (doc) {
    disconnectFromUndoHistory(doc);
    doc->remove_observer(this);
  }
}

Events::EventType SpriteEvents::eventType(const char* eventName) const
{
  if (std::strcmp(eventName, "change") == 0)
    return Change;
  else if (std::strcmp(eventName, "filenamechange") == 0)
    return FilenameChange;
  else if (std::strcmp(eventName, "afteraddtile") == 0)
    return AfterAddTile;
  else if (std::strcmp(eventName, "layerblendmode") == 0)
    return LayerBlendMode;
  else if (std::strcmp(eventName, "layername") == 0)
    return LayerName;
  else if (std::strcmp(eventName, "layeropacity") == 0)
    return LayerOpacity;
  else if (std::strcmp(eventName, "layervisibility") == 0)
    return LayerVisibility;
  else
    return Unknown;
}

void SpriteEvents::onCloseDocument(Doc* doc)
{
  SpriteClosed();
}

void SpriteEvents::onFileNameChanged(Doc* doc)
{
  call(FilenameChange);
}

void SpriteEvents::onAfterAddTile(DocEvent& ev)
{
  call(AfterAddTile,
       {
         { "sprite",      ev.sprite()    },
         { "layer",       ev.layer()     },
         // This is detected as a "int" type
         { "frameNumber", ev.frame() + 1 },
         { "tileset",     ev.tileset()   },
         { "tileIndex",   ev.tileIndex() }
  });
}

void SpriteEvents::onAddUndoState(DocUndo* doc_undo)
{
  call(Change,
       {
         { "fromUndo", false }
  });
}

void SpriteEvents::onCurrentUndoStateChange(DocUndo* doc_undo)
{
  call(Change,
       {
         { "fromUndo", true }
  });
}

void SpriteEvents::onLayerBlendModeChange(DocEvent& ev)
{
  call(LayerBlendMode,
       {
         { "layer", ev.layer() }
  });
}

void SpriteEvents::onLayerNameChange(DocEvent& ev)
{
  call(LayerName,
       {
         { "layer", ev.layer() }
  });
}

void SpriteEvents::onLayerOpacityChange(DocEvent& ev)
{
  call(LayerOpacity,
       {
         { "layer", ev.layer() }
  });
}

void SpriteEvents::onAfterLayerVisibilityChange(DocEvent& ev)
{
  call(LayerVisibility,
       {
         { "layer", ev.layer() }
  });
}

void SpriteEvents::onAddFirstListener(EventType eventType)
{
  switch (eventType) {
    case Change:
      ASSERT(!m_observingUndo);
      doc()->undoHistory()->add_observer(this);
      m_observingUndo = true;
      break;
  }
}

void SpriteEvents::onRemoveLastListener(EventType eventType)
{
  switch (eventType) {
    case Change: {
      disconnectFromUndoHistory(doc());
      break;
    }
  }
}

Doc* SpriteEvents::doc() const
{
  const Sprite* sprite = doc::get<Sprite>(m_spriteId);
  if (sprite)
    return static_cast<Doc*>(sprite->document());

  return nullptr;
}

void SpriteEvents::disconnectFromUndoHistory(Doc* doc)
{
  if (m_observingUndo) {
    doc->undoHistory()->remove_observer(this);
    m_observingUndo = false;
  }
}

int Events_on(lua_State* L)
{
  auto* evs = get_ptr<Events>(L, 1);
  const char* eventName = lua_tostring(L, 2);
  if (!eventName)
    return 0;

  const int type = evs->eventType(eventName);
  if (type < 0)
    return luaL_error(L, "invalid event name to listen");

  if (!lua_isfunction(L, 3))
    return luaL_error(L, "second argument must be a function");

  // Copy the callback function to add it to the global registry
  lua_pushvalue(L, 3);
  int callbackRef = luaL_ref(L, LUA_REGISTRYINDEX);
  evs->add(type, callbackRef);

  // Return the callback ref (this is an EventListener easier to use
  // in Events_off())
  lua_pushinteger(L, callbackRef);
  return 1;
}

int Events_off(lua_State* L)
{
  auto* evs = get_ptr<Events>(L, 1);
  int callbackRef = LUA_REFNIL;

  // Remove by listener value
  if (lua_isinteger(L, 2)) {
    callbackRef = lua_tointeger(L, 2);
  }
  // Remove by function reference
  else if (lua_isfunction(L, 2)) {
    lua_pushnil(L);
    while (lua_next(L, LUA_REGISTRYINDEX) != 0) {
      if (lua_isnumber(L, -2) && lua_isfunction(L, -1)) {
        int i = lua_tointeger(L, -2);
        if ( // Compare value=function in 2nd argument
          lua_compare(L, -1, 2, LUA_OPEQ) &&
          // Check that this Events contain this reference
          evs->hasListener(i)) {
          callbackRef = i;
          lua_pop(L, 2); // Pop value and key
          break;
        }
      }
      lua_pop(L, 1); // Pop the value, leave the key for next lua_next()
    }
  }
  else {
    return luaL_error(L, "first argument must be a function or a EventListener");
  }

  if (callbackRef != LUA_REFNIL &&
      // Check that we are removing a listener from this Events and no
      // other random value from the Lua registry
      evs->hasListener(callbackRef)) {
    evs->remove(callbackRef);
    luaL_unref(L, LUA_REGISTRYINDEX, callbackRef);
  }
  return 0;
}

// We don't need a __gc (to call ~Events()), because Events instances
// will be deleted when the Sprite is deleted or on App Exit
const luaL_Reg Events_methods[] = {
  { "on",    Events_on  },
  { "off",   Events_off },
  { nullptr, nullptr    }
};

DEF_MTNAME(Events);

void register_events_class(lua_State* L)
{
  REG_CLASS(L, Events);
}

void push_app_events(lua_State* L)
{
  push_ptr<Events>(L, get_engine(L)->appEvents());
}

void push_sprite_events(lua_State* L, Sprite* sprite)
{
  ASSERT(sprite);
  push_ptr<Events>(L, get_engine(L)->spriteEvents(sprite));
}

void push_window_events(lua_State* L, ui::Window* window)
{
  push_ptr<Events>(L, get_engine(L)->windowEvents(window));
}

}} // namespace app::script
