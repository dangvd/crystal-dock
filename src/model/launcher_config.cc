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

#include "launcher_config.h"

#include <QFileInfo>

#include "utils/desktop_file.h"

namespace crystaldock {

LauncherConfig::LauncherConfig(const QString& desktopFile) {
  DesktopFile entry(desktopFile);
  appId = QFileInfo(desktopFile).completeBaseName();
  name = entry.name();
  icon = entry.icon();
  command = filterFieldCodes(entry.exec());
}

void LauncherConfig::saveToFile(const QString &filePath) const {
  DesktopFile desktopFile;
  desktopFile.setName(name);
  desktopFile.setIcon(icon);
  desktopFile.setExec(command);
  desktopFile.setType("Application");
  desktopFile.write(filePath + "/" + appId + ".desktop");
}

}  // namespace crystaldock
