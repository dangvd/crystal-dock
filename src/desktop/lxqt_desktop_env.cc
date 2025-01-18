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

#include "lxqt_desktop_env.h"

#include <QProcess>

namespace crystaldock {

std::vector<Category> LxqtDesktopEnv::getApplicationMenuSystemCategories() const {
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
        "lxqt-leave --logout",
        ""},
      }
    },
    {"Power", "Power", "system-shutdown", {
      {"reboot",
        "Reboot",
        "",
        "system-reboot",
        "lxqt-leave --reboot",
        ""},
      {"shutdown",
        "Shut Down",
        "",
        "system-shutdown",
        "lxqt-leave --shutdown",
        ""}
      }
    }
  };

  return kSystemCategories;
}

std::vector<QString> LxqtDesktopEnv::getDefaultLaunchers() const {
  return {"qterminal", "pcmanfm-qt", "lxqt-config"};
}

bool LxqtDesktopEnv::setWallpaper(int screen, const QString& wallpaper) {
  // LXQt doesn't support setting different wallpapers for different screens.
  return QProcess::startDetached("pcmanfm-qt", {"--set-wallpaper=" + wallpaper});
}

}  // namespace crystaldock
