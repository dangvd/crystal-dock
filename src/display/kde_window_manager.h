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

#ifndef KDE_WINDOW_MANAGER_H_
#define KDE_WINDOW_MANAGER_H_

#include <memory>
#include <string>

#include "plasma_window_management.h"
#include "window_system.h"

namespace crystaldock {

class KdeWindowManager : public QObject {
  Q_OBJECT

 private:
  KdeWindowManager() = default;

 signals:
  void windowAdded(const WindowInfo*);
  void windowRemoved(void*);
  void windowLeftCurrentDesktop(void*);
  void windowGeometryChanged(const WindowInfo*);
  void windowStateChanged(const WindowInfo*);
  void windowTitleChanged(const WindowInfo*);
  void activeWindowChanged(void*);
  void windowLeftCurrentActivity(void*);

 public:
  static KdeWindowManager* self();
  static void init(struct org_kde_plasma_window_management* window_management);

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
  static void minimizeWindow(void* window);
  static void closeWindow(void* window);
  static bool showingDesktop();
  static void setShowingDesktop(bool show);

 private:

  // org_kde_plasma_window_management interface.

  static void show_desktop_changed(
      void *data,
      struct org_kde_plasma_window_management *org_kde_plasma_window_management,
      uint32_t state);

  static void window(
      void *data,
      struct org_kde_plasma_window_management *org_kde_plasma_window_management,
      uint32_t id);

  static void stacking_order_changed(
      void *data,
      struct org_kde_plasma_window_management *org_kde_plasma_window_management,
      struct wl_array *ids);

  static void stacking_order_uuid_changed(
      void *data,
      struct org_kde_plasma_window_management *org_kde_plasma_window_management,
      const char *uuids);

  static void window_with_uuid(
      void *data,
      struct org_kde_plasma_window_management *org_kde_plasma_window_management,
      uint32_t id,
      const char *uuid);

  // org_kde_plasma_window interface.

  static void title_changed(
      void *data,
      struct org_kde_plasma_window *org_kde_plasma_window,
      const char *title);

  static void app_id_changed(
      void *data,
      struct org_kde_plasma_window *org_kde_plasma_window,
      const char *app_id);

  static void state_changed(
      void *data,
      struct org_kde_plasma_window *org_kde_plasma_window,
      uint32_t flags);

  static void virtual_desktop_changed(
      void *data,
      struct org_kde_plasma_window *org_kde_plasma_window,
      int32_t number);

  static void themed_icon_name_changed(
      void *data,
      struct org_kde_plasma_window *org_kde_plasma_window,
      const char *name);

  static void unmapped(
      void *data,
      struct org_kde_plasma_window *org_kde_plasma_window);

  static void initial_state(
      void *data, struct org_kde_plasma_window *org_kde_plasma_window);

  static void parent_window(
      void *data,
      struct org_kde_plasma_window *org_kde_plasma_window,
      struct org_kde_plasma_window *parent);

  static void geometry(
      void *data,
      struct org_kde_plasma_window *org_kde_plasma_window,
      int32_t x,
      int32_t y,
      uint32_t width,
      uint32_t height);

  static void icon_changed(
      void *data,
      struct org_kde_plasma_window *org_kde_plasma_window);

  static void pid_changed(
      void *data,
      struct org_kde_plasma_window *org_kde_plasma_window,
      uint32_t pid);

  static void virtual_desktop_entered(
      void *data,
      struct org_kde_plasma_window *org_kde_plasma_window,
      const char *id);

  static void virtual_desktop_left(
      void *data,
      struct org_kde_plasma_window *org_kde_plasma_window,
      const char *is);

  static void application_menu(
      void *data,
      struct org_kde_plasma_window *org_kde_plasma_window,
      const char *service_name,
      const char *object_path);

  static void activity_entered(
      void *data,
      struct org_kde_plasma_window *org_kde_plasma_window,
      const char *id);

  static void activity_left(
      void *data,
      struct org_kde_plasma_window *org_kde_plasma_window,
      const char *id);

  static void resource_name_changed(
      void *data,
      struct org_kde_plasma_window *org_kde_plasma_window,
      const char *resource_name);

  static constexpr struct org_kde_plasma_window_management_listener
      window_management_listener_ = {
          show_desktop_changed,
          window,
          stacking_order_changed,
          stacking_order_uuid_changed,
          window_with_uuid
  };
  static constexpr struct org_kde_plasma_window_listener window_listener_ = {
      title_changed,
      app_id_changed,
      state_changed,
      virtual_desktop_changed,
      themed_icon_name_changed,
      unmapped,
      initial_state,
      parent_window,
      geometry,
      icon_changed,
      pid_changed,
      virtual_desktop_entered,
      virtual_desktop_left,
      application_menu,
      activity_entered,
      activity_left,
      resource_name_changed,
  };

  static org_kde_plasma_window_management* window_management_;

  static std::unordered_map<struct org_kde_plasma_window*, std::unique_ptr<WindowInfo>> windows_;
  static std::unordered_map<std::string, struct org_kde_plasma_window*> uuids_;
  static std::vector<std::string> stackingOrder_;
  static struct org_kde_plasma_window* activeWindow_;
  static bool showingDesktop_;
};

}

#endif // KDE_WINDOW_MANAGER_H_
