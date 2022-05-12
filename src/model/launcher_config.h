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

#ifndef CRYSTALDOCK_LAUNCHER_CONFIG_H_
#define CRYSTALDOCK_LAUNCHER_CONFIG_H_

#include <QString>

#include <utils/command_utils.h>

namespace crystaldock {

struct LauncherConfig {
  QString name;
  QString icon;
  QString command;
  QString taskCommand;

  LauncherConfig() = default;
  LauncherConfig(const QString& name2, const QString& icon2,
                 const QString& command2)
      : name(name2), icon(icon2), command(command2),
        taskCommand(getTaskCommand(command)) {}
  LauncherConfig(const QString& desktopFile);

  // Saves to file in desktop file format.
  void saveToFile(const QString& filePath) const;
};

}  // namespace crystaldock

#endif  // CRYSTALDOCK_LAUNCHER_CONFIG_H_
