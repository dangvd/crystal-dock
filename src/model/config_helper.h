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

#ifndef CRYSTALDOCK_CONFIG_HELPER_H_
#define CRYSTALDOCK_CONFIG_HELPER_H_

#include <tuple>
#include <vector>

#include <QDir>

namespace crystaldock {

// Helper class for working with configurations.
class ConfigHelper  {
 public:
  // Individual dock configs.
  static constexpr char kConfigPattern[] = "panel_*.conf";

  // Global appearance config.
  static constexpr char kAppearanceConfig[] = "appearance.conf";

  explicit ConfigHelper(const QString& configDir);
  ~ConfigHelper() = default;

  // Gets the appearance config file path.
  QString appearanceConfigPath() const {
    return configDir_.filePath(kAppearanceConfig);
  }

  static QString wallpaperConfigKey(std::string_view desktopId, int screen) {
    // Screen is 0-based.
    return QString("wallpaper") + QString::fromStdString(std::string(desktopId)) +
        ((screen == 0) ? "" : (QString("_") + QString::number(screen + 1)));
  }

  // Finds the configs of all existing docks.
  // Returns a list of a tuple of <dock config path, dock launchers path>.
  std::vector<std::tuple<QString, QString>> findAllDockConfigs() const;

  // Finds the next available configs for a new dock.
  std::tuple<QString, QString> findNextDockConfigs() const;

  // Copies a launchers directory.
  static void copyLaunchersDir(const QString& launchersDir,
                               const QString& newLaunchersDir);

  // Removes a launchers directory.
  static void removeLaunchersDir(const QString& launchersDir);

 private:
  // Gets the config file name of a dock.
  static QString dockConfigFile(int fileId) {
    return QString("panel_") + QString::number(fileId) + ".conf";
  }

  // Gets the config file path of a dock.
  QString dockConfigPath(int fileId) const {
    return configDir_.filePath(dockConfigFile(fileId));
  }

  QString dockConfigPath(QString configFile) const {
    return configDir_.filePath(configFile);
  }

  static QString dockLaunchersDir(int fileId) {
    return QString("panel_") + QString::number(fileId) + "_launchers";
  }

  // Gets the launchers dir path of a dock.
  QString dockLaunchersPath(int fileId) const {
    return configDir_.filePath(dockLaunchersDir(fileId));
  }

  QString dockLaunchersPathForConfigFile(const QString& configFile) const {
    QString launchersDir = configFile;
    launchersDir.replace(".conf", "_launchers");
    return configDir_.filePath(launchersDir);
  }

  QDir configDir_;

  friend class MultiDockModelTest;
};

}  // namespace crystaldock

#endif  // CRYSTALDOCK_CONFIG_HELPER_H_
