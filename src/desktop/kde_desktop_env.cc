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

#include "kde_desktop_env.h"

#include <cstdlib>
#include <iostream>

#include <QDBusMessage>

namespace crystaldock {

KdeDesktopEnv::KdeDesktopEnv()
  : plasmaShellDBus_("org.kde.plasmashell",
                     "/PlasmaShell",
                     "org.kde.PlasmaShell") {
  const char* commands[3] = {"qdbus", "qdbus6", "qdbus-qt5"};
  for (const auto& command : commands) {
    if (system(command) == 0) {
      qdbusCommand_ = command;
    }
  }
  if (qdbusCommand_.size() == 0) {
    std::cerr << "Could not find QDBus command. Certain functionalities will not work." << std::endl;
  }
}

std::vector<Category> KdeDesktopEnv::getApplicationMenuSystemCategories() const {
  static const std::vector<Category> kSystemCategories = {
    {"Session", "Session", "system-log-out", {
      {"Lock Screen",
        "",
        "system-lock-screen",
        "xdg-screensaver lock",
        ""},
      {"Log Out",
        "",
        "system-log-out",
        qdbusCommand_ + " org.kde.ksmserver /KSMServer logout -1 0 3",
        ""},
      }
    },
    {"Power", "Power", "system-shutdown", {
      {"Reboot",
        "",
        "system-reboot",
        qdbusCommand_ + " org.kde.ksmserver /KSMServer logout -1 1 3",
        ""},
      {"Shut Down",
        "",
        "system-shutdown",
        qdbusCommand_ + " org.kde.ksmserver /KSMServer logout -1 2 3",
        ""}
      }
    }
  };

  return kSystemCategories;
}

std::vector<QString> KdeDesktopEnv::getDefaultLaunchers() const {
  return {"org.kde.konsole.desktop", "org.kde.dolphin.desktop",
          "systemsettings.desktop"};
}

bool KdeDesktopEnv::setWallpaper(int screen, const QString& wallpaper) {
  const QDBusMessage response = plasmaShellDBus_.call(
      "evaluateScript",
      "var allDesktops = desktops();"
      "d = allDesktops[" + QString::number(screen) + "];" +
      "d.wallpaperPlugin ='org.kde.image';"
      "d.currentConfigGroup = Array('Wallpaper', 'org.kde.image','General');"
      "d.writeConfig('Image','file://"
      + wallpaper + "')");
  return response.type() != QDBusMessage::ErrorMessage;
}

}  // namespace crystaldock
