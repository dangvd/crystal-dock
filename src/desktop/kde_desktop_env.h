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

#ifndef CRYSTALDOCK_KDE_DESKTOP_ENV_H_
#define CRYSTALDOCK_KDE_DESKTOP_ENV_H_

#include "desktop_env.h"

#include <QDBusInterface>

namespace crystaldock {

class KdeDesktopEnv : public DesktopEnv {
 public:
  KdeDesktopEnv();

  QString getApplicationMenuIcon() const override { return "start-here-kde"; }

  std::vector<Category> getApplicationMenuSystemCategories() const override;

  const ApplicationEntry* getApplicationMenuSearchEntry() const override;

  std::vector<QString> getDefaultLaunchers() const override;

  bool supportSeparateSreenWallpapers() const override { return true; }

  bool setWallpaper(int screen, const QString& wallpaper) override;

 private:
  QDBusInterface plasmaShellDBus_;
};

}  // namespace crystaldock

#endif // CRYSTALDOCK_KDE_DESKTOP_ENV_H_
