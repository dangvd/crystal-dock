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

#include "xfce_desktop_env.h"

namespace crystaldock {

std::vector<Category> XfceDesktopEnv::getApplicationMenuSystemCategories() const {
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
        "xfce4-session-logout --logout",
        ""},
      }
    },
    {"Power", "Power", "system-shutdown", {
      {"Reboot",
        "",
        "system-reboot",
        "xfce4-session-logout --reboot",
        ""},
      {"Shut Down",
        "",
        "system-shutdown",
        "xfce4-session-logout --halt",
        ""}
      }
    }
  };

  return kSystemCategories;
}

std::vector<QString> XfceDesktopEnv::getDefaultLaunchers() const {
  return {"xfce4-terminal-emulator.desktop", "thunar.desktop",
      "xfce-settings-manager.desktop"};
}

}  // namespace crystaldock
