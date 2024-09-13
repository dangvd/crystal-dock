/*
 * This file is part of Crystal Dock.
 * Copyright (C) 2023 Viet Dang (dangvd@gmail.com)
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

#ifndef CRYSTALDOCK_WINDOW_SYSTEM_H_
#define CRYSTALDOCK_WINDOW_SYSTEM_H_

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

#include <wayland-client.h>

#include <QDBusInterface>
#include <QObject>
#include <QPixmap>
#include <QRect>
#include <QScreen>
#include <QString>
#include <QWidget>

#include "plasma_virtual_desktop.h"
#include "plasma_window_management.h"

#include <LayerShellQt/Window>

#include <model/application_menu_config.h>

namespace crystaldock {

struct VirtualDesktopInfo {
  std::string id;
  uint32_t number;  // 1-based.
  std::string name;
  struct org_kde_plasma_virtual_desktop* virtual_desktop;
};

struct WindowInfo {
  std::string uuid;
  std::string appId;
  std::string title;
  std::string desktop;
  std::string activity;
  bool skipTaskbar;
  bool onAllDesktops;
  bool demandsAttention;
  bool minimized;
  bool restoreAfterShowDesktop;
  int32_t x;
  int32_t y;
  uint32_t width;
  uint32_t height;
};

class WindowSystem : public QObject {
  Q_OBJECT

 private:
  WindowSystem() = default;

 signals:
  void currentDesktopChanged(std::string_view);
  void numberOfDesktopsChanged(int);

  void windowStateChanged(std::string_view);
  void windowAdded(const WindowInfo*);
  void windowRemoved(std::string);
  void windowLeftCurrentDesktop(std::string_view);
  void windowGeometryChanged(const WindowInfo*);

  void desktopNameChanged(std::string_view desktopId, std::string_view desktopName);

  void currentActivityChanged(std::string_view);
  void windowLeftCurrentActivity(std::string_view);

 private:
  // We need this to be non-static for the slot.
  std::string currentActivity_;

 public:
  static std::string_view currentActivity() { return WindowSystem::self()->currentActivity_; }
  void setCurrentActivity(std::string_view activity) { currentActivity_ = activity; }

 public slots:
  void onCurrentActivityChanged(QString activity) {
    currentActivity_ = activity.toStdString();
    emit currentActivityChanged(activity.toStdString());
  }

 public:
  static WindowSystem* self();
  static bool init(struct wl_display* display);

  static int numberOfDesktops();
  static std::vector<VirtualDesktopInfo> desktops() { return desktops_; }
  static std::string_view currentDesktop();
  static void setCurrentDesktop(std::string_view);
  static bool showingDesktop();
  static void setShowingDesktop(bool show);  

  static std::vector<const WindowInfo*> windows();
  static std::string_view activeWindow();
  // We manually reset active window, usually when the new active window is the dock itself.
  // We don't want to always do this (e.g. handle this in state_change() handler) because
  // otherwise we wouldn't be able to click on an active window's icon to minimize it
  // (the click action would change the active window to be the dock).
  static void resetActiveWindow();
  static void activateWindow(const std::string& uuid);
  static void activateOrMinimizeWindow(const std::string& uuid);
  static void closeWindow(const std::string& uuid);

  static void setAnchorAndStrut(QWidget* widget, LayerShellQt::Window::Anchors anchors,
                                uint32_t strutSize);
  static void setLayer(QWidget* widget, LayerShellQt::Window::Layer layer);

  static std::vector<QScreen*> screens();
  // Sets the widget to display on the screen with index `screen` (0-based).
  static void setScreen(QWidget* widget, int screen);

 private:

  // wl_registry interface.

  static void registry_global(
         void* data,
         struct wl_registry* registry,
         uint32_t name,
         const char* interface,
         uint32_t version
         );
  static void registry_global_remove(void *data,
                                             struct wl_registry *wl_registry,
                                             uint32_t name);

  // org_kde_plasma_virtual_desktop_management interface.

  static void desktop_management_desktop_created(
      void* data, org_kde_plasma_virtual_desktop_management* virtual_desktop_management,
      const char* desktop_id, uint32_t position);
  static void desktop_management_desktop_removed(
      void* data, org_kde_plasma_virtual_desktop_management* virtual_desktop_management,
      const char* desktop_id);
  static void desktop_management_done(void *data,
                                   struct org_kde_plasma_virtual_desktop_management* virtual_desktop_management);
  static void desktop_management_desktop_rows(
      void* data, org_kde_plasma_virtual_desktop_management* virtual_desktop_management,
      uint32_t rows);

  // org_kde_plasma_virtual_desktop interface.

  static void desktop_id(
      void *data,
      struct org_kde_plasma_virtual_desktop *org_kde_plasma_virtual_desktop,
      const char *desktop_id);

  static void desktop_name(
      void *data,
      struct org_kde_plasma_virtual_desktop *org_kde_plasma_virtual_desktop,
      const char *name);

  static void desktop_activated(
      void *data,
      struct org_kde_plasma_virtual_desktop *org_kde_plasma_virtual_desktop);

  static void desktop_deactivated(
      void *data,
      struct org_kde_plasma_virtual_desktop *org_kde_plasma_virtual_desktop);

  static void desktop_done(
      void *data,
      struct org_kde_plasma_virtual_desktop *org_kde_plasma_virtual_desktop);

  static void desktop_removed(
      void *data,
      struct org_kde_plasma_virtual_desktop *org_kde_plasma_virtual_desktop);

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

  static org_kde_plasma_virtual_desktop_management* virtual_desktop_management_;
  static org_kde_plasma_window_management* window_management_;

  static constexpr struct wl_registry_listener registry_listener_ = {
      registry_global,
      registry_global_remove
  };
  static constexpr struct org_kde_plasma_virtual_desktop_management_listener
      virtual_desktop_management_listener_ = {
          desktop_management_desktop_created,
          desktop_management_desktop_removed,
          desktop_management_done,
          desktop_management_desktop_rows
      };
  static constexpr struct org_kde_plasma_virtual_desktop_listener virtual_desktop_listener_ = {
      desktop_id,
      desktop_name,
      desktop_activated,
      desktop_deactivated,
      desktop_done,
      desktop_removed
  };
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

  static void initScreens();

  static std::vector<VirtualDesktopInfo> desktops_;
  // Current desktop ID.
  static std::string currentDesktop_;
  static bool showingDesktop_;

  static std::vector<QScreen*> screens_;

  static std::unique_ptr<QDBusInterface> activityManager_;

  static std::unordered_map<struct org_kde_plasma_window*, WindowInfo*> windows_;
  static std::unordered_map<std::string, struct org_kde_plasma_window*> uuids_;
  static std::vector<std::string> stackingOrder_;
  // Active window's UUID.
  static std::string activeUuid_;

  static ApplicationMenuConfig applicationMenuConfig_;
};

}  // namespace crystaldock

#endif  // CRYSTALDOCK_WINDOW_SYSTEM_H_
