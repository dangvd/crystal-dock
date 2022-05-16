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

#ifndef CRYSTALDOCK_DESKTOP_ENV_H_
#define CRYSTALDOCK_DESKTOP_ENV_H_

#include <vector>

#include <model/application_menu_entry.h>

namespace crystaldock {

// Desktop Environment specific data/logic.
// Currently supports KDE, GNOME, XFCE, LXQT, Cinnamon, MATE.
class DesktopEnv {
 protected:
  DesktopEnv() = default;

 public:
  static DesktopEnv* getDesktopEnv();
  static QString getDesktopEnvName();

  virtual QString getApplicationMenuIcon() const { return "start-here"; }

  // System categories (e.g. Session/Power) on the Application Menu.
  virtual std::vector<Category> getApplicationMenuSystemCategories() const { return {}; }

  // Desktop search entry on the Application Menu, if available.
  // For example, on KDE it should point to krunner.
  virtual const ApplicationEntry* getApplicationMenuSearchEntry() const { return nullptr; }

  // Default desktop environment-specific launchers
  // e.g. File Manager, Console, System Settings.
  // Returns a list of desktop files.
  virtual std::vector<QString> getDefaultLaunchers() const { return {}; };

  // Sets the wallpaper for the current desktop for the specified screen only.
  // Args:
  //   screen: screen to set wallpaper.
  //   wallpaper: path to the wallpaper file.
  virtual bool setWallpaper(int screen, const QString& wallpaper) { return false; }
};

}  // namespace crystaldock

#endif  // CRYSTALDOCK_DESKTOP_ENV_H_
