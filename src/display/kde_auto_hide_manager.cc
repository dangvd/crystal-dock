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

#include <iostream>

#include <qpa/qplatformwindow_p.h>

#include "kde_auto_hide_manager.h"

namespace crystaldock {

kde_screen_edge_manager_v1* KdeAutoHideManager::screen_edge_manager_;

/* static */ KdeAutoHideManager* KdeAutoHideManager::self() {
  static KdeAutoHideManager* self = nullptr;
  if (!self) {
    self = new KdeAutoHideManager();
  }
  return self;
}

/* static */ void KdeAutoHideManager::init(
    struct kde_screen_edge_manager_v1* screen_edge_manager) {
  screen_edge_manager_ = screen_edge_manager;
}

/* static */ void KdeAutoHideManager::bindAutoHideManagerFunctions(
    AutoHideManager* autoHideManager) {
  autoHideManager->setAutoHide = KdeAutoHideManager::setAutoHide;
}

/* static */ void KdeAutoHideManager::setAutoHide(QWidget* widget, Qt::Edge edge, bool on) {
  auto* window = WindowSystem::getWindow(widget);
  if (!window) {
    return;
  }
  auto* waylandWindow = window->nativeInterface<QNativeInterface::Private::QWaylandWindow>();
  if (!waylandWindow) {
    std::cerr << "Failed to get Wayland window" << std::endl;
    return;
  }

  auto* surface = waylandWindow->surface();
  if (!surface) {
    std::cerr << "Failed to get Wayland surface" << std::endl;
    return;
  }

  kde_screen_edge_manager_v1_border border = KDE_SCREEN_EDGE_MANAGER_V1_BORDER_BOTTOM;
  switch (edge) {
    case Qt::TopEdge:
      border = KDE_SCREEN_EDGE_MANAGER_V1_BORDER_TOP;
      break;
    case Qt::BottomEdge:
      border = KDE_SCREEN_EDGE_MANAGER_V1_BORDER_BOTTOM;
      break;
    case Qt::LeftEdge:
      border = KDE_SCREEN_EDGE_MANAGER_V1_BORDER_LEFT;
      break;
    case Qt::RightEdge:
      border = KDE_SCREEN_EDGE_MANAGER_V1_BORDER_RIGHT;
      break;
    }
  auto* screen_edge = kde_screen_edge_manager_v1_get_auto_hide_screen_edge(
      screen_edge_manager_, border, surface);
  if (!screen_edge) {
    std::cerr << "Failed to get Auto Hide screen edge object" << std::endl;
    return;
  }
  if (on) {
    kde_auto_hide_screen_edge_v1_activate(screen_edge);
  } else {
    kde_auto_hide_screen_edge_v1_deactivate(screen_edge);
  }
}

}  // namespace crystaldock
