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
  // [Deprecated] single-dock configs.
  // crystaldockrc will be renamed to crystaldockrc.old when it is converted
  // to the new multi-dock configs.
  static constexpr char kSingleDockConfig[] = "crystaldockrc";
  static constexpr char kSingleDockOldConfig[] = "crystaldockrc.old";
  static constexpr char kSingleDockLaunchers[] = "launchers";

  // Individual dock configs.
  static constexpr char kConfigPattern[] = "panel_*.conf";

  // Global appearance config.
  static constexpr char kAppearanceConfig[] = "appearance.conf";

  // Global icon override rules (for task manager).
  static constexpr char kIconOverrideRules[] = "icon_override.rules";

  explicit ConfigHelper(const QString& configDir);
  ~ConfigHelper() = default;

  // Gets the appearance config file path.
  QString appearanceConfigPath() const {
    return configDir_.filePath(kAppearanceConfig);
  }

  // Gets the icon override rules file path.
  QString iconOverrideRulesPath() const {
    return configDir_.filePath(kIconOverrideRules);
  }

  static QString wallpaperConfigKey(int desktop, int screen) {
    // Screen is 0-based.
    return QString("wallpaper") + QString::number(desktop) +
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

  // For conversion from old single-dock config to the new multi-dock config.

  // Whether this is a old version of Crystal Dock using single-dock config.
  bool isSingleDockConfig() const {
    return configDir_.exists(kSingleDockConfig) &&
        !configDir_.exists(kAppearanceConfig);
  }

  // Gets the config file path of the old single-dock config.
  QString singleDockConfigPath() const {
    return configDir_.filePath(kSingleDockConfig);
  }

  // Gets the launchers dir path of the old single-dock config.
  QString singleDockLaunchersPath() const {
    return configDir_.filePath(kSingleDockLaunchers);
  }

  // Used when conversion from single-dock config to multi-dock config.
  QString dockConfigPathFromSingleDock() const {
    return dockConfigPath(1);
  }

  // Renames single-dock configs. Is meant to be called after conversion to
  // multi-dock config has been done.
  void renameSingleDockConfigs() {
    configDir_.rename(kSingleDockConfig, kSingleDockOldConfig);
    configDir_.rename(kSingleDockLaunchers, dockLaunchersDir(1));
  }

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
