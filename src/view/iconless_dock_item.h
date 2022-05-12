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

#ifndef CRYSTALDOCK_ICONLESS_DOCK_ITEM_H_
#define CRYSTALDOCK_ICONLESS_DOCK_ITEM_H_

#include "dock_item.h"

namespace crystaldock {

// Base class for dock items without an icon, such as clock.
class IconlessDockItem : public DockItem {
 public:
  IconlessDockItem(DockPanel* parent, const QString& label,
      Qt::Orientation orientation, int minSize, int maxSize,
      float whRatio, bool reverseWhRatio = false)
      : DockItem(parent, label, orientation, minSize, maxSize),
        whRatio_(whRatio), reverseWhRatio_(reverseWhRatio) {}
  virtual ~IconlessDockItem() {}

  int getWidthForSize(int size) const override;
  int getHeightForSize(int size) const override;

 protected:
  // Width/height ratio.
  float whRatio_;

  // Iff true, reverse width/height ratio when the orientation is vertical.
  bool reverseWhRatio_;
};

}  // namespace crystaldock

#endif  // CRYSTALDOCK_ICONLESS_DOCK_ITEM_H_
