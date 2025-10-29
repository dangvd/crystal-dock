/*
 * This file is part of Crystal Dock.
 * Copyright (C) 2022 Viet Dang (dangvd@gmail.com)
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

#ifndef CRYSTALDOCK_DESKTOP_SELECTOR_H_
#define CRYSTALDOCK_DESKTOP_SELECTOR_H_

#include "icon_based_dock_item.h"

#include <memory>

#include <QAction>
#include <QMenu>
#include <QObject>
#include <QString>

#include "display/window_system.h"

#include <desktop/desktop_env.h>

namespace crystaldock {

// Pager icons.
class DesktopSelector : public QObject, public IconBasedDockItem {
  Q_OBJECT

 public:
  DesktopSelector(DockPanel* parent, MultiDockModel* model,
                  Qt::Orientation orientation, int minSize, int maxSize,
                  const VirtualDesktopInfo& desktop, int screen);

  virtual ~DesktopSelector() = default;

  int getWidthForSize(int size) const override {
    return isHorizontal() ? (size * desktopWidth_ / desktopHeight_) : size;
  }

  int getHeightForSize(int size) const override {
    return isHorizontal() ? size : (size * desktopHeight_ / desktopWidth_);
  }

  void draw(QPainter* painter) const override;
  void mousePressEvent(QMouseEvent* e) override;
  void mouseReleaseEvent(QMouseEvent* e) override;
  void loadConfig() override;

  // Sets the icon but scales the pixmap to the screen's width/height ratio.
  void setIconScaled(const QPixmap& icon);

 public slots:
  void onDesktopNameChanged(std::string_view desktopId, std::string_view desktopName);

 private:
  bool isCurrentDesktop() const {
    return WindowSystem::currentDesktop() == desktop_.id;
  }

  void createMenu();

  void saveConfig();

  DesktopEnv* desktopEnv_;

  VirtualDesktopInfo desktop_;
  // The screen that the parent panel is on, 0-based.
  int screen_;
  QSettings* config_;

  // Context (right-click) menu.
  QMenu menu_;

  QAction* titleAction_;
  QAction* showDesktopNumberAction_;

  int desktopWidth_;
  int desktopHeight_;

  bool hasCustomWallpaper_;
};

}  // namespace crystaldock

#endif  // CRYSTALDOCK_DESKTOP_SELECTOR_H_
