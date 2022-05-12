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

#include "iconless_dock_item.h"

namespace crystaldock {

int IconlessDockItem::getWidthForSize(int size) const {
  return isHorizontal() ? static_cast<int>(size * whRatio_) : size;
}

int IconlessDockItem::getHeightForSize(int size) const {
  return isHorizontal() ? size : static_cast<int>(reverseWhRatio_
      ? (size * whRatio_)
      : (size / whRatio_));
}

}  // namespace crystaldock
