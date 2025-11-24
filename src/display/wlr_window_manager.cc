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

#include "wlr_window_manager.h"

#include <iostream>

#include <QGuiApplication>

namespace crystaldock {

zwlr_foreign_toplevel_manager_v1* WlrWindowManager::window_manager_;

std::unordered_map<struct zwlr_foreign_toplevel_handle_v1*, std::unique_ptr<WindowInfo>>
    WlrWindowManager::windows_;
struct zwlr_foreign_toplevel_handle_v1* WlrWindowManager::activeWindow_;
struct zwlr_foreign_toplevel_handle_v1* WlrWindowManager::activeWindowBeforeShowDesktop_;
bool WlrWindowManager::showingDesktop_;

/* static */ WlrWindowManager* WlrWindowManager::self() {
  static WlrWindowManager self;
  return &self;
}

/* static */ void WlrWindowManager::init(
    struct zwlr_foreign_toplevel_manager_v1* window_manager) {
  window_manager_ = window_manager;
  zwlr_foreign_toplevel_manager_v1_add_listener(
      window_manager_, &window_manager_listener_, NULL);
  connect(WlrWindowManager::self(), &WlrWindowManager::activeWindowChanged,
      WindowSystem::self(), &WindowSystem::activeWindowChanged);
  connect(WlrWindowManager::self(), &WlrWindowManager::windowAdded,
      WindowSystem::self(), &WindowSystem::windowAdded);
  connect(WlrWindowManager::self(), &WlrWindowManager::windowGeometryChanged,
      WindowSystem::self(), &WindowSystem::windowGeometryChanged);
  connect(WlrWindowManager::self(), &WlrWindowManager::windowLeftCurrentDesktop,
      WindowSystem::self(), &WindowSystem::windowLeftCurrentDesktop);
  connect(WlrWindowManager::self(), &WlrWindowManager::windowRemoved,
      WindowSystem::self(), &WindowSystem::windowRemoved);
  connect(WlrWindowManager::self(), &WlrWindowManager::windowStateChanged,
      WindowSystem::self(), &WindowSystem::windowStateChanged);
  connect(WlrWindowManager::self(), &WlrWindowManager::windowTitleChanged,
      WindowSystem::self(), &WindowSystem::windowTitleChanged);
  connect(WlrWindowManager::self(), &WlrWindowManager::windowEnteredOutput,
      WindowSystem::self(), &WindowSystem::windowEnteredOutput);
  connect(WlrWindowManager::self(), &WlrWindowManager::windowLeftOutput,
      WindowSystem::self(), &WindowSystem::windowLeftOutput);
}

/* static */ void WlrWindowManager::bindWindowManagerFunctions(
    WindowManager* windowManager) {
  windowManager->activateOrMinimizeWindow = WlrWindowManager::activateOrMinimizeWindow;
  windowManager->activateWindow = WlrWindowManager::activateWindow;
  windowManager->minimizeWindow = WlrWindowManager::minimizeWindow;
  windowManager->activeWindow = WlrWindowManager::activeWindow;
  windowManager->closeWindow = WlrWindowManager::closeWindow;
  windowManager->resetActiveWindow = WlrWindowManager::resetActiveWindow;
  windowManager->windows = WlrWindowManager::windows;
  windowManager->setShowingDesktop = WlrWindowManager::setShowingDesktop;
  windowManager->showingDesktop = WlrWindowManager::showingDesktop;
}

/* static */ std::vector<const WindowInfo*> WlrWindowManager::windows() {
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

/* static */ void* WlrWindowManager::activeWindow() {
  return activeWindow_;
}

/* static */ void WlrWindowManager::resetActiveWindow() {
  activeWindow_ = nullptr;
  emit WlrWindowManager::self()->activeWindowChanged(nullptr);
}

/* static */ void WlrWindowManager::activateWindow(void* window_handle) {
  auto* window = static_cast<struct zwlr_foreign_toplevel_handle_v1*>(window_handle);
  if (!window) {
    return;
  }

  auto app = dynamic_cast<QGuiApplication*>(QGuiApplication::instance());
  auto waylandApp = app->nativeInterface<QNativeInterface::QWaylandApplication>();
  if (!waylandApp) {
    return;
  }

  auto seat = waylandApp->seat();
  if (!seat) {
    return;
  }

  zwlr_foreign_toplevel_handle_v1_activate(window, seat);
}

/* static */ void WlrWindowManager::activateOrMinimizeWindow(void* window_handle) {
  auto* window = static_cast<struct zwlr_foreign_toplevel_handle_v1*>(window_handle);
  if (window) {
    if (windows_.count(window) == 0) {
      return;
    }

    if (windows_[window]->minimized || window != activeWindow_) {
      activateWindow(window);
    } else {
      zwlr_foreign_toplevel_handle_v1_set_minimized(window);
    }
  }
}

/* static */ void WlrWindowManager::minimizeWindow(void* window_handle) {
  auto* window = static_cast<struct zwlr_foreign_toplevel_handle_v1*>(window_handle);
  if (window) {
    if (windows_.count(window) == 0) {
      return;
    }

    zwlr_foreign_toplevel_handle_v1_set_minimized(window);
  }
}

/*static*/ void WlrWindowManager::closeWindow(void* window_handle) {
  auto* window = static_cast<struct zwlr_foreign_toplevel_handle_v1*>(window_handle);
  if (window) {
    zwlr_foreign_toplevel_handle_v1_close(window);
  }
}

/* static */ bool WlrWindowManager::showingDesktop() {
  return showingDesktop_;
}

/* static */ void WlrWindowManager::setShowingDesktop(bool show) {
  for (const auto& [window, windowInfo] : windows_) {
    if (show) {
      windowInfo->restoreAfterShowDesktop = !windowInfo->minimized;
      if (!windowInfo->minimized) {
        zwlr_foreign_toplevel_handle_v1_set_minimized(window);
      }
      if (activeWindow_) {
        activeWindowBeforeShowDesktop_ = activeWindow_;
      }
      showingDesktop_ = true;
    } else {
      if (windowInfo->restoreAfterShowDesktop) {
        activateWindow(window);
      }
      if (activeWindowBeforeShowDesktop_) {
        activateWindow(activeWindowBeforeShowDesktop_);
      }
      showingDesktop_ = false;
    }
  }
}

// zwlr_foreign_toplevel_manager_v1 interface.

/* static */ void WlrWindowManager::toplevel(
    void *data,
    struct zwlr_foreign_toplevel_manager_v1 *zwlr_foreign_toplevel_manager_v1,
    struct zwlr_foreign_toplevel_handle_v1 *window) {
  windows_[window] = std::make_unique<WindowInfo>();
  windows_[window]->window = window;
  static uint32_t mapping_order = 0;
  windows_[window]->mapping_order = mapping_order++;

  zwlr_foreign_toplevel_handle_v1_add_listener(window, &window_listener_, NULL);
}

/* static */ void WlrWindowManager::finished(
    void *data,
    struct zwlr_foreign_toplevel_manager_v1 *zwlr_foreign_toplevel_manager_v1) {}

// zwlr_foreign_toplevel_handle_v1 interface.

/* static */ void WlrWindowManager::title(
    void *data,
    struct zwlr_foreign_toplevel_handle_v1 *window,
    const char *title) {
  if (windows_.count(window) == 0) {
    return;
  }

  windows_[window]->title = title;
  if (windows_[window]->initialized) {
    emit self()->windowTitleChanged(windows_[window].get());
  }
}

/* static */ void WlrWindowManager::app_id(
    void *data,
    struct zwlr_foreign_toplevel_handle_v1 *window,
    const char *app_id) {
  if (windows_.count(window) == 0) {
    return;
  }

  windows_[window]->appId = app_id;
}

/* static */ void WlrWindowManager::output_enter(
    void *data,
    struct zwlr_foreign_toplevel_handle_v1 *window,
    struct wl_output *output) {
  if (windows_.count(window) == 0) {
    return;
  }

  windows_[window]->outputs.insert(output);
  emit self()->windowEnteredOutput(windows_[window].get(), output);
}

/* static */ void WlrWindowManager::output_leave(
    void *data,
    struct zwlr_foreign_toplevel_handle_v1 *window,
    struct wl_output *output) {
  if (windows_.count(window) == 0) {
    return;
  }

  windows_[window]->outputs.erase(output);
  emit self()->windowLeftOutput(windows_[window].get(), output);
}

/* static */ void WlrWindowManager::state(
    void *data,
    struct zwlr_foreign_toplevel_handle_v1 *window,
    struct wl_array *state) {
  if (windows_.count(window) == 0) {
    return;
  }

  windows_[window]->minimized = false;
  windows_[window]->maximized = false;
  windows_[window]->fullscreen = false;
  for (uint32_t* entry = static_cast<uint32_t*>(state->data);
       state->size != 0 && reinterpret_cast<const char*>(entry)
           < static_cast<const char*>(state->data) + state->size;
       ++entry) {
    if (*entry == ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_MAXIMIZED) {
      if (!windows_[window]->minimized) {
        windows_[window]->maximized = true;
      }
    }
    if (*entry == ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_MINIMIZED) {
      windows_[window]->minimized = true;
      windows_[window]->maximized = false;
      windows_[window]->fullscreen = false;
      if (activeWindow_ == window) {
        activeWindow_ = nullptr;
        if (windows_[window]->initialized) {
          emit self()->activeWindowChanged(activeWindow_);
        }
      }
      // This is to fix the case where a window could be Minimized
      // and Activated at the same time on Wayfire.
      break;
    }
    if (*entry == ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_ACTIVATED) {
      if (activeWindow_ != window) {
        activeWindow_ = window;
        if (windows_[window]->initialized) {
          emit self()->activeWindowChanged(activeWindow_);
        }
      }
    }
    if (*entry == ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_FULLSCREEN) {
      if (!windows_[window]->minimized) {
        windows_[window]->fullscreen = true;
      }
    }
  }

  if (windows_[window]->initialized) {
    emit self()->windowStateChanged(windows_[window].get());
  }
}

/* static */ void WlrWindowManager::done(
    void *data,
    struct zwlr_foreign_toplevel_handle_v1 *window) {
  if (windows_.count(window) == 0) {
    return;
  }

  windows_[window]->initialized = true;
  emit self()->windowAdded(windows_[window].get());
}

/* static */ void WlrWindowManager::closed(
    void *data,
    struct zwlr_foreign_toplevel_handle_v1 *window) {
  if (windows_.count(window) == 0) {
    return;
  }

  emit self()->windowRemoved(windows_[window]->window);
  windows_.erase(window);
}

/* static */ void WlrWindowManager::parent(
    void *data,
    struct zwlr_foreign_toplevel_handle_v1 *window,
    struct zwlr_foreign_toplevel_handle_v1 *parent) {}

}  // namespace crystaldock
