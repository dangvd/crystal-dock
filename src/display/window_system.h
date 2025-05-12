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
#include <QWindow>

#include "kde_screen_edge.h"
#include "plasma_virtual_desktop.h"
#include "plasma_window_management.h"
#include "wlr_foreign_toplevel_management.h"

#include <LayerShellQt/Window>

namespace crystaldock {

struct VirtualDesktopInfo {
  std::string id;
  uint32_t number;  // 1-based.
  std::string name;
  // Pointer to an implementation-specific virtual desktop struct.
  void* virtual_desktop;
};

struct WindowInfo {
  // Pointer to an implementation-specific windows struct.
  void* window;
  std::string appId;
  std::string title;
  std::string icon;
  std::string desktop;
  std::string activity;
  bool initialized;
  bool skipTaskbar;
  bool onAllDesktops;
  bool demandsAttention;
  bool minimized;
  bool restoreAfterShowDesktop;
  int32_t x;
  int32_t y;
  uint32_t width;
  uint32_t height;
  uint32_t mapping_order;
};

struct VirtualDesktopManager {
  int (*numberOfDesktops)();
  std::vector<VirtualDesktopInfo> (*desktops)();
  std::string_view (*currentDesktop)();
  void (*setCurrentDesktop)(std::string_view);
};

struct WindowManager {
  std::vector<const WindowInfo*> (*windows)();
  void* (*activeWindow)();
  // We manually reset active window, usually when the new active window is the dock itself.
  // We don't want to always do this (e.g. handle this in state_change() handler) because
  // otherwise we wouldn't be able to click on an active window's icon to minimize it
  // (the click action would change the active window to be the dock).
  void (*resetActiveWindow)();
  void (*activateWindow)(void* window);
  void (*activateOrMinimizeWindow)(void* window);
  void (*closeWindow)(void* window);
  bool (*showingDesktop)();
  void (*setShowingDesktop)(bool show);
};

struct AutoHideManager {
  void (*setAutoHide)(QWidget* widget, Qt::Edge edge, bool on);
};

class WindowSystem : public QObject {
  Q_OBJECT

 private:
  WindowSystem() = default;

 signals:
  void currentDesktopChanged(std::string_view);
  void numberOfDesktopsChanged(int);
  void desktopNameChanged(std::string_view desktopId, std::string_view desktopName);

  void windowAdded(const WindowInfo*);
  void windowRemoved(void*);
  void windowLeftCurrentDesktop(void*);
  void windowGeometryChanged(const WindowInfo*);
  void windowStateChanged(const WindowInfo*);
  void windowTitleChanged(const WindowInfo*);
  void activeWindowChanged(void*);
  void windowLeftCurrentActivity(void*);

  void currentActivityChanged(std::string_view);

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

  static bool hasVirtualDesktopManager();
  static bool hasAutoHideManager();
  static bool hasActivityManager();

  static int numberOfDesktops() {
    if (hasVirtualDesktopManager()) {
      return virtualDesktopManager_.numberOfDesktops();
    }
    return 1;
  }

  static std::vector<VirtualDesktopInfo> desktops() {
    if (hasVirtualDesktopManager()) {
      return virtualDesktopManager_.desktops();
    }
    return {};
  }

  static std::string_view currentDesktop() {
    if (hasVirtualDesktopManager()) {
      return virtualDesktopManager_.currentDesktop();
    }
    static constexpr char kDesktop[] = "";
    return kDesktop;
  }

  static void setCurrentDesktop(std::string_view desktop) {
    if (hasVirtualDesktopManager()) {
      virtualDesktopManager_.setCurrentDesktop(desktop);
    }
  }

  static std::vector<const WindowInfo*> windows() { return windowManager_.windows(); }
  static void* activeWindow() { return windowManager_.activeWindow(); }
  // We manually reset active window, usually when the new active window is the dock itself.
  // We don't want to always do this (e.g. handle this in state_change() handler) because
  // otherwise we wouldn't be able to click on an active window's icon to minimize it
  // (the click action would change the active window to be the dock).
  static void resetActiveWindow() { windowManager_.resetActiveWindow(); }
  static void activateWindow(void* window) { windowManager_.activateWindow(window); }
  static void activateOrMinimizeWindow(void* window) {
    windowManager_.activateOrMinimizeWindow(window);
  }
  static void closeWindow(void* window) { windowManager_.closeWindow(window); }
  static bool showingDesktop() { return windowManager_.showingDesktop(); }
  static void setShowingDesktop(bool show) { windowManager_.setShowingDesktop(show); }

  static void setAutoHide(QWidget* widget, Qt::Edge edge, bool on = true) {
    if (hasAutoHideManager()) {
      autoHideManager_.setAutoHide(widget, edge, on);
    }
  }

  static void setAnchorAndStrut(QWidget* widget, LayerShellQt::Window::Anchors anchors,
                                uint32_t strutSize);
  static void setLayer(QWidget* widget, LayerShellQt::Window::Layer layer);

  static std::vector<QScreen*> screens();
  // Sets the widget to display on the screen with index `screen` (0-based).
  static void setScreen(QWidget* widget, int screen);

  static QWindow* getWindow(QWidget* widget) {
    widget->winId();  // we need this for widget->windowHandle() to not return nullptr.
    return widget->windowHandle();
  }

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



  static constexpr struct wl_registry_listener registry_listener_ = {
      registry_global,
      registry_global_remove
  };


  static void initScreens();

  static org_kde_plasma_virtual_desktop_management* kde_virtual_desktop_management_;
  static org_kde_plasma_window_management* kde_window_management_;
  static kde_screen_edge_manager_v1* kde_screen_edge_manager_;

  static zwlr_foreign_toplevel_manager_v1* wlr_window_manager_;

  static VirtualDesktopManager virtualDesktopManager_;
  static WindowManager windowManager_;
  static AutoHideManager autoHideManager_;

  static std::vector<QScreen*> screens_;

  static std::unique_ptr<QDBusInterface> activityManager_;
};

}  // namespace crystaldock

#endif  // CRYSTALDOCK_WINDOW_SYSTEM_H_
