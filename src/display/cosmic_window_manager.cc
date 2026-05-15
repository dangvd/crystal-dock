/*
 * This file is part of Crystal Dock.
 * Copyright (C) 2026 Crystal Dock contributors
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

#include "cosmic_window_manager.h"

#include <algorithm>
#include <iostream>

#include <QGuiApplication>

namespace crystaldock {

zcosmic_toplevel_info_v1* CosmicWindowManager::info_;
zcosmic_toplevel_manager_v1* CosmicWindowManager::manager_;

std::unordered_map<struct zcosmic_toplevel_handle_v1*, std::unique_ptr<WindowInfo>>
    CosmicWindowManager::windows_;
struct zcosmic_toplevel_handle_v1* CosmicWindowManager::activeWindow_;
struct zcosmic_toplevel_handle_v1* CosmicWindowManager::activeWindowBeforeShowDesktop_;
bool CosmicWindowManager::showingDesktop_;

/* static */ CosmicWindowManager* CosmicWindowManager::self() {
  static CosmicWindowManager self;
  return &self;
}

/* static */ void CosmicWindowManager::init(
    struct zcosmic_toplevel_info_v1* info,
    struct zcosmic_toplevel_manager_v1* manager) {
  info_ = info;
  manager_ = manager;
  zcosmic_toplevel_info_v1_add_listener(info_, &info_listener_, NULL);

  connect(CosmicWindowManager::self(), &CosmicWindowManager::activeWindowChanged,
      WindowSystem::self(), &WindowSystem::activeWindowChanged);
  connect(CosmicWindowManager::self(), &CosmicWindowManager::windowAdded,
      WindowSystem::self(), &WindowSystem::windowAdded);
  connect(CosmicWindowManager::self(), &CosmicWindowManager::windowGeometryChanged,
      WindowSystem::self(), &WindowSystem::windowGeometryChanged);
  connect(CosmicWindowManager::self(), &CosmicWindowManager::windowLeftCurrentDesktop,
      WindowSystem::self(), &WindowSystem::windowLeftCurrentDesktop);
  connect(CosmicWindowManager::self(), &CosmicWindowManager::windowRemoved,
      WindowSystem::self(), &WindowSystem::windowRemoved);
  connect(CosmicWindowManager::self(), &CosmicWindowManager::windowStateChanged,
      WindowSystem::self(), &WindowSystem::windowStateChanged);
  connect(CosmicWindowManager::self(), &CosmicWindowManager::windowTitleChanged,
      WindowSystem::self(), &WindowSystem::windowTitleChanged);
  connect(CosmicWindowManager::self(), &CosmicWindowManager::windowEnteredOutput,
      WindowSystem::self(), &WindowSystem::windowEnteredOutput);
  connect(CosmicWindowManager::self(), &CosmicWindowManager::windowLeftOutput,
      WindowSystem::self(), &WindowSystem::windowLeftOutput);
}

/* static */ void CosmicWindowManager::bindWindowManagerFunctions(
    WindowManager* windowManager) {
  windowManager->activateOrMinimizeWindow = CosmicWindowManager::activateOrMinimizeWindow;
  windowManager->activateWindow = CosmicWindowManager::activateWindow;
  windowManager->minimizeWindow = CosmicWindowManager::minimizeWindow;
  windowManager->activeWindow = CosmicWindowManager::activeWindow;
  windowManager->closeWindow = CosmicWindowManager::closeWindow;
  windowManager->resetActiveWindow = CosmicWindowManager::resetActiveWindow;
  windowManager->windows = CosmicWindowManager::windows;
  windowManager->setShowingDesktop = CosmicWindowManager::setShowingDesktop;
  windowManager->showingDesktop = CosmicWindowManager::showingDesktop;
}

