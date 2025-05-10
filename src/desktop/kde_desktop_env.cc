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

#include <model/multi_dock_model.h>
#include <utils/command_utils.h>

namespace crystaldock {

KdeDesktopEnv::KdeDesktopEnv()
  : plasmaShellDBus_("org.kde.plasmashell",
                     "/PlasmaShell",
                     "org.kde.PlasmaShell") {
  qdbusCommand_ = commandExists({"qdbus", "qdbus6", "qdbus-qt6"});
  if (qdbusCommand_.size() == 0) {
    std::cerr << "Could not find QDBus command. Certain functionalities will not work." << std::endl;
  }
}

std::vector<Category> KdeDesktopEnv::getApplicationMenuSystemCategories() const {
  static const std::vector<Category> kSystemCategories = {
    {"Session", "Session", "system-log-out", {
      {"lock-screen",
        "Lock Screen",
        "",
        "system-lock-screen",
        "xdg-screensaver lock",
        ""},
      {"log-out",
        "Log Out",
        "",
        "system-log-out",
        qdbusCommand_ + " org.kde.LogoutPrompt /LogoutPrompt promptLogout",
        ""},
      }
    },
    {"Power", "Power", "system-shutdown", {
      {"reboot",
        "Reboot",
        "",
        "system-reboot",
        qdbusCommand_ + " org.kde.LogoutPrompt /LogoutPrompt promptReboot",
        ""},
      {"shutdown",
        "Shut Down",
        "",
        "system-shutdown",
        qdbusCommand_ + " org.kde.LogoutPrompt /LogoutPrompt promptShutDown",
        ""}
      }
    }
  };

  return kSystemCategories;
}

std::vector<QString> KdeDesktopEnv::getDefaultLaunchers() const {
  return { kShowDesktopId, defaultWebBrowser(),
           "org.kde.konsole", "org.kde.dolphin", kSeparatorId,
           "systemsettings", "shutdown" };
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
