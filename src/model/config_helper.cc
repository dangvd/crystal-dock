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

#include "desktop/desktop_env.h"

namespace crystaldock {

constexpr char ConfigHelper::kConfigPattern[];
constexpr char ConfigHelper::kAppearanceConfig[];

// Creates a seperate config dir for each desktop environment.
ConfigHelper::ConfigHelper(const QString& configDir)
    : configDir_{configDir + "/" + DesktopEnv::getDesktopEnvName()} {
  if (!configDir_.exists()) {
    QDir::root().mkpath(configDir_.path());
  }
}

std::vector<QString> ConfigHelper::findAllDockConfigs() const {
  std::vector<QString> allConfigs;
  QStringList files = configDir_.entryList(
      {kConfigPattern}, QDir::Files, QDir::Name);
  if (files.isEmpty()) {
    return allConfigs;
  }

  for (int i = 0; i < files.size(); ++i) {
    const QString& configFile = files.at(i);
    allConfigs.push_back(dockConfigPath(configFile));
  }
  return allConfigs;
}

QString ConfigHelper::findNextDockConfig() const {
  for (int fileId = 1; ; ++fileId) {
    if (!configDir_.exists(dockConfigFile(fileId))) {
      return dockConfigPath(fileId);
    }
  }
}

}  // namespace crystaldock
