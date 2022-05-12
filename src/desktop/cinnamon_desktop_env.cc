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

#include "cinnamon_desktop_env.h"

#include <QProcess>

namespace crystaldock {

std::vector<Category> CinnamonDesktopEnv::getApplicationMenuSystemCategories() const {
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
        "cinnamon-session-quit --logout",
        ""},
      }
    },
    {"Power", "Power", "system-shutdown", {
      {"Reboot",
        "",
        "system-reboot",
        "cinnamon-session-quit --reboot",
        ""},
      {"Shut Down",
        "",
        "system-shutdown",
        "cinnamon-session-quit --power-off",
        ""}
      }
    }
  };

  return kSystemCategories;
}

std::vector<QString> CinnamonDesktopEnv::getDefaultLaunchers() const {
  return {"org.gnome.Terminal.desktop", "nemo.desktop", "cinnamon-settings.desktop"};
}

bool CinnamonDesktopEnv::setWallpaper(int screen, const QString& wallpaper) {
  // Cinnamon doesn't support setting different wallpapers for different screens.
  return QProcess::startDetached("gsettings", {"set", "org.gnome.desktop.background", "picture-uri",
                                               "file://" + wallpaper});
}

}  // namespace crystaldock
