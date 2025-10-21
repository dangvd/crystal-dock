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

#ifndef CRYSTALDOCK_ICON_BASED_DOCK_ITEM_H_
#define CRYSTALDOCK_ICON_BASED_DOCK_ITEM_H_

#include <vector>

#include <QPainter>
#include <QPixmap>
#include <QString>
#include <Qt>

#include "dock_item.h"

namespace crystaldock {

// Base class for icon-based dock items, such as launchers and pager icons.
class IconBasedDockItem : public DockItem {
 public:
  IconBasedDockItem(DockPanel* parent, MultiDockModel* model, const QString& label,
                    Qt::Orientation orientation, const QString& iconName, int minSize, int maxSize);
  IconBasedDockItem(DockPanel* parent, MultiDockModel* model, const QString& label,
                    Qt::Orientation orientation, const QPixmap& icon, int minSize, int maxSize);
  virtual ~IconBasedDockItem() {}

  int getWidthForSize(int size) const override {
    const auto& icon = getIcon(size);
    return !icon.isNull() ? icon.width() : size;
  }

  int getHeightForSize(int size) const override {
    const auto& icon = getIcon(size);
    return !icon.isNull() ? icon.height() : size;
  }

  void draw(QPainter* painter) const override;

  // Sets the icon on the fly.
  void setIcon(const QPixmap& icon);
  void setIconName(const QString& iconName, const QString& backupIconName = "");
  const QPixmap& getIcon(int size) const;
  QString getIconName() const { return iconName_; }

 protected:
  std::vector<QPixmap> icons_;

  QString iconName_;

 private:
  void generateIcons(const QPixmap& icon);

  friend class DockPanel;
};

}  // namespace crystaldock

#endif  // CRYSTALDOCK_ICON_BASED_DOCK_ITEM_H_
