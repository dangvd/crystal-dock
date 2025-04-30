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

#ifndef KDE_AUTO_HIDE_MANAGER_H_
#define KDE_AUTO_HIDE_MANAGER_H_

#include "kde_screen_edge.h"
#include "window_system.h"

namespace crystaldock {

class KdeAutoHideManager : public QObject {
  Q_OBJECT

 private:
  KdeAutoHideManager() = default;

 public:
  static KdeAutoHideManager* self();
  static void init(struct kde_screen_edge_manager_v1* screen_edge_manager);

  static void bindAutoHideManagerFunctions(AutoHideManager* autoHideManager);

  static void setAutoHide(QWidget* widget, Qt::Edge edge, bool on);

 private:
  static kde_screen_edge_manager_v1* screen_edge_manager_;
};

}

#endif // KDE_AUTO_HIDE_MANAGER_H_
