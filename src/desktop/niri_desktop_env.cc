/*
 * This file is part of Crystal Dock.
 * Copyright (C) 2025 Viet Dang (dangvd@gmail.com)
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

#include "niri_desktop_env.h"

#include <model/multi_dock_model.h>

namespace crystaldock {

std::vector<Category> NiriDesktopEnv::getApplicationMenuSystemCategories() const {
  static const std::vector<Category> kSystemCategories = {
    {"Session", "Session", "system-log-out",
      {
        {kLockScreenId,
          "Lock Screen",
          "",
          "system-lock-screen",
          "swaylock",
          ""
        },
        {kLogOutId,
          "Log Out",
          "",
          "system-log-out",
          "niri msg action quit",
          ""
        },
      },
    },
  };

  return kSystemCategories;
}

std::vector<QString> NiriDesktopEnv::getDefaultLaunchers() const {
  return { defaultWebBrowser(), "alacritty",
           kSeparatorId, kLockScreenId, kLogOutId, kSeparatorId };
}

}  // namespace crystaldock
