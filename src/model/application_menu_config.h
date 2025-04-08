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

#ifndef CRYSTALDOCK_APPLICATION_MENU_CONFIG_H_
#define CRYSTALDOCK_APPLICATION_MENU_CONFIG_H_

#include <list>
#include <string>
#include <unordered_map>
#include <vector>

#include <QDir>
#include <QEvent>
#include <QFileSystemWatcher>
#include <QObject>
#include <QString>
#include <QStringList>

#include "application_menu_entry.h"
#include <desktop/desktop_env.h>

namespace crystaldock {

class ApplicationMenuConfig : public QObject {
  Q_OBJECT

 public:
  ApplicationMenuConfig(const QStringList& entryDirs = getEntryDirs());

  ~ApplicationMenuConfig() = default;

  static QStringList getEntryDirs();

  const std::vector<Category>& categories() const { return categories_; }
  const std::vector<Category>& systemCategories() const { return systemCategories_; }

  // Finds the application entry given the application ID.
  // Will match with each of App ID, WM Class and Name in the entry list in that order.
  const ApplicationEntry* findApplication(const std::string& appId) const;

  bool isAppMenuEntry(const std::string& appId) const {
    return entries_.count(appId) > 0;
  }

  // Tries to find a matching application ID using different heuristics.
  const ApplicationEntry* tryMatchingApplicationId(const std::string& appId) const;

  // Searches for applications with the name containing the given text.
  const std::vector<ApplicationEntry> searchApplications(
      const QString& text, unsigned int maxNumResults) const;

 signals:
  void configChanged();

 public slots:
  void reload();

 private:
  // Initializes application categories.
  void initCategories();

  // Initializes system categories.
  void initSystemCategories();

  // Clears all application entries.
  void clearEntries();

  // Loads application entries from entryDir.
  bool loadEntries();

  // Loads an application entry from the .desktop file.
  bool loadEntry(const QString& file);

  // The directories that contains the list of all application entries as
  // desktop files, e.g. /usr/share/applications
  const QStringList entryDirs_;

  // Application entries, organized by categories.
  std::vector<Category> categories_;
  // System entries (e.g. Lock Screen / Shut Down), organized by categories.
  std::vector<Category> systemCategories_;
  // Map from category names to category indices in the above vector,
  // to make loading entries faster.
  std::unordered_map<std::string, int> categoryMap_;
  // Map from app ids to application entries for fast look-up.
  std::unordered_map<std::string, const ApplicationEntry*> entries_;
  // Map from short commands to application entries for fast look-up.
  // Short command means for example, "command" instead of "/usr/bin/command -a -b"
  std::unordered_map<std::string, const ApplicationEntry*> commands_;
  // Map from WM classes to application entries for fast look-up.
  std::unordered_map<std::string, const ApplicationEntry*> wmClasses_;
  // Map from names to application entries for fast look-up.
  std::unordered_map<std::string, const ApplicationEntry*> names_;

  QFileSystemWatcher fileWatcher_;

  DesktopEnv* desktopEnv_;

  friend class ApplicationMenuConfigTest;
};

}

#endif  // CRYSTALDOCK_APPLICATION_MENU_CONFIG_H_
