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

#ifndef CRYSTALDOCK_GNOME_DESKTOP_ENV_H_
#define CRYSTALDOCK_GNOME_DESKTOP_ENV_H_

#include "desktop_env.h"

namespace crystaldock {

class GnomeDesktopEnv : public DesktopEnv {
 public:
  std::vector<Category> getApplicationMenuSystemCategories() const override;

  std::vector<QString> getDefaultLaunchers() const override;

  bool setWallpaper(int screen, const QString& wallpaper) override;
};

}  // namespace crystaldock

#endif // CRYSTALDOCK_GNOME_DESKTOP_ENV_H_

