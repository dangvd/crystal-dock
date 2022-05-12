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

#include "gnome_desktop_env.h"

#include <QProcess>

namespace crystaldock {

std::vector<Category> GnomeDesktopEnv::getApplicationMenuSystemCategories() const {
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
        "gnome-session-quit --logout",
        ""},
      }
    },
    {"Power", "Power", "system-shutdown", {
      {"Reboot",
        "",
        "system-reboot",
        "gnome-session-quit --reboot",
        ""},
      {"Shut Down",
        "",
        "system-shutdown",
        "gnome-session-quit --power-off",
        ""}
      }
    }
  };

  return kSystemCategories;
}

std::vector<QString> GnomeDesktopEnv::getDefaultLaunchers() const {
  return {"org.gnome.Terminal.desktop", "org.gnome.Nautilus.desktop",
          "gnome-control-center.desktop", "org.gnome.tweaks.desktop"};
}

bool GnomeDesktopEnv::setWallpaper(int screen, const QString& wallpaper) {
  // GNOME doesn't support setting different wallpapers for different screens.
  return QProcess::startDetached("gsettings", {"set", "org.gnome.desktop.background", "picture-uri",
                                               "file://" + wallpaper});
}

}  // namespace crystaldock
