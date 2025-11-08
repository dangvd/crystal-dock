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

#ifndef CRYSTAL_DOCK_BATTERY_INDICATOR_H_
#define CRYSTAL_DOCK_BATTERY_INDICATOR_H_

#include "icon_based_dock_item.h"

#include <QMouseEvent>
#include <QProcess>
#include <QTimer>

namespace crystaldock {

// A battery indicator that integrates with upower.
class BatteryIndicator : public QObject, public IconBasedDockItem {
  Q_OBJECT

 public:
  BatteryIndicator(DockPanel* parent, MultiDockModel* model, Qt::Orientation orientation,
                   int minSize, int maxSize);
  virtual ~BatteryIndicator();

  void mousePressEvent(QMouseEvent* e) override;
  QString getLabel() const override;
  bool beforeTask(const QString& program) override { return false; }

 public slots:
  void refreshBatteryInfo();

 private:
  static constexpr char kCommand[] = "upower";
  static constexpr char kLabel[] = "Battery Indicator";
  static constexpr char kIcon[] = "battery";
  static constexpr int kUpdateInterval = 1000;  // 1 second.

  void getBatteryDevice();

  // Creates the context menu.
  void createMenu();

  void updateUi();

  bool hasBattery_ = true;
  QString batteryDevice_;
  int batteryLevel_ = 0;  // in percentage.
  bool isCharging_ = false;

  // Update timer.
  QTimer* updateTimer_;

  // upower process.
  QProcess* process_ = nullptr;

  // Right-click context menu.
  QMenu contextMenu_;
};

}  // namespace crystaldock

#endif  // CRYSTAL_DOCK_BATTERY_INDICATOR_H_
