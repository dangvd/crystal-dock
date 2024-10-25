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

constexpr char MultiDockModel::kGeneralCategory[];
constexpr char MultiDockModel::kAutoHide[];
constexpr char MultiDockModel::kPosition[];
constexpr char MultiDockModel::kScreen[];
constexpr char MultiDockModel::kShowApplicationMenu[];
constexpr char MultiDockModel::kShowClock[];
constexpr char MultiDockModel::kShowPager[];
constexpr char MultiDockModel::kShowTaskManager[];
constexpr char MultiDockModel::kVisibility[];
constexpr char MultiDockModel::kPanelStyle[];

constexpr char MultiDockModel::kBackgroundColor[];
constexpr char MultiDockModel::kBackgroundColor2D[];
constexpr char MultiDockModel::kBackgroundColorMetal2D[];
constexpr char MultiDockModel::kBorderColor[];
constexpr char MultiDockModel::kBorderColorMetal2D[];
constexpr char MultiDockModel::kActiveIndicatorColor[];
constexpr char MultiDockModel::kActiveIndicatorColor2D[];
constexpr char MultiDockModel::kActiveIndicatorColorMetal2D[];
constexpr char MultiDockModel::kInactiveIndicatorColor[];
constexpr char MultiDockModel::kInactiveIndicatorColor2D[];
constexpr char MultiDockModel::kInactiveIndicatorColorMetal2D[];
constexpr char MultiDockModel::kMaximumIconSize[];
constexpr char MultiDockModel::kMinimumIconSize[];
constexpr char MultiDockModel::kSpacingFactor[];
constexpr char MultiDockModel::kShowBorder[];
constexpr char MultiDockModel::kFloatingMargin[];
constexpr char MultiDockModel::kFirstRunMultiScreen[];
constexpr char MultiDockModel::kFirstRunWindowCountIndicator[];

constexpr char MultiDockModel::kTooltipFontSize[];
constexpr char MultiDockModel::kApplicationMenuCategory[];
constexpr char MultiDockModel::kLabel[];
constexpr char MultiDockModel::kFontSize[];
constexpr char MultiDockModel::kBackgroundAlpha[];
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
                             bool showTaskManager, bool showClock) {
  auto configPath = configHelper_.findNextDockConfig();
  auto dockId = addDock(configPath, position, screen);
  setVisibility(dockId, kDefaultVisibility);
  setLaunchers(dockId, defaultLaunchers());
  setShowApplicationMenu(dockId, showApplicationMenu);
  setShowPager(dockId, showPager);
  setShowTaskManager(dockId, showTaskManager);
  setShowClock(dockId, showClock);
  emit dockAdded(dockId);

  if (dockCount() == 1) {
    setMinIconSize(kDefaultMinSize);
    setMaxIconSize(kDefaultMaxSize);
    setSpacingFactor(kDefaultSpacingFactor);
    QColor color(kDefaultBackgroundColor);
    color.setAlphaF(kDefaultBackgroundAlpha);
    setBackgroundColor(color);
    setShowBorder(kDefaultShowBorder);
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
    if (appId == "separator") {
      entries.push_back(LauncherConfig(kSeparatorId, "", "", ""));
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

  auto* defaultWebBrowser = defaultBrowser();
  if (defaultWebBrowser) {
    launchers.append(defaultWebBrowser->appId);
  } else {
    const auto* entry = applicationMenuConfig_.findApplication("firefox");
    if (entry != nullptr) {
      launchers.append("firefox");
    }
  }

  auto desktopEnvItems = desktopEnv_->getDefaultLaunchers();
  launchers.reserve(desktopEnvItems.size() + 3);
  for (const auto& appId : desktopEnvItems) {
    const auto* entry = applicationMenuConfig_.findApplication(appId.toStdString());
    if (entry != nullptr) {
      launchers.append(appId);
    }
  }

  launchers.append("separator");
  launchers.append("lock-screen");

  return launchers;
}

const ApplicationEntry* MultiDockModel::defaultBrowser() {
  QProcess process;
  process.start("xdg-settings", {"get", "default-web-browser"});
  process.waitForFinished(1000 /*msecs*/);
  QString desktopFile = process.readAllStandardOutput().trimmed();
  auto appId = desktopFile.first(desktopFile.lastIndexOf('.'));
  return applicationMenuConfig_.findApplication(appId.toStdString());
}

}  // namespace crystaldock
