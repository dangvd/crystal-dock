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

#include "battery_indicator.h"

#include <QMessageBox>

#include "dock_panel.h"

#include <utils/command_utils.h>

namespace crystaldock {

BatteryIndicator::BatteryIndicator(DockPanel* parent, MultiDockModel* model,
                                   Qt::Orientation orientation, int minSize, int maxSize)
    : IconBasedDockItem(parent, model, kLabel, orientation, kIcon,
                        minSize, maxSize),
    updateTimer_(new QTimer(this)),
    process_(nullptr) {
  createMenu();

  // Sets up update timer.
  QTimer::singleShot(500, this, [this]() {
    connect(updateTimer_, &QTimer::timeout, this, &BatteryIndicator::refreshBatteryInfo);
    updateTimer_->start(kUpdateInterval);
    getBatteryDevice();
  });

  connect(&contextMenu_, &QMenu::aboutToHide, this,
          [this]() {
            parent_->setShowingPopup(false);
          });
}

BatteryIndicator::~BatteryIndicator() {
  if (process_ && process_->state() != QProcess::NotRunning) {
    process_->kill();
    process_->waitForFinished(1000);
  }
}

void BatteryIndicator::mousePressEvent(QMouseEvent* e) {
  if (e->button() == Qt::LeftButton) {
    if (commandExists({kCommand}).isEmpty()) {
      QMessageBox::warning(parent_, "Command not found",
                           QString("Command '") + kCommand + "' not found. This is required by the "
                               + kLabel + " component.");
      return;
    }
  } else if (e->button() == Qt::RightButton) {
    showPopupMenu(&contextMenu_);
  }
}

QString BatteryIndicator::getLabel() const {
  return hasBattery_
      ? batteryLevel_ > 0
          ? "Battery: " + QString::number(batteryLevel_) + "%"
              + (isCharging_ ? " (charging)" : "")
          : kLabel
      : "Battery: Not found";
}

void BatteryIndicator::refreshBatteryInfo() {
  // Prevent concurrent processes
  if (process_ && process_->state() != QProcess::NotRunning) {
    return;
  }

  if (batteryDevice_.isEmpty()) {
    return;
  }

  process_ = new QProcess(parent_);
  connect(process_, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
          [this](int exitCode, QProcess::ExitStatus exitStatus) {
            if (exitCode == 0) {
              bool isCharging = false;
              int batteryLevel = 0;
              QRegularExpression chargingRe(R"(state:(\s+)charging)");
              QRegularExpression percentageRe(R"(percentage:(\s+)(\d+.?\d*)(\s*)%)");
              for (const QString& line : process_->readAllStandardOutput().split('\n')) {
                if (chargingRe.match(line).hasMatch()) {
                  isCharging = true;
                }
                auto match = percentageRe.match(line);
                if (match.hasMatch()) {
                  batteryLevel = static_cast<int>(std::round(match.captured(2).toFloat()));
                }
              }
              if (isCharging_ != isCharging || batteryLevel_ != batteryLevel) {
                isCharging_ = isCharging;
                batteryLevel_ = batteryLevel;
                updateUi();
              }
            }
            process_->deleteLater();
            process_ = nullptr;
          });

  process_->start(kCommand, QStringList() << "-i" << batteryDevice_);
}

void BatteryIndicator::getBatteryDevice() {
  // Prevent concurrent processes
  if (process_ && process_->state() != QProcess::NotRunning) {
    return;
  }

  process_ = new QProcess(parent_);
  connect(process_, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
          [this](int exitCode, QProcess::ExitStatus exitStatus) {
            if (exitCode == 0) {
              for (const QString& device : process_->readAllStandardOutput().split('\n')) {
                if (device.toLower().contains("battery")) {
                  batteryDevice_ = device;
                  break;
                }
              }
              if (batteryDevice_.isEmpty()) {
                hasBattery_ = false;
              }
            }
            process_->deleteLater();
            process_ = nullptr;
          });

  process_->start(kCommand, QStringList() << "--enumerate");
}

void BatteryIndicator::createMenu() {
  contextMenu_.addSection(kLabel);
  contextMenu_.addSeparator();
  parent_->addPanelSettings(&contextMenu_);
}

void BatteryIndicator::updateUi() {
  QString iconName = kIcon;
  if (hasBattery_ && batteryLevel_ > 0 && !isCharging_) {
    if (batteryLevel_ < 20) {
      iconName = "battery-low";
    } else if (batteryLevel_ < 40) {
      iconName = "battery-caution";
    }
  }

  setIconName(iconName);
  parent_->update();
}

}  // namespace crystaldock
