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

#include <QFileInfo>
#include <QProcess>

#include "display/window_system.h"

namespace crystaldock {

MultiDockModel::MultiDockModel(const QString& configDir)
    : configHelper_(configDir),
      appearanceConfig_(configHelper_.appearanceConfigPath(),
                        QSettings::IniFormat),
      desktopEnv_(DesktopEnv::getDesktopEnv()) {
  loadDocks();
  connect(&applicationMenuConfig_, SIGNAL(configChanged()),
          this, SIGNAL(applicationMenuConfigChanged()));
  if (maxIconSize() < minIconSize()) {
    setMaxIconSize(minIconSize());
  }
  if (firstRunWindowCountIndicator()) {
    setActiveIndicatorColor(kDefaultActiveIndicatorColor);
    setInactiveIndicatorColor(kDefaultInactiveIndicatorColor);
  }
}

void MultiDockModel::loadDocks() {
  // Dock ID starts from 1.
  int dockId = 1;
  dockConfigs_.clear();
  for (const auto& configPath : configHelper_.findAllDockConfigs()) {
    dockConfigs_[dockId] = std::make_tuple(
        configPath,
        std::make_unique<QSettings>(configPath, QSettings::IniFormat));
    if (screen(dockId) < static_cast<int>(WindowSystem::screens().size())) {
      ++dockId;
    } else {  // Invalid screen.
      dockConfigs_.erase(dockId);
    }
  }
  nextDockId_ = dockId;

  maybeAddDockForMultiScreen();
}

void MultiDockModel::addDock(PanelPosition position, int screen,
                             bool showApplicationMenu, bool showPager,
                             bool showTaskManager, bool showTrash,
                             bool showWifiManager, bool showVolumeControl,
                             bool showBatteryIndicator,
                             bool showVersionChecker, bool showClock) {
  auto configPath = configHelper_.findNextDockConfig();
  auto dockId = addDock(configPath, position, screen);
  setVisibility(dockId, kDefaultVisibility);
  setLaunchers(dockId, defaultLaunchers());
  setShowApplicationMenu(dockId, showApplicationMenu);
  setShowPager(dockId, showPager);
  setShowTaskManager(dockId, showTaskManager);
  setShowTrash(dockId, showTrash);
  setShowWifiManager(dockId, showWifiManager);
  setShowVolumeControl(dockId, showVolumeControl);
  setShowBatteryIndicator(dockId, showBatteryIndicator);
  setShowVersionChecker(dockId, showVersionChecker);
  setShowClock(dockId, showClock);
  emit dockAdded(dockId);

  if (dockCount() == 1) {
    setMinIconSize(kDefaultMinSize);
    setMaxIconSize(kDefaultMaxSize);
    setSpacingFactor(kDefaultSpacingFactor);
    QColor color(kDefaultBackgroundColor);
    color.setAlphaF(kDefaultBackgroundAlpha);
    setBackgroundColor(color);
    setBorderColor(QColor(kDefaultBorderColor));
    setTooltipFontSize(kDefaultTooltipFontSize);

    setApplicationMenuName(kDefaultApplicationMenuName);
    setApplicationMenuFontSize(kDefaultApplicationMenuFontSize);
    setApplicationMenuBackgroundAlpha(kDefaultApplicationMenuBackgroundAlpha);

    setUse24HourClock(kDefaultUse24HourClock);
    setClockFontScaleFactor(kDefaultClockFontScaleFactor);

    syncAppearanceConfig();
  }
  syncDockConfig(dockId);
}

int MultiDockModel::addDock(const QString& configPath,
                            PanelPosition position, int screen) {
  const auto dockId = nextDockId_;
  ++nextDockId_;
  dockConfigs_[dockId] = std::make_tuple(
      configPath,
      std::make_unique<QSettings>(configPath, QSettings::IniFormat));
  setPanelPosition(dockId, position);
  setScreen(dockId, screen);

  return dockId;
}

void MultiDockModel::cloneDock(int srcDockId, PanelPosition position,
                               int screen) {
  auto configPath = configHelper_.findNextDockConfig();

  // Clone the dock config.
  QFile::copy(dockConfigPath(srcDockId), configPath);
  auto dockId = addDock(configPath, position, screen);
  emit dockAdded(dockId);

  syncDockConfig(dockId);
}

void MultiDockModel::removeDock(int dockId) {
  QFile::remove(dockConfigPath(dockId));
  dockConfigs_.erase(dockId);
  // No need to emit a signal here.
}

void MultiDockModel::maybeAddDockForMultiScreen() {
  const auto screenCount = static_cast<int>(WindowSystem::screens().size());
  if (screenCount > 1 && dockCount() == 1 && firstRunMultiScreen()) {
    const auto dockId = dockConfigs_.cbegin()->first;
    const auto dockPosition = panelPosition(dockId);
    const auto dockScreen = screen(dockId);
    for (int screen = 0; screen < screenCount; ++screen) {
      if (screen != dockScreen) {
        cloneDock(dockId, dockPosition, screen);
      }
    }
  }
}

bool MultiDockModel::hasPager() const {
  for (const auto& dock : dockConfigs_) {
    if (showPager(dock.first)) {
      return true;
    }
  }
  return false;
}

const std::vector<LauncherConfig> MultiDockModel::launcherConfigs(int dockId) const {
  std::vector<LauncherConfig> entries;
  for (const auto& appId : launchers(dockId)) {
    if (appId == kSeparatorId) {
      entries.push_back(LauncherConfig(kSeparatorId, "", "", ""));
      continue;
    }

    if (appId == kLauncherSeparatorId) {
      entries.push_back(LauncherConfig(kLauncherSeparatorId, "", "", ""));
      continue;
    }

    if (appId == kShowDesktopId) {
      entries.push_back(LauncherConfig(kShowDesktopId, kShowDesktopName, kShowDesktopIcon, ""));
      continue;
    }

    const auto* entry = applicationMenuConfig_.findApplication(appId.toStdString());
    if (entry != nullptr) {
      entries.push_back(LauncherConfig(entry->appId, entry->name, entry->icon,
                                       entry->command));
    }
  }
  return entries;
}

QStringList MultiDockModel::defaultLaunchers() {
  QStringList launchers;
  const auto desktopEnvItems = desktopEnv_->getDefaultLaunchers();
  launchers.reserve(desktopEnvItems.size());
  for (const auto& appId : desktopEnvItems) {
    launchers.append(appId);
  }

  return launchers;
}

}  // namespace crystaldock
