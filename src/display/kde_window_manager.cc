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

#include "kde_window_manager.h"

namespace crystaldock {

org_kde_plasma_window_management* KdeWindowManager::window_management_;
std::unordered_map<struct org_kde_plasma_window*, std::unique_ptr<WindowInfo>>
    KdeWindowManager::windows_;
std::unordered_map<std::string, struct org_kde_plasma_window*> KdeWindowManager::uuids_;
std::vector<std::string> KdeWindowManager::stackingOrder_;
struct org_kde_plasma_window* KdeWindowManager::activeWindow_;
bool KdeWindowManager::showingDesktop_;

/* static */ KdeWindowManager* KdeWindowManager::self() {
  static KdeWindowManager self;
  return &self;
}

/* static */ void KdeWindowManager::init(
    struct org_kde_plasma_window_management* window_management) {
  window_management_ = window_management;
  org_kde_plasma_window_management_add_listener(
      window_management_, &window_management_listener_, NULL);
  connect(KdeWindowManager::self(), &KdeWindowManager::activeWindowChanged,
      WindowSystem::self(), &WindowSystem::activeWindowChanged);
  connect(KdeWindowManager::self(), &KdeWindowManager::windowAdded,
      WindowSystem::self(), &WindowSystem::windowAdded);
  connect(KdeWindowManager::self(), &KdeWindowManager::windowGeometryChanged,
      WindowSystem::self(), &WindowSystem::windowGeometryChanged);
  connect(KdeWindowManager::self(), &KdeWindowManager::windowLeftCurrentActivity,
      WindowSystem::self(), &WindowSystem::windowLeftCurrentActivity);
  connect(KdeWindowManager::self(), &KdeWindowManager::windowLeftCurrentDesktop,
      WindowSystem::self(), &WindowSystem::windowLeftCurrentDesktop);
  connect(KdeWindowManager::self(), &KdeWindowManager::windowRemoved,
      WindowSystem::self(), &WindowSystem::windowRemoved);
  connect(KdeWindowManager::self(), &KdeWindowManager::windowStateChanged,
      WindowSystem::self(), &WindowSystem::windowStateChanged);
  connect(KdeWindowManager::self(), &KdeWindowManager::windowTitleChanged,
      WindowSystem::self(), &WindowSystem::windowTitleChanged);
}

/* static */ void KdeWindowManager::bindWindowManagerFunctions(
    WindowManager* windowManager) {
  windowManager->activateOrMinimizeWindow = KdeWindowManager::activateOrMinimizeWindow;
  windowManager->activateWindow = KdeWindowManager::activateWindow;
  windowManager->minimizeWindow = KdeWindowManager::minimizeWindow;
  windowManager->activeWindow = KdeWindowManager::activeWindow;
  windowManager->closeWindow = KdeWindowManager::closeWindow;
  windowManager->resetActiveWindow = KdeWindowManager::resetActiveWindow;
  windowManager->windows = KdeWindowManager::windows;
  windowManager->setShowingDesktop = KdeWindowManager::setShowingDesktop;
  windowManager->showingDesktop = KdeWindowManager::showingDesktop;
}

/* static */ std::vector<const WindowInfo*> KdeWindowManager::windows() {
  std::vector<const WindowInfo*> windows;
  for (const auto& element : windows_) {
    if (element.second) {
      windows.push_back(element.second.get());
    }
  }
  std::sort(windows.begin(), windows.end(),
            [](const WindowInfo* w1, const WindowInfo* w2) {
    return w1->mapping_order < w2->mapping_order;
  });
  return windows;
}

/* static */ void* KdeWindowManager::activeWindow() {
  return activeWindow_;
}

/* static */ void KdeWindowManager::resetActiveWindow() {
  activeWindow_ = nullptr;
  emit KdeWindowManager::self()->activeWindowChanged(nullptr);
}

/* static */ void KdeWindowManager::activateWindow(void* window_handle) {
  auto* window = static_cast<struct org_kde_plasma_window*>(window_handle);
  if (window) {
    org_kde_plasma_window_set_state(
        window,
        ORG_KDE_PLASMA_WINDOW_MANAGEMENT_STATE_ACTIVE,
        ORG_KDE_PLASMA_WINDOW_MANAGEMENT_STATE_ACTIVE);
  }
}

/* static */ void KdeWindowManager::activateOrMinimizeWindow(void* window_handle) {
  auto* window = static_cast<struct org_kde_plasma_window*>(window_handle);
  if (window) {
    if (windows_.count(window) == 0) {
      return;
    }

    if (windows_[window]->minimized || window != activeWindow_) {
        org_kde_plasma_window_set_state(
            window,
            ORG_KDE_PLASMA_WINDOW_MANAGEMENT_STATE_ACTIVE,
            ORG_KDE_PLASMA_WINDOW_MANAGEMENT_STATE_ACTIVE);
    } else {
      org_kde_plasma_window_set_state(
          window,
          ORG_KDE_PLASMA_WINDOW_MANAGEMENT_STATE_MINIMIZED,
          ORG_KDE_PLASMA_WINDOW_MANAGEMENT_STATE_MINIMIZED);
    }
  }
}

/* static */ void KdeWindowManager::minimizeWindow(void* window_handle) {
  auto* window = static_cast<struct org_kde_plasma_window*>(window_handle);
  if (window) {
    org_kde_plasma_window_set_state(
        window,
        ORG_KDE_PLASMA_WINDOW_MANAGEMENT_STATE_MINIMIZED,
        ORG_KDE_PLASMA_WINDOW_MANAGEMENT_STATE_MINIMIZED);
  }
}

/*static*/ void KdeWindowManager::closeWindow(void* window_handle) {
  auto* window = static_cast<struct org_kde_plasma_window*>(window_handle);
  if (window) {
    org_kde_plasma_window_close(window);
  }
}

/* static */ bool KdeWindowManager::showingDesktop() {
  return showingDesktop_;
}

/* static */ void KdeWindowManager::setShowingDesktop(bool show) {
  // This does not work because it would hide Crystal Dock.
  /*
  org_kde_plasma_window_management_show_desktop(
      window_management_,
      show ? ORG_KDE_PLASMA_WINDOW_MANAGEMENT_SHOW_DESKTOP_ENABLED
           : ORG_KDE_PLASMA_WINDOW_MANAGEMENT_SHOW_DESKTOP_DISABLED);
  */

  // So we make our own implementation.
  for (const auto& uuid : stackingOrder_) {
    auto* window = uuids_[uuid];
    if (windows_[window]->desktop != WindowSystem::currentDesktop()) {
      continue;
    }

    if (show) {
      windows_[window]->restoreAfterShowDesktop = !windows_[window]->minimized;
      if (!windows_[window]->minimized) {
        org_kde_plasma_window_set_state(
            window,
            ORG_KDE_PLASMA_WINDOW_MANAGEMENT_STATE_MINIMIZED,
            ORG_KDE_PLASMA_WINDOW_MANAGEMENT_STATE_MINIMIZED);
      }
      showingDesktop_ = true;
    } else {
      if (windows_[window]->restoreAfterShowDesktop) {
        org_kde_plasma_window_set_state(
            window,
            ORG_KDE_PLASMA_WINDOW_MANAGEMENT_STATE_ACTIVE,
            ORG_KDE_PLASMA_WINDOW_MANAGEMENT_STATE_ACTIVE);
      }
      showingDesktop_ = false;
    }
  }
}

// org_kde_plasma_window_management interface.

/* static */ void KdeWindowManager::show_desktop_changed(
    void *data,
    struct org_kde_plasma_window_management *org_kde_plasma_window_management,
    uint32_t state) {
  // Ignore and use our own state.
  //showingDesktop_ = state & ORG_KDE_PLASMA_WINDOW_MANAGEMENT_SHOW_DESKTOP_ENABLED;
}

/* static */ void KdeWindowManager::window(
    void *data,
    struct org_kde_plasma_window_management *org_kde_plasma_window_management,
    uint32_t id) {
  // Ignore.
}

/* static */ void KdeWindowManager::stacking_order_changed(
    void *data,
    struct org_kde_plasma_window_management *org_kde_plasma_window_management,
    struct wl_array *ids) {
  // Ignore.
}

/* static */ void KdeWindowManager::stacking_order_uuid_changed(
    void *data,
    struct org_kde_plasma_window_management *org_kde_plasma_window_management,
    const char *uuids) {
  QStringList ids = QString(uuids).split(";", Qt::SkipEmptyParts);
  stackingOrder_.clear();
  for (const auto& id : ids) {
    stackingOrder_.push_back(id.toStdString());
  }
}

/* static */ void KdeWindowManager::window_with_uuid(
    void *data,
    struct org_kde_plasma_window_management *org_kde_plasma_window_management,
    uint32_t id,
    const char *uuid) {
  struct org_kde_plasma_window* window =
      org_kde_plasma_window_management_get_window_by_uuid(window_management_, uuid);
  windows_[window] = std::make_unique<WindowInfo>();
  windows_[window]->window = window;
  static uint32_t mapping_order = 0;
  windows_[window]->mapping_order = mapping_order++;
  uuids_[std::string{uuid}] = window;

  org_kde_plasma_window_add_listener(window, &window_listener_, NULL);
}

// org_kde_plasma_window interface.

/* static */ void KdeWindowManager::title_changed(
    void *data,
    struct org_kde_plasma_window* window,
    const char *title) {
  if (windows_.count(window) == 0) {
    return;
  }

  windows_[window]->title = title;
  if (windows_[window]->initialized) {
    emit self()->windowTitleChanged(windows_[window].get());
  }
}

/* static */ void KdeWindowManager::app_id_changed(
    void *data,
    struct org_kde_plasma_window* window,
    const char *app_id) {
  if (windows_.count(window) == 0) {
    return;
  }

  windows_[window]->appId = app_id;

  if (std::string(app_id) == "crystal-dock") {
    org_kde_plasma_window_set_state(
        window,
        ORG_KDE_PLASMA_WINDOW_MANAGEMENT_STATE_ON_ALL_DESKTOPS |
            ORG_KDE_PLASMA_WINDOW_MANAGEMENT_STATE_SKIPTASKBAR,
        ORG_KDE_PLASMA_WINDOW_MANAGEMENT_STATE_ON_ALL_DESKTOPS |
            ORG_KDE_PLASMA_WINDOW_MANAGEMENT_STATE_SKIPTASKBAR);
  }
}

/* static */ void KdeWindowManager::state_changed(
    void *data,
    struct org_kde_plasma_window* window,
    uint32_t flags) {
  if (windows_.count(window) == 0) {
    return;
  }

  windows_[window]->skipTaskbar = flags & ORG_KDE_PLASMA_WINDOW_MANAGEMENT_STATE_SKIPTASKBAR;
  windows_[window]->onAllDesktops = flags & ORG_KDE_PLASMA_WINDOW_MANAGEMENT_STATE_ON_ALL_DESKTOPS;
  windows_[window]->demandsAttention = flags & ORG_KDE_PLASMA_WINDOW_MANAGEMENT_STATE_DEMANDS_ATTENTION;
  windows_[window]->minimized = flags & ORG_KDE_PLASMA_WINDOW_MANAGEMENT_STATE_MINIMIZED;

  if (windows_[window]->minimized && activeWindow_ == window) {
      activeWindow_ = nullptr;
      if (windows_[window]->initialized) { emit self()->activeWindowChanged(activeWindow_); }
  } else if (flags & ORG_KDE_PLASMA_WINDOW_MANAGEMENT_STATE_ACTIVE && activeWindow_ != window) {
      activeWindow_ = window;
      if (windows_[window]->initialized) { emit self()->activeWindowChanged(activeWindow_); }
  }

  if (windows_[window]->initialized) {
    emit self()->windowStateChanged(windows_[window].get());
  }
}

/* static */ void KdeWindowManager::virtual_desktop_changed(
    void *data,
    struct org_kde_plasma_window* window,
    int32_t number) {
  // Ignore.
}

/* static */ void KdeWindowManager::themed_icon_name_changed(
    void *data,
    struct org_kde_plasma_window* window,
    const char *name) {
  if (windows_.count(window) == 0) {
    return;
  }

  windows_[window]->icon = name;
}

/* static */ void KdeWindowManager::unmapped(
    void *data,
    struct org_kde_plasma_window* window) {
  if (windows_.count(window) == 0) {
    return;
  }

