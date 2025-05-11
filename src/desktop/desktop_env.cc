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

#include "desktop_env.h"

#include <memory>

#include <QtGlobal>
#include <QProcess>
#include <QStringList>

#include "hyprland_desktop_env.h"
#include "kde_desktop_env.h"
#include "labwc_desktop_env.h"
#include "lxqt_desktop_env.h"
#include "wayfire_desktop_env.h"
#include <model/application_menu_config.h>
#include <model/multi_dock_model.h>

namespace crystaldock {

DesktopEnv* DesktopEnv::getDesktopEnv() {
  QString currentDesktopEnv = getDesktopEnvName();
  if (currentDesktopEnv == "Hyprland") {
    static std::unique_ptr<HyprlandDesktopEnv> hyprland(new HyprlandDesktopEnv);
    return hyprland.get();
  } else if (currentDesktopEnv == "KDE") {
    static std::unique_ptr<KdeDesktopEnv> kde(new KdeDesktopEnv);
    return kde.get();
  } else if (currentDesktopEnv == "labwc") {
    static std::unique_ptr<LabwcDesktopEnv> labwc(new LabwcDesktopEnv);
    return labwc.get();
  } else if (currentDesktopEnv == "LXQt") {
    static std::unique_ptr<LxqtDesktopEnv> lxqt(new LxqtDesktopEnv);
    return lxqt.get();
  } else if (currentDesktopEnv == "Wayfire") {
    static std::unique_ptr<WayfireDesktopEnv> wayfire(new WayfireDesktopEnv);
    return wayfire.get();
  }

  static std::unique_ptr<DesktopEnv> generic(new DesktopEnv);
  return generic.get();
}

QString DesktopEnv::getDesktopEnvName() {
  QStringList desktops = qEnvironmentVariable("XDG_CURRENT_DESKTOP").split(",", Qt::SkipEmptyParts);
  // The format is usually something like 'KDE' or 'labwc:wlroots'.
  constexpr char kGenericDesktop[] = "generic";
  return desktops.isEmpty() ? kGenericDesktop
                            : desktops.first().mid(0, desktops.first().indexOf(':'));
}

std::vector<QString> DesktopEnv::getDefaultLaunchers() const {
  return { defaultWebBrowser() };
}

QString DesktopEnv::defaultWebBrowser() const {
  QProcess process;
  process.start("xdg-settings", {"get", "default-web-browser"});
  process.waitForFinished(1000 /*msecs*/);
  QString desktopFile = process.readAllStandardOutput().trimmed();
  auto appId = desktopFile.first(desktopFile.lastIndexOf('.'));
  return !appId.isEmpty() ? appId : "firefox";
}

}  // namespace crystaldock
