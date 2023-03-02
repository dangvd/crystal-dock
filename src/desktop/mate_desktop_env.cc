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

#include "mate_desktop_env.h"

#include <QProcess>

namespace crystaldock {

std::vector<Category> MateDesktopEnv::getApplicationMenuSystemCategories() const {
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
        "mate-session-save --logout",
        ""},
      }
    },
    {"Power", "Power", "system-shutdown", {
      {"Shut Down",
        "",
        "system-shutdown",
        "mate-session-save --shutdown-dialog",
        ""}
      }
    }
  };

  return kSystemCategories;
}

std::vector<QString> MateDesktopEnv::getDefaultLaunchers() const {
  return {"mate-terminal.desktop", "caja-browser.desktop", "matecc.desktop"};
}

bool MateDesktopEnv::setWallpaper(int screen, const QString& wallpaper) {
  // MATE doesn't support setting different wallpapers for different screens.
  return QProcess::startDetached("gsettings", {"set", "org.mate.background", "picture-filename",
                                               "file://" + wallpaper});
}

}  // namespace crystaldock
