/*
 * This file is part of Crystal Dock.
 * Copyright (C) 2026 Crystal Dock contributors
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

#include "cosmic_desktop_env.h"

#include <QIcon>

#include <model/multi_dock_model.h>

namespace crystaldock {

CosmicDesktopEnv::CosmicDesktopEnv() {
  // COSMIC ships its own icon theme ("Cosmic") that includes both COSMIC's
  // own app icons and freedesktop standard action/status icons. Qt running
  // inside a containerized build (or without GTK theme integration) does
  // not detect this automatically and falls back to a hicolor-only search
  // path, which causes system-action icon names ("system-lock-screen",
  // "system-log-out", etc.) to render blank because hicolor only ships
  // app icons. Set the theme explicitly so those names resolve.
  //
  // Icon themes inherit from hicolor for app icons, so existing per-app
  // lookups (e.g. brave-browser, com.system76.CosmicFiles) continue to
  // work after this is set.
  QIcon::setThemeName("Cosmic");
}

QString CosmicDesktopEnv::getApplicationMenuIcon() const {
  // COSMIC ships com.system76.CosmicAppLibrary as the equivalent of an
  // application-launcher icon. Using its reverse-DNS name resolves via
  // hicolor and produces the actual COSMIC App Library icon rather than
  // the freedesktop "start-here" name (which isn't in COSMIC's theme).
  return "com.system76.CosmicAppLibrary";
}

std::vector<Category> CosmicDesktopEnv::getApplicationMenuSystemCategories() const {
  static const std::vector<Category> kSystemCategories = {
    {"Session", "Session", "system-log-out",
      {
        {kLockScreenId,
          "Lock Screen",
          "",
          "system-lock-screen",
          "loginctl lock-session",
          ""
        },
        {kLogOutId,
          "Log Out",
          "",
          "system-log-out",
          "loginctl terminate-user $USER",
          ""
        },
      },
    },
  };

  return kSystemCategories;
}

std::vector<QString> CosmicDesktopEnv::getDefaultLaunchers() const {
  return {
    defaultWebBrowser(),
    "com.system76.CosmicFiles",
    "com.system76.CosmicTerm",
    "com.system76.CosmicEdit",
    "com.system76.CosmicSettings",
    "com.system76.CosmicStore",
    kSeparatorId,
    kLockScreenId,
    kLogOutId,
    kSeparatorId
  };
}

}  // namespace crystaldock
