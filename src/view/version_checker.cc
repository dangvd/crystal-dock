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

#include "version_checker.h"

#include <QActionGroup>
#include <QJsonDocument>
#include <QJsonObject>
#include <QOverload>
#include <QProcess>
#include <QTimer>

#include "dock_panel.h"

namespace crystaldock {

VersionChecker::VersionChecker(DockPanel* parent, MultiDockModel* model,
                               Qt::Orientation orientation, int minSize, int maxSize)
    : IconBasedDockItem(parent, model, "Version Checker", orientation, "",
                        minSize, maxSize),
      infoDialog_(QMessageBox::Information, "Version Information",
                  QString{}, QMessageBox::Ok, parent, Qt::Tool) {
  createMenu();

  const QString version = DockPanel::kVersion;
  if (version.toLower().contains("alpha")) {
    setVersionStatus(VersionStatus::Alpha);
  } else if (version.toLower().contains("beta") || version.toLower().contains("rc")) {
    setVersionStatus(VersionStatus::Beta);
  } else {
    setVersionStatus(VersionStatus::UpToDate);
  }
  if (status_ == VersionStatus::UpToDate) {
    // checks version now and every hour.
    QTimer::singleShot(1000, this, &VersionChecker::checkVersion);
    timer_ = new QTimer(this);
    connect(timer_, &QTimer::timeout, this, &VersionChecker::checkVersion);
    timer_->start(timerInterval_);
  }

  connect(&menu_, &QMenu::aboutToHide, this,
          [this]() {
            parent_->setShowingPopup(false);
          });
}

void VersionChecker::mousePressEvent(QMouseEvent* e) {
  if (e->button() == Qt::LeftButton) {
    parent_->minimize();
    QTimer::singleShot(DockPanel::kExecutionDelayMs, [this]{
      showVersionInfo();
    });
  } else if (e->button() == Qt::RightButton) {
    showPopupMenu(&menu_);
  }
}

void VersionChecker::checkVersion() {
  if (status_ != VersionStatus::UpToDate) {
    return;
  }

  QProcess* curlProcess = new QProcess(parent_);
  connect(curlProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
          [this, curlProcess](int exitCode, QProcess::ExitStatus exitStatus) {
            if (exitCode == 0) {
              const QString output = curlProcess->readAllStandardOutput();
              const QJsonDocument jsonDoc = QJsonDocument::fromJson(output.toUtf8());
              if (jsonDoc.isNull() || !jsonDoc.isObject()) {
                return;
              }
              const QJsonObject json = jsonDoc.object();
              if (!json.contains("tag_name")) {
                return;
              }
              QString latestRelease = json.value("tag_name").toString().trimmed();
              latestRelease = latestRelease.mid(latestRelease.indexOf("v") + 1);
              const QString version = DockPanel::kVersion;
              if (version == latestRelease) {
                setVersionStatus(VersionStatus::UpToDate);
              } else {
                setVersionStatus(VersionStatus::OutOfDate);
              }
            }
            curlProcess->deleteLater();
          });
  curlProcess->start("curl", QStringList() << "-s" << "https://api.github.com/repos/dangvd/crystal-dock/releases/latest");
}

void VersionChecker::setVersionStatus(VersionStatus status) {
  status_ = status;
  switch (status_) {
  case VersionStatus::Alpha:
    setIconName("dialog-warning");
    setLabel("Warning: alpha version");
    infoDialog_.setIcon(QMessageBox::Warning);
    infoDialog_.setText(QString("<p>Warning: You are using an alpha version of Crystal Dock. Please use the latest release instead:")
                        + "<p><a href=\"https://github.com/dangvd/crystal-dock/releases\">https://github.com/dangvd/crystal-dock/releases</a>");
    break;
  case VersionStatus::Beta:
    setIconName("dialog-warning");
    setLabel("Warning: beta version");
    infoDialog_.setIcon(QMessageBox::Warning);
    infoDialog_.setText(QString("<p>Warning: You are using a beta version of Crystal Dock. Please use the latest release instead:")
                        + "<p><a href=\"https://github.com/dangvd/crystal-dock/releases\">https://github.com/dangvd/crystal-dock/releases</a>");
    break;
  case VersionStatus::OutOfDate:
    setIconName("dialog-warning");
    setLabel("Warning: out-of-date version");
    infoDialog_.setIcon(QMessageBox::Warning);
    infoDialog_.setText(QString("<p>Warning: You are using an out-of-date version of Crystal Dock. Please use the latest release instead:")
                        + "<p><a href=\"https://github.com/dangvd/crystal-dock/releases\">https://github.com/dangvd/crystal-dock/releases</a>");
    break;
  case VersionStatus::UpToDate:
    setIconName("dialog-ok");
    setLabel("Up-to-date version");
    infoDialog_.setIcon(QMessageBox::Information);
    infoDialog_.setText(QString("<p>You are using the latest release of Crystal Dock."));
    break;
  }
}

void VersionChecker::createMenu() {
  menu_.addSection("Version Checker");
  auto* frequencyMenu = menu_.addMenu("Checking Frequency");
  auto* frequencyGroup = new QActionGroup(this);
  auto* hourlyAction = frequencyMenu->addAction("Hourly", this, [this](){
    if (status_ == VersionStatus::UpToDate) {
      timerInterval_ = 60 * 60 * 1000;
      timer_->start(timerInterval_);
    }
  });
  hourlyAction->setCheckable(true);
  hourlyAction->setActionGroup(frequencyGroup);
  hourlyAction->setChecked(true);

  auto* dailyAction = frequencyMenu->addAction("Daily", this, [this](){
    if (status_ == VersionStatus::UpToDate) {
      timerInterval_ = 24 * 60 * 60 * 1000;
      timer_->start(timerInterval_);
    }
  });
  dailyAction->setCheckable(true);
  dailyAction->setActionGroup(frequencyGroup);

  menu_.addSeparator();
  parent_->addPanelSettings(&menu_);
}

void VersionChecker::showVersionInfo() {
  infoDialog_.exec();
}

}
