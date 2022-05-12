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

#include "config_helper.h"

#include <QFile>
#include <QStringList>

namespace crystaldock {

constexpr char ConfigHelper::kSingleDockConfig[];
constexpr char ConfigHelper::kSingleDockOldConfig[];
constexpr char ConfigHelper::kSingleDockLaunchers[];

constexpr char ConfigHelper::kConfigPattern[];
constexpr char ConfigHelper::kAppearanceConfig[];
constexpr char ConfigHelper::kIconOverrideRules[];

// Creates a seperate config dir for each desktop environment.
ConfigHelper::ConfigHelper(const QString& configDir)
    : configDir_{configDir + "/" + qEnvironmentVariable("XDG_CURRENT_DESKTOP")} {
  if (!configDir_.exists()) {
    QDir::root().mkpath(configDir);
  }
}

std::vector<std::tuple<QString, QString>> ConfigHelper::findAllDockConfigs()
    const {
  std::vector<std::tuple<QString, QString>> allConfigs;
  QStringList files = configDir_.entryList(
      {kConfigPattern}, QDir::Files, QDir::Name);
  if (files.isEmpty()) {
    return allConfigs;
  }

  for (int i = 0; i < files.size(); ++i) {
    const QString& configFile = files.at(i);
    allConfigs.push_back(std::make_tuple(
        dockConfigPath(configFile),
        dockLaunchersPathForConfigFile(configFile)));
  }
  return allConfigs;
}

std::tuple<QString, QString> ConfigHelper::findNextDockConfigs() const {
  for (int fileId = 1; ; ++fileId) {
    if (!configDir_.exists(dockConfigFile(fileId))) {
      return std::make_tuple(dockConfigPath(fileId),
                             dockLaunchersPath(fileId));
    }
  }
}

void ConfigHelper::copyLaunchersDir(const QString& launchersDir,
                                    const QString& newLaunchersDir) {
  QDir::root().mkpath(newLaunchersDir);
  QDir dir(launchersDir);
  QStringList files = dir.entryList({"*.desktop"}, QDir::Files, QDir::Name);
  for (int i = 0; i < files.size(); ++i) {
    const auto srcFile = launchersDir + "/" + files.at(i);
    const auto destFile = newLaunchersDir + "/" + files.at(i);
    QFile::copy(srcFile, destFile);
  }
}

void ConfigHelper::removeLaunchersDir(const QString& launchersDir) {
  QDir dir(launchersDir);
  QStringList files = dir.entryList({"*.desktop"}, QDir::Files, QDir::Name);
  for (int i = 0; i < files.size(); ++i) {
    const auto launcherFile = launchersDir + "/" + files.at(i);
    QFile::remove(launcherFile);
  }
  QDir::root().rmdir(launchersDir);
}

}  // namespace crystaldock
