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

#include "icon_based_dock_item.h"

#include <QIcon>
#include <QImage>

#include "dock_panel.h"

#include <utils/draw_utils.h>
#include <utils/icon_utils.h>

namespace crystaldock {

IconBasedDockItem::IconBasedDockItem(DockPanel* parent, MultiDockModel* model, const QString& label,
                                     Qt::Orientation orientation, const QString& iconName,
                                     int minSize, int maxSize)
    : DockItem(parent, model, label, orientation, minSize, maxSize),
    icons_(maxSize - minSize + 1) {
  setIconName(iconName);
}

IconBasedDockItem::IconBasedDockItem(DockPanel* parent, MultiDockModel* model, const QString& label,
                                     Qt::Orientation orientation, const QPixmap& icon,
                                     int minSize, int maxSize)
    : DockItem(parent, model, label, orientation, minSize, maxSize),
    icons_(maxSize - minSize + 1) {
  setIcon(icon);
}

void IconBasedDockItem::draw(QPainter* painter) const {
  const auto& icon = icons_[size_ - minSize_];
  if (!icon.isNull()) {
    painter->drawPixmap(left_, top_, icon);
  } else {  // Fall-back "icon".
    QColor fillColor = model_->backgroundColor();
    fillColor.setAlphaF(kDefaultBackgroundAlpha);
    drawFallbackIcon(left_, top_, size_, model_->borderColor(), fillColor, painter);
  }
}

void IconBasedDockItem::setIcon(const QPixmap& icon) {
  generateIcons(icon);
}

void IconBasedDockItem::setIconName(const QString& iconName) {
  QPixmap icon = loadIcon(iconName, DockPanel::kIconLoadSize);
  if (!icon.isNull()) {
    iconName_ = iconName;
    setIcon(icon);
  }
}

const QPixmap& IconBasedDockItem::getIcon(int size) const {
  if (size < minSize_) {
    size = minSize_;
  } else if (size > maxSize_) {
    size = maxSize_;
  }
  return icons_[size - minSize_];
}

void IconBasedDockItem::generateIcons(const QPixmap& icon) {
  QImage image = icon.toImage(); // Convert to QImage for fast scaling.
  if (image.isNull()) {
    return;
  }

  for (int size = minSize_; size <= maxSize_; ++size) {
    icons_[size - minSize_] = QPixmap::fromImage(
        (orientation_ == Qt::Horizontal)
            ? image.scaledToHeight(size, Qt::SmoothTransformation)
            : image.scaledToWidth(size, Qt::SmoothTransformation));
    //https://doc.qt.io/qt-6/highdpi.html
    icons_[size - minSize_].setDevicePixelRatio(1.0f);
  }
}

}  // namespace crystaldock
