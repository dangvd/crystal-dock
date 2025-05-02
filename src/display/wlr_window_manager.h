/*
 * This file is part of Crystal Dock.
 * Copyright (C) 2025 Viet Dang (dangvd@gmail.com)
 *
 * Crystal Dock is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Crystal Dock is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Crystal Dock.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef WLR_WINDOW_MANAGER_H_
#define WLR_WINDOW_MANAGER_H_

#include <memory>
#include <string>

#include "window_system.h"
#include "wlr_foreign_toplevel_management.h"

namespace crystaldock {

class WlrWindowManager : public QObject {
  Q_OBJECT

 private:
  WlrWindowManager() = default;

 signals:
  void windowAdded(const WindowInfo*);
  void windowRemoved(void*);
  void windowLeftCurrentDesktop(void*);
  void windowGeometryChanged(const WindowInfo*);
  void windowStateChanged(const WindowInfo*);
  void windowTitleChanged(const WindowInfo*);
  void activeWindowChanged(void*);

 public:
  static WlrWindowManager* self();
  static void init(struct zwlr_foreign_toplevel_manager_v1* window_manager);

  static void bindWindowManagerFunctions(WindowManager* windowManager);

  static std::vector<const WindowInfo*> windows();
  static void* activeWindow();
  // We manually reset active window, usually when the new active window is the dock itself.
  // We don't want to always do this (e.g. handle this in state_change() handler) because
  // otherwise we wouldn't be able to click on an active window's icon to minimize it
  // (the click action would change the active window to be the dock).
  static void resetActiveWindow();
  static void activateWindow(void* window);
  static void activateOrMinimizeWindow(void* window);
  static void closeWindow(void* window);
  static bool showingDesktop();
  static void setShowingDesktop(bool show);

 private:

  // zwlr_foreign_toplevel_manager_v1 interface.

  static void toplevel(
      void *data,
      struct zwlr_foreign_toplevel_manager_v1 *zwlr_foreign_toplevel_manager_v1,
      struct zwlr_foreign_toplevel_handle_v1 *window);

  static void finished(
      void *data,
      struct zwlr_foreign_toplevel_manager_v1 *zwlr_foreign_toplevel_manager_v1);

  // zwlr_foreign_toplevel_handle_v1 interface.

  static void title(
      void *data,
      struct zwlr_foreign_toplevel_handle_v1 *window,
      const char *title);

  static void app_id(
      void *data,
      struct zwlr_foreign_toplevel_handle_v1 *window,
      const char *app_id);

  static void output_enter(
      void *data,
      struct zwlr_foreign_toplevel_handle_v1 *window,
      struct wl_output *output);

  static void output_leave(
      void *data,
      struct zwlr_foreign_toplevel_handle_v1 *window,
      struct wl_output *output);

  static void state(
      void *data,
      struct zwlr_foreign_toplevel_handle_v1 *window,
      struct wl_array *state);

  static void done(
      void *data,
      struct zwlr_foreign_toplevel_handle_v1 *window);

  static void closed(
      void *data,
      struct zwlr_foreign_toplevel_handle_v1 *window);

  static void parent(
      void *data,
      struct zwlr_foreign_toplevel_handle_v1 *window,
      struct zwlr_foreign_toplevel_handle_v1 *parent);

  static constexpr struct zwlr_foreign_toplevel_manager_v1_listener
      window_manager_listener_ = {
          toplevel,
          finished
  };
  static constexpr struct zwlr_foreign_toplevel_handle_v1_listener window_listener_ = {
      title,
      app_id,
      output_enter,
      output_leave,
      state,
      done,
      closed,
      parent,
  };

  static zwlr_foreign_toplevel_manager_v1* window_manager_;

  static std::unordered_map<struct zwlr_foreign_toplevel_handle_v1*, std::unique_ptr<WindowInfo>>
      windows_;
  static struct zwlr_foreign_toplevel_handle_v1* activeWindow_;
  static bool showingDesktop_;
};

}

#endif // WLR_WINDOW_MANAGER_H_
