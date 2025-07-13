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

#ifndef CRYSTALDOCK_VERSION_CHECKER_H_
#define CRYSTALDOCK_VERSION_CHECKER_H_

#include "icon_based_dock_item.h"

#include <QMessageBox>

namespace crystaldock {

enum class VersionStatus { Alpha, Beta, OutOfDate, UpToDate };

class VersionChecker : public QObject, public IconBasedDockItem {
  Q_OBJECT
 public:
  VersionChecker(DockPanel* parent, MultiDockModel* model, Qt::Orientation orientation,
        int minSize, int maxSize);
  virtual ~VersionChecker() = default;

  bool beforeTask(const QString& program) override { return false; }
  void mousePressEvent(QMouseEvent* e) override;

 private:
  void checkVersion();
  void setVersionStatus(VersionStatus status);
  void createMenu();
  void showVersionInfo();

  VersionStatus status_;
  QMenu menu_;
  QTimer* timer_ = nullptr;
  uint32_t timerInterval_ = 60 * 60 * 1000;  // hourly.
  QMessageBox infoDialog_;
};

}  // namespace crystaldock

#endif // CRYSTALDOCK_VERSION_CHECKER_H_