/* static */ std::vector<const WindowInfo*> CosmicWindowManager::windows() {
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

/* static */ void* CosmicWindowManager::activeWindow() {
  return activeWindow_;
}

/* static */ void CosmicWindowManager::resetActiveWindow() {
  activeWindow_ = nullptr;
  emit CosmicWindowManager::self()->activeWindowChanged(nullptr);
}

/* static */ void CosmicWindowManager::activateWindow(void* window_handle) {
  auto* window = static_cast<struct zcosmic_toplevel_handle_v1*>(window_handle);
  if (!window || !manager_) {
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

  // Cosmic compositor expects unminimize-then-activate. Calling
  // unset_minimized on a non-minimized toplevel is a no-op.
  zcosmic_toplevel_manager_v1_unset_minimized(manager_, window);
  zcosmic_toplevel_manager_v1_activate(manager_, window, seat);
}

/* static */ void CosmicWindowManager::activateOrMinimizeWindow(void* window_handle) {
  auto* window = static_cast<struct zcosmic_toplevel_handle_v1*>(window_handle);
  if (!window || !manager_) {
    return;
  }
  if (windows_.count(window) == 0) {
    return;
  }

  if (windows_[window]->minimized || window != activeWindow_) {
    activateWindow(window);
  } else {
    zcosmic_toplevel_manager_v1_set_minimized(manager_, window);
  }
}

/* static */ void CosmicWindowManager::minimizeWindow(void* window_handle) {
  auto* window = static_cast<struct zcosmic_toplevel_handle_v1*>(window_handle);
  if (!window || !manager_) {
    return;
  }
  if (windows_.count(window) == 0) {
    return;
  }
  zcosmic_toplevel_manager_v1_set_minimized(manager_, window);
}

/*static*/ void CosmicWindowManager::closeWindow(void* window_handle) {
  auto* window = static_cast<struct zcosmic_toplevel_handle_v1*>(window_handle);
  if (!window || !manager_) {
    return;
  }
  zcosmic_toplevel_manager_v1_close(manager_, window);
}

/* static */ bool CosmicWindowManager::showingDesktop() {
  return showingDesktop_;
}

/* static */ void CosmicWindowManager::setShowingDesktop(bool show) {
  if (!manager_) {
    return;
  }
  for (const auto& [window, windowInfo] : windows_) {
    if (show) {
      windowInfo->restoreAfterShowDesktop = !windowInfo->minimized;
      if (!windowInfo->minimized) {
        zcosmic_toplevel_manager_v1_set_minimized(manager_, window);
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

// zcosmic_toplevel_info_v1 interface.

/* static */ void CosmicWindowManager::info_toplevel(
    void *data,
    struct zcosmic_toplevel_info_v1 *info,
    struct zcosmic_toplevel_handle_v1 *toplevel) {
  windows_[toplevel] = std::make_unique<WindowInfo>();
  windows_[toplevel]->window = toplevel;
  static uint32_t mapping_order = 0;
  windows_[toplevel]->mapping_order = mapping_order++;

  zcosmic_toplevel_handle_v1_add_listener(toplevel, &handle_listener_, NULL);
}

/* static */ void CosmicWindowManager::info_finished(
    void *data,
    struct zcosmic_toplevel_info_v1 *info) {}

/* static */ void CosmicWindowManager::info_done(
    void *data,
    struct zcosmic_toplevel_info_v1 *info) {}

// zcosmic_toplevel_handle_v1 interface.

/* static */ void CosmicWindowManager::handle_closed(
    void *data,
    struct zcosmic_toplevel_handle_v1 *handle) {
  if (windows_.count(handle) == 0) {
    return;
  }
  emit self()->windowRemoved(windows_[handle]->window);
  windows_.erase(handle);
}

/* static */ void CosmicWindowManager::handle_done(
    void *data,
    struct zcosmic_toplevel_handle_v1 *handle) {
  if (windows_.count(handle) == 0) {
    return;
  }
  if (!windows_[handle]->initialized) {
    windows_[handle]->initialized = true;
    emit self()->windowAdded(windows_[handle].get());
  }
}

/* static */ void CosmicWindowManager::handle_title(
    void *data,
    struct zcosmic_toplevel_handle_v1 *handle,
    const char *title) {
  if (windows_.count(handle) == 0) {
    return;
  }
  windows_[handle]->title = title;
  if (windows_[handle]->initialized) {
    emit self()->windowTitleChanged(windows_[handle].get());
  }
}

/* static */ void CosmicWindowManager::handle_app_id(
    void *data,
    struct zcosmic_toplevel_handle_v1 *handle,
    const char *app_id) {
  if (windows_.count(handle) == 0) {
    return;
  }
  windows_[handle]->appId = app_id;
}

/* static */ void CosmicWindowManager::handle_output_enter(
    void *data,
    struct zcosmic_toplevel_handle_v1 *handle,
    struct wl_output *output) {
  if (windows_.count(handle) == 0) {
    return;
  }
  windows_[handle]->outputs.insert(output);
  emit self()->windowEnteredOutput(windows_[handle].get(), output);
}

/* static */ void CosmicWindowManager::handle_output_leave(
    void *data,
    struct zcosmic_toplevel_handle_v1 *handle,
    struct wl_output *output) {
  if (windows_.count(handle) == 0) {
    return;
  }
  windows_[handle]->outputs.erase(output);
  emit self()->windowLeftOutput(windows_[handle].get(), output);
}

/* static */ void CosmicWindowManager::handle_workspace_enter(
    void *data,
    struct zcosmic_toplevel_handle_v1 *handle,
    struct zcosmic_workspace_handle_v1 *workspace) {
  // Workspace tracking not currently used; the dock filters tasks via
  // ext_workspace events on newer compositors and shows-all on older ones.
}

/* static */ void CosmicWindowManager::handle_workspace_leave(
    void *data,
    struct zcosmic_toplevel_handle_v1 *handle,
    struct zcosmic_workspace_handle_v1 *workspace) {}

/* static */ void CosmicWindowManager::handle_state(
    void *data,
    struct zcosmic_toplevel_handle_v1 *handle,
    struct wl_array *state) {
  if (windows_.count(handle) == 0) {
    return;
  }

  windows_[handle]->minimized = false;
  windows_[handle]->maximized = false;
  windows_[handle]->fullscreen = false;
  for (uint32_t* entry = static_cast<uint32_t*>(state->data);
       state->size != 0 && reinterpret_cast<const char*>(entry)
           < static_cast<const char*>(state->data) + state->size;
       ++entry) {
    if (*entry == ZCOSMIC_TOPLEVEL_HANDLE_V1_STATE_MAXIMIZED) {
      if (!windows_[handle]->minimized) {
        windows_[handle]->maximized = true;
      }
    }
    if (*entry == ZCOSMIC_TOPLEVEL_HANDLE_V1_STATE_MINIMIZED) {
      windows_[handle]->minimized = true;
      windows_[handle]->maximized = false;
      windows_[handle]->fullscreen = false;
      if (activeWindow_ == handle) {
        activeWindow_ = nullptr;
        if (windows_[handle]->initialized) {
          emit self()->activeWindowChanged(activeWindow_);
        }
      }
      break;
    }
    if (*entry == ZCOSMIC_TOPLEVEL_HANDLE_V1_STATE_ACTIVATED) {
      if (activeWindow_ != handle) {
        activeWindow_ = handle;
        if (windows_[handle]->initialized) {
          emit self()->activeWindowChanged(activeWindow_);
        }
      }
    }
    if (*entry == ZCOSMIC_TOPLEVEL_HANDLE_V1_STATE_FULLSCREEN) {
      if (!windows_[handle]->minimized) {
        windows_[handle]->fullscreen = true;
      }
    }
  }

  if (windows_[handle]->initialized) {
    emit self()->windowStateChanged(windows_[handle].get());
  }
}

/* static */ void CosmicWindowManager::handle_geometry(
    void *data,
    struct zcosmic_toplevel_handle_v1 *handle,
    struct wl_output *output,
    int32_t x,
    int32_t y,
    int32_t width,
    int32_t height) {
  if (windows_.count(handle) == 0) {
    return;
  }
  windows_[handle]->x = x;
  windows_[handle]->y = y;
  windows_[handle]->width = static_cast<uint32_t>(width);
  windows_[handle]->height = static_cast<uint32_t>(height);
  if (windows_[handle]->initialized) {
    emit self()->windowGeometryChanged(windows_[handle].get());
  }
}

/* static */ void CosmicWindowManager::handle_ext_workspace_enter(
    void *data,
    struct zcosmic_toplevel_handle_v1 *handle,
    struct ext_workspace_handle_v1 *workspace) {}

/* static */ void CosmicWindowManager::handle_ext_workspace_leave(
    void *data,
    struct zcosmic_toplevel_handle_v1 *handle,
    struct ext_workspace_handle_v1 *workspace) {}

}  // namespace crystaldock
