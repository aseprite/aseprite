// Aseprite
// Copyright (c) 2026-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_SCRIPT_EVENTS_H_INCLUDED
#define APP_SCRIPT_EVENTS_H_INCLUDED
#pragma once

#include "app/context.h"
#include "app/context_observer.h"
#include "app/doc.h"
#include "app/doc_undo_observer.h"
#include "app/site.h"
#include "ui/window.h"

#include <any>
#include <initializer_list>

namespace app { namespace script {

using EventListener = int;

class Events {
public:
  using EventType = int;

  explicit Events(lua_State* L) : L(L) {};
  virtual ~Events() = default;
  Events(const Events&) = delete;
  Events& operator=(const Events&) = delete;

  virtual EventType eventType(const char* eventName) const = 0;
  bool empty() const;

  bool hasListener(EventListener callbackRef) const;
  void add(EventType eventType, EventListener callbackRef);
  void remove(EventListener callbackRef);

protected:
  void call(EventType eventType,
            const std::initializer_list<std::pair<const std::string, std::any>>& args = {}) const;

private:
  virtual void onAddFirstListener(EventType eventType) = 0;
  virtual void onRemoveLastListener(EventType eventType) = 0;

  using EventListeners = std::vector<EventListener>;
  std::vector<EventListeners> m_listeners;
  lua_State* L;
};

class AppEvents : public Events,
                  private ContextObserver {
public:
  enum : EventType {
    Unknown = -1,
    BeforeSiteChange,
    SiteChange,
    FgColorChange,
    BgColorChange,
    BeforeCommand,
    AfterCommand,
  };

  explicit AppEvents(lua_State* L);
  ~AppEvents();

  EventType eventType(const char* eventName) const override;

private:
  void onAddFirstListener(EventType eventType) override;
  void onRemoveLastListener(EventType eventType) override;
  void onFgColorChange();
  void onBgColorChange();
  void onBeforeCommand(CommandExecutionEvent& ev);
  void onAfterCommand(CommandExecutionEvent& ev);

  // ContextObserver impl
  void onActiveSiteChange(const Site& site) override;
  void onBeforeActiveSiteChange(const Site& fromSite) override;

  obs::scoped_connection m_fgConn;
  obs::scoped_connection m_bgConn;
  obs::scoped_connection m_beforeCmdConn;
  obs::scoped_connection m_afterCmdConn;

  int m_addedObserver;
  std::optional<Site> m_lastActiveSite;
  std::optional<Site> m_lastBeforeActiveSite;
}; // namespace app

class WindowEvents : public Events,
                     private ContextObserver {
public:
  enum : EventType {
    Unknown = -1,
    Resize,
  };

  explicit WindowEvents(lua_State* L, ui::Window* window) : Events(L), m_window(window) {}

  ui::Window* window() const { return m_window; }

  EventType eventType(const char* eventName) const override;

private:
  void onAddFirstListener(EventType eventType) override;
  void onRemoveLastListener(EventType eventType) override;

  void onResize(ui::ResizeEvent& ev);

  ui::Window* m_window;
  obs::scoped_connection m_resizeConn;
};

class SpriteEvents : public Events,
                     public DocUndoObserver,
                     public DocObserver {
public:
  enum : EventType {
    Unknown = -1,
    Change,
    FilenameChange,
    AfterAddTile,
    LayerBlendMode,
    LayerName,
    LayerOpacity,
    LayerVisibility,
  };

  explicit SpriteEvents(lua_State* L, const Sprite* sprite);
  ~SpriteEvents() override;

  EventType eventType(const char* eventName) const override;

  // DocObserver impl
  void onCloseDocument(Doc* doc) override;
  void onFileNameChanged(Doc* doc) override;
  void onAfterAddTile(DocEvent& ev) override;

  // DocUndoObserver impl
  void onAddUndoState(DocUndo*) override;
  void onCurrentUndoStateChange(DocUndo*) override;
  void onLayerBlendModeChange(DocEvent& ev) override;
  void onLayerNameChange(DocEvent& ev) override;
  void onLayerOpacityChange(DocEvent& ev) override;
  void onAfterLayerVisibilityChange(DocEvent& ev) override;

  obs::signal<void()> SpriteClosed;

private:
  void onAddFirstListener(EventType eventType) override;
  void onRemoveLastListener(EventType eventType) override;

  Doc* doc() const;
  void disconnectFromUndoHistory(Doc* doc);

  ObjectId m_spriteId;
  bool m_observingUndo = false;
};

}} // namespace app::script

#endif
