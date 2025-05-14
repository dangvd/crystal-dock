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

#ifndef CRYSTALDOCK_SEPARATOR_H_
#define CRYSTALDOCK_SEPARATOR_H_

#include "iconless_dock_item.h"

namespace crystaldock {

// A digital Separator.
class Separator : public QObject, public IconlessDockItem {
  Q_OBJECT

 public:
  Separator(DockPanel* parent, MultiDockModel* model, Qt::Orientation orientation,
        int minSize, int maxSize, bool isLauncherSeparator);
  virtual ~Separator() = default;

  void draw(QPainter* painter) const override;

  void mousePressEvent(QMouseEvent* e) override { /* no-op */ }

  bool beforeTask(const QString& program) override { return isLauncherSeparator_; }

  QString getAppId() const override {
    return isLauncherSeparator_ ? kLauncherSeparatorId : kSeparatorId;
  }

 private:
  static constexpr float kWhRatio = 0.1;

  // A Launcher Separator will push task icons, that do not belong to pinned
  // applications, behind it.
  bool isLauncherSeparator_ = false;
};

}  // namespace crystaldock

#endif  // CRYSTALDOCK_SEPARATOR_H_
