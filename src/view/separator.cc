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

#include "separator.h"

#include "dock_panel.h"

namespace crystaldock {

constexpr float Separator::kWhRatio;

Separator::Separator(DockPanel* parent, MultiDockModel* model, Qt::Orientation orientation,
                     int minSize, int maxSize)
    : IconlessDockItem(parent, model, "" /* label */, orientation, minSize, maxSize,
                       kWhRatio, /*reverseWhRatio=*/ true) {
  whRatio_ = 0.5;
}

void Separator::draw(QPainter* painter) const {
  int x, y, w, h;
  if (orientation_ == Qt::Horizontal) {
    x = left_ + getWidth() / 2;
    y = (parent_->position() == PanelPosition::Top)
        ? top_
        : getHeight() - getMinHeight() + top_;
    w = 1;
    h = getMinHeight();
  } else {  // Vertical.
    x = (parent_->position() == PanelPosition::Left)
        ? left_
        : getWidth() - getMinWidth() + left_;
    y = top_ + getHeight() / 2;
    w = getMinWidth();
    h = 1;
  }

  if (parent_->is3D()) {
    if (parent_->isBottom()) {
      // For 3D docks, do not draw anything for now.
    } else {
      painter->fillRect(x, y, w, h, model_->borderColor());
    }
  } else {
    painter->fillRect(x, y, w, h, model_->backgroundColor2D().lighter());
  }
}

}  // namespace crystaldock
