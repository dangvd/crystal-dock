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

#ifndef CRYSTALDOCK_APPLICATION_MENU_ENTRY_H_
#define CRYSTALDOCK_APPLICATION_MENU_ENTRY_H_

#include <list>

#include <QString>

namespace crystaldock {

// An application entry in the application menu.
struct ApplicationEntry {
  // App ID e.g. org.kde.dolphin
  QString appId;

  // Name e.g. 'Chrome'.
  QString name;

  // Generic name e.g. 'Web Brower'.
  QString genericName;

  // Icon name e.g. 'chrome'.
  QString icon;

  // Command to execute e.g. '/usr/bin/google-chrome-stable'.
  QString command;

  // The path to the desktop file e.g. '/usr/share/applications/google-chrome.desktop'
  QString desktopFile;

  // If it's hidden, it won't show on the Application Menu.
  bool hidden;

  ApplicationEntry(const QString& appId2, const QString& name2, const QString& genericName2,
                   const QString& icon2, const QString& command2,
                   const QString& desktopFile2, bool hidden2 = false)
      : appId(appId2), name(name2), genericName(genericName2), icon(icon2),
        command(command2), desktopFile(desktopFile2), hidden(hidden2) {}
};

inline bool operator<(const ApplicationEntry &e1, const ApplicationEntry &e2) {
  return e1.name.toLower() < e2.name.toLower();
}

// A category in the application menu.
struct Category {
  // Name for the category e.g. 'Development' or 'Utility'. See:
  // https://specifications.freedesktop.org/menu-spec/latest/apa.html
  QString name;

  // Display name for the category e.g. 'Utilities'.
  QString displayName;

  // Icon name for the category e.g. 'applications-internet'.
  QString icon;

  // Application entries for this category.
  std::list<ApplicationEntry> entries;

  Category(const QString& name2, const QString& displayName2,
           const QString& icon2)
      : name(name2), displayName(displayName2), icon(icon2) {}

  Category(const QString& name2, const QString& displayName2,
           const QString& icon2, std::list<ApplicationEntry> entries2)
      : name(name2), displayName(displayName2), icon(icon2), entries(entries2) {
  }
};

}  // namespace crystaldock

#endif  // CRYSTALDOCK_APPLICATION_MENU_ENTRY_H_
