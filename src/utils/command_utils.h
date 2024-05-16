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

#ifndef CRYSTALDOCK_COMMAND_UTILS_H_
#define CRYSTALDOCK_COMMAND_UTILS_H_

#include <vector>

#include <QDir>
#include <QString>
#include <QStringList>

namespace crystaldock {

inline QString filterFieldCodes(const QString& command) {
  QString filtered = command.contains('%')
      ? command.left(command.indexOf('%') - 1)
      : command;

  if (filtered.startsWith("env")) {
    int i = filtered.lastIndexOf("=");
    filtered = filtered.mid(filtered.indexOf(" ", i) + 1);
  }

  return filtered;
}

// Returns a command in the list that exists in the system.
inline QString commandExists(std::vector<const char*> commands) {
  QStringList paths = qEnvironmentVariable("PATH").split(":", Qt::SkipEmptyParts);
  for (const auto& path : paths) {
    QDir dir(path);
    for (const auto& command : commands) {
      if (dir.exists(command)) {
        return command;
      }
    }
  }
  return QString();
}

}  // namespace crystaldock

#endif  // CRYSTALDOCK_COMMAND_UTILS_H_
