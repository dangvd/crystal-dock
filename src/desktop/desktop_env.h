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

#include <QString>

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

  virtual ~DesktopEnv() = default;

  virtual QString getApplicationMenuIcon() const { return "start-here"; }

  // System categories (e.g. Session/Power) on the Application Menu.
  virtual std::vector<Category> getApplicationMenuSystemCategories() const { return {}; }

  // Default desktop environment-specific launchers
  // e.g. File Manager, Console, System Settings.
  // Returns a list of desktop files.
  virtual std::vector<QString> getDefaultLaunchers() const { return {}; };

  // Supports separate wallpapers for separate screens.
  virtual bool supportSeparateSreenWallpapers() const { return false; }

  // Sets the wallpaper for the current desktop for the specified screen.
  // If the desktop environment does not support separate wallpapers for
  // separate screens, this simply sets the wallpaper for the current desktop
  // for all screens.
  // Args:
  //   screen: screen to set wallpaper for.
  //   wallpaper: path to the wallpaper file.
  virtual bool setWallpaper(int screen, const QString& wallpaper) { return false; }
};

}  // namespace crystaldock

#endif  // CRYSTALDOCK_DESKTOP_ENV_H_
