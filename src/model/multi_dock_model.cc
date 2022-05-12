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

#include "multi_dock_model.h"

#include <iostream>

#include <QProcess>

#include <KWindowSystem>

#include <utils/command_utils.h>

namespace crystaldock {

constexpr char MultiDockModel::kGeneralCategory[];
constexpr char MultiDockModel::kAutoHide[];
constexpr char MultiDockModel::kPosition[];
constexpr char MultiDockModel::kScreen[];
constexpr char MultiDockModel::kShowApplicationMenu[];
constexpr char MultiDockModel::kShowClock[];
constexpr char MultiDockModel::kShowPager[];
constexpr char MultiDockModel::kShowTaskManager[];
constexpr char MultiDockModel::kVisibility[];

constexpr char MultiDockModel::kBackgroundColor[];
constexpr char MultiDockModel::kBorderColor[];
constexpr char MultiDockModel::kMaximumIconSize[];
constexpr char MultiDockModel::kMinimumIconSize[];
constexpr char MultiDockModel::kSpacingFactor[];
constexpr char MultiDockModel::kShowBorder[];
constexpr char MultiDockModel::kTooltipFontSize[];
constexpr char MultiDockModel::kApplicationMenuCategory[];
constexpr char MultiDockModel::kIcon[];
constexpr char MultiDockModel::kLabel[];
constexpr char MultiDockModel::kStrut[];
constexpr char MultiDockModel::kPagerCategory[];
constexpr char MultiDockModel::kWallpaper[];
constexpr char MultiDockModel::kShowDesktopNumber[];
constexpr char MultiDockModel::kTaskManagerCategory[];
constexpr char MultiDockModel::kCurrentDesktopTasksOnly[];
constexpr char MultiDockModel::kCurrentScreenTasksOnly[];
constexpr char MultiDockModel::kClockCategory[];
constexpr char MultiDockModel::kUse24HourClock[];
constexpr char MultiDockModel::kFontScaleFactor[];

MultiDockModel::MultiDockModel(const QString& configDir)
    : configHelper_(configDir),
      appearanceConfig_(configHelper_.appearanceConfigPath(),
                        QSettings::IniFormat),
      desktopEnv_(DesktopEnv::getDesktopEnv()) {
  loadDocks();
  connect(&applicationMenuConfig_, SIGNAL(configChanged()),
          this, SIGNAL(applicationMenuConfigChanged()));
}

void MultiDockModel::loadDocks() {
  // Dock ID starts from 1.
  int dockId = 1;
  dockConfigs_.clear();
  for (const auto& configs : configHelper_.findAllDockConfigs()) {
    const auto& configPath = std::get<0>(configs);
    const auto& launchersPath = std::get<1>(configs);
    dockConfigs_[dockId] = std::make_tuple(
        configPath,
        std::make_unique<QSettings>(configPath, QSettings::IniFormat),
        launchersPath,
        loadDockLaunchers(launchersPath));
    ++dockId;
  }
  nextDockId_ = dockId;
}

void MultiDockModel::addDock(PanelPosition position, int screen,
                             bool showApplicationMenu, bool showPager,
                             bool showTaskManager, bool showClock) {
  auto configs = configHelper_.findNextDockConfigs();
  auto dockId = addDock(configs, position, screen);
  setVisibility(dockId, kDefaultVisibility);
  setShowApplicationMenu(dockId, showApplicationMenu);
  setShowPager(dockId, showPager);
  setShowTaskManager(dockId, showTaskManager);
  setShowClock(dockId, showClock);
  emit dockAdded(dockId);

  if (dockCount() == 1) {
    setMinIconSize(kDefaultMinSize);
    setMaxIconSize(kDefaultMaxSize);
    QColor color(kDefaultBackgroundColor);
    color.setAlphaF(kDefaultBackgroundAlpha);
    setBackgroundColor(color);
    setShowBorder(kDefaultShowBorder);
    setBorderColor(QColor(kDefaultBorderColor));
    setTooltipFontSize(kDefaultTooltipFontSize);

    setApplicationMenuName(kDefaultApplicationMenuName);
    setApplicationMenuIcon(kDefaultApplicationMenuIcon);

    setUse24HourClock(kDefaultUse24HourClock);
    setClockFontScaleFactor(kDefaultClockFontScaleFactor);

    syncAppearanceConfig();
  }
  syncDockConfig(dockId);
  syncDockLaunchersConfig(dockId);
}

int MultiDockModel::addDock(const std::tuple<QString, QString>& configs,
                            PanelPosition position, int screen) {
  const auto dockId = nextDockId_;
  ++nextDockId_;
  const auto& configPath = std::get<0>(configs);
  const auto& launchersPath = std::get<1>(configs);
  dockConfigs_[dockId] = std::make_tuple(
      configPath,
      std::make_unique<QSettings>(configPath, QSettings::IniFormat),
      launchersPath,
      loadDockLaunchers(launchersPath));
  setPanelPosition(dockId, position);
  setScreen(dockId, screen);

  return dockId;
}

void MultiDockModel::cloneDock(int srcDockId, PanelPosition position,
                               int screen) {
  auto configs = configHelper_.findNextDockConfigs();

  // Clone the dock config and launchers.
  QFile::copy(dockConfigPath(srcDockId), std::get<0>(configs));
  ConfigHelper::copyLaunchersDir(dockLaunchersPath(srcDockId),
                                 std::get<1>(configs));

  auto dockId = addDock(configs, position, screen);
  emit dockAdded(dockId);

  syncDockConfig(dockId);
  syncDockLaunchersConfig(dockId);
}

void MultiDockModel::removeDock(int dockId) {
  QFile::remove(dockConfigPath(dockId));
  ConfigHelper::removeLaunchersDir(dockLaunchersPath(dockId));
  dockConfigs_.erase(dockId);
  // No need to emit a signal here.
}

bool MultiDockModel::hasPager() const {
  for (const auto& dock : dockConfigs_) {
    if (showPager(dock.first)) {
      return true;
    }
  }
  return false;
}

void MultiDockModel::syncDockLaunchersConfig(int dockId) {
  const auto& launchersPath = dockLaunchersPath(dockId);
  QDir launchersDir(launchersPath);
  if (launchersDir.exists()) {
    QStringList files = launchersDir.entryList(QDir::Files, QDir::Name);
    for (int i = 0; i < files.size(); ++i) {
      launchersDir.remove(files.at(i));
    }
  } else {
    QDir::root().mkpath(launchersPath);
  }

  int launcherId = 1;
  for (const auto& item : dockLauncherConfigs(dockId)) {
    item.saveToFile(QString("%1/%2 - %3.desktop")
        .arg(launchersPath)
        .arg(launcherId, 2, 10, QChar('0'))
        .arg(item.name));
    ++launcherId;
  }
}

std::vector<LauncherConfig> MultiDockModel::loadDockLaunchers(
    const QString& dockLaunchersPath) {
  QDir launchersDir(dockLaunchersPath);
  QStringList files = launchersDir.entryList({"*.desktop"}, QDir::Files,
                                             QDir::Name);
  if (files.empty()) {
    return createDefaultLaunchers();
  }

  std::vector<LauncherConfig> launchers;
  launchers.reserve(files.size());
  for (int i = 0; i < files.size(); ++i) {
    const QString& desktopFile = dockLaunchersPath + "/" + files.at(i);
    launchers.push_back(LauncherConfig(desktopFile));
  }

  return launchers;
}

std::vector<LauncherConfig> MultiDockModel::createDefaultLaunchers() {
  std::vector<LauncherConfig> launchers;

  auto* defaultWebBrowser = getDefaultBrowser();
  if (defaultWebBrowser) {
    launchers.push_back(
        LauncherConfig(defaultWebBrowser->name, defaultWebBrowser->icon, defaultWebBrowser->command));
  } else {
    const auto* entry = applicationMenuConfig_.findApplicationFromFile(
          QString("firefox.desktop"));
    if (entry != nullptr) {
      launchers.push_back(LauncherConfig(entry->name, entry->icon, entry->command));
    }
  }

  auto desktopEnvItems = desktopEnv_->getDefaultLaunchers();
  launchers.reserve(desktopEnvItems.size() + 3);
  for (const auto& file : desktopEnvItems) {
    const auto* entry = applicationMenuConfig_.findApplicationFromFile(file);
    if (entry != nullptr) {
      launchers.push_back(LauncherConfig(entry->name, entry->icon, entry->command));
    }
  }

  launchers.push_back(
      LauncherConfig("Separator", "", "SEPARATOR"));
  launchers.push_back(
      LauncherConfig("Lock Screen", "system-lock-screen", "xdg-screensaver lock"));

  return launchers;
}

const ApplicationEntry* MultiDockModel::getDefaultBrowser() {
  QProcess process;
  process.start("xdg-settings", {"get", "default-web-browser"});
  process.waitForFinished(1000 /*msecs*/);
  QString browserEntry = process.readAllStandardOutput().trimmed();
  return applicationMenuConfig_.findApplicationFromFile(browserEntry);
}

}  // namespace crystaldock