  emit self()->windowRemoved(windows_[window]->window);
  windows_.erase(window);
}

/* static */ void KdeWindowManager::initial_state(
    void *data, struct org_kde_plasma_window* window) {
  if (windows_.count(window) == 0) {
    return;
  }

  windows_[window]->initialized = true;
  if (!windows_[window]->skipTaskbar) {
    emit self()->windowAdded(windows_[window].get());
  }
}

/* static */ void KdeWindowManager::parent_window(
    void *data,
    struct org_kde_plasma_window* window,
    struct org_kde_plasma_window *parent) {

}

/* static */ void KdeWindowManager::geometry(
    void *data,
    struct org_kde_plasma_window* window,
    int32_t x,
    int32_t y,
    uint32_t width,
    uint32_t height) {
  if (windows_.count(window) == 0) {
    return;
  }

  windows_[window]->x = x;
  windows_[window]->y = y;
  windows_[window]->width = width;
  windows_[window]->height = height;
  if (windows_[window]->initialized) {
    emit self()->windowGeometryChanged(windows_[window].get());
  }
}

/* static */ void KdeWindowManager::icon_changed(
    void *data,
    struct org_kde_plasma_window* window) {}

/* static */ void KdeWindowManager::pid_changed(
    void *data,
    struct org_kde_plasma_window* window,
    uint32_t pid) {}

/* static */ void KdeWindowManager::virtual_desktop_entered(
    void *data,
    struct org_kde_plasma_window* window,
    const char *id) {
  if (windows_.count(window) == 0) {
    return;
  }
  windows_[window]->desktop = id;
}

/* static */ void KdeWindowManager::virtual_desktop_left(
    void *data,
    struct org_kde_plasma_window* window,
    const char *id) {
  if (id != WindowSystem::currentDesktop()) {
    return;
  }
  if (windows_.count(window) == 0) {
    return;
  }
  if (windows_[window]->initialized && !windows_[window]->onAllDesktops) {
    emit self()->windowLeftCurrentDesktop(windows_[window]->window);
  }
}

/* static */ void KdeWindowManager::application_menu(
    void *data,
    struct org_kde_plasma_window* window,
    const char *service_name,
    const char *object_path) {}

/* static */ void KdeWindowManager::activity_entered(
    void *data,
    struct org_kde_plasma_window* window,
    const char *id) {
  if (windows_.count(window) == 0) {
    return;
  }

  windows_[window]->activity = id;
}

/* static */ void KdeWindowManager::activity_left(
    void *data,
    struct org_kde_plasma_window* window,
    const char *id) {
  if (id != WindowSystem::currentActivity()) {
    return;
  }
  if (windows_.count(window) == 0) {
    return;
  }
  if (windows_[window]->initialized) {
    emit self()->windowLeftCurrentActivity(windows_[window]->window);
  }
}

/* static */ void KdeWindowManager::resource_name_changed(
    void *data,
    struct org_kde_plasma_window* window,
    const char *resource_name) {}

}  // namespace crystaldock
