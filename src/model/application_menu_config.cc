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

#include "application_menu_config.h"

#include <algorithm>
#include <iostream>

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QStringBuilder>
#include <QUrl>

#include <utils/command_utils.h>
#include <utils/desktop_file.h>

namespace crystaldock {

namespace {

bool isHidden(const DesktopFile& desktopFile) {
  if (desktopFile.noDisplay() || desktopFile.hidden()) {
    return true;
  }

  QString desktopEnvName = DesktopEnv::getDesktopEnvName();
  // Some desktop files still use the legacy "X-Desktop" name.
  if (!desktopFile.showOnDesktop(desktopEnvName) &&
      !desktopFile.showOnDesktop("X-" + desktopEnvName)) {
    return true;
  }

  // Do not show LXQt special entries (Log Out / Reboot etc.) in the standard categories,
  // as they are already available in special sections (Session / Power).
  if (desktopFile.exec().startsWith("lxqt-leave")) {
    return true;
  }

  if (desktopFile.categories().isEmpty()) {
    return true;
  }

  return false;
}

}  // namespace

ApplicationMenuConfig::ApplicationMenuConfig(const QStringList& entryDirs)
    : entryDirs_(entryDirs),
      fileWatcher_(entryDirs),
      desktopEnv_(DesktopEnv::getDesktopEnv()) {
  initCategories();
  initSystemCategories();
  loadEntries();
  connect(&fileWatcher_, SIGNAL(directoryChanged(const QString&)),
          this, SLOT(reload()));
  connect(&fileWatcher_, SIGNAL(fileChanged(const QString&)),
          this, SLOT(reload()));
}

QStringList ApplicationMenuConfig::getEntryDirs() {
  QStringList entryDirs{QDir::homePath() + "/.local/share/applications"};
  QStringList dataDirs = qEnvironmentVariable("XDG_DATA_DIRS").split(":", Qt::SkipEmptyParts);
  if (dataDirs.empty()) {
    dataDirs.append("/usr/share/");
    dataDirs.append("/usr/local/share/");
  }
  for (const auto& dir : dataDirs) {
    entryDirs.append(dir + "/applications");
  }
  return entryDirs;
}

void ApplicationMenuConfig::initCategories() {
  // We use the main categories as defined in:
  // https://specifications.freedesktop.org/menu-spec/latest/apa.html
  // plus a special Uncategorized category.
  static constexpr int kNumCategories = 12;
  static const char* const kCategories[kNumCategories][3] = {
    // Name, display name, icon.
    // Sorted by display name.
    {"Development", "Development", "applications-development"},
    {"Education", "Education", "applications-science"},
    {"Game", "Games", "applications-games"},
    {"Graphics", "Graphics", "applications-graphics"},
    {"Network", "Internet", "applications-internet"},
    {"AudioVideo", "Multimedia", "applications-multimedia"},
    {"Office", "Office", "applications-office"},
    {"Science", "Science", "applications-science"},
    {"Settings", "Settings", "preferences-system"},
    {"System", "System", "applications-system"},
    {"Utility", "Utilities", "applications-utilities"},
    // Uncategorized is not visible anyway.
    {kUncategorized, kUncategorized, "applications-other"},
  };
  categories_.reserve(kNumCategories);
  for (int i = 0; i < kNumCategories; ++i) {
    categories_.push_back(Category(
        kCategories[i][0], kCategories[i][1], kCategories[i][2]));
    categoryMap_[kCategories[i][0]] = i;
  }
}

void ApplicationMenuConfig::initSystemCategories() {
  systemCategories_ = desktopEnv_->getApplicationMenuSystemCategories();
}

void ApplicationMenuConfig::clearEntries() {
  for (auto& category : categories_) {
    category.entries.clear();
  }
  entries_.clear();
  shortAppIds_.clear();
  wmClasses_.clear();
  names_.clear();
}

bool ApplicationMenuConfig::loadEntries() {
  for (const QString& entryDir : entryDirs_) {
    if (!QDir::root().exists(entryDir)) {
      continue;
    }

    QDir dir(entryDir);
    QStringList files = dir.entryList({"*.desktop"}, QDir::Files, QDir::Name);
    if (files.isEmpty()) {
      continue;
    }

    for (int i = 0; i < files.size(); ++i) {
      const QString& file = entryDir + "/" + files.at(i);
      loadEntry(file);
    }
  }

  return true;
}

bool ApplicationMenuConfig::loadEntry(const QString &file) {
  DesktopFile desktopFile(file);

  if (desktopFile.type() != "Application") {
    return false;
  }

  auto categories = desktopFile.categories();
  if (categories.isEmpty()) {
    categories = {kUncategorized};
  }

  const QString appId = desktopFile.appId();
  for (int i = 0; i < categories.size(); ++i) {
    const std::string category = categories[i].toStdString();
    if (categoryMap_.count(category) > 0 &&
        entries_.count(appId.toStdString()) == 0) {
      const QString command = filterFieldCodes(desktopFile.exec().simplified());
      ApplicationEntry newEntry(appId,
                                desktopFile.name(),
                                desktopFile.genericName(),
                                desktopFile.icon(),
                                command,
                                file,
                                isHidden(desktopFile));
      auto& entries = categories_[categoryMap_[category]].entries;
      auto next = std::lower_bound(entries.begin(), entries.end(), newEntry);
      entries.insert(next, newEntry);

      auto* entry = &(*--next);
      const auto appId = newEntry.appId.toLower();
      entries_[appId.toStdString()] = entry;
      auto shortAppId = appId.simplified().replace(" ", "");
      shortAppId = shortAppId.mid(shortAppId.lastIndexOf('.') + 1);
      shortAppIds_[shortAppId.toStdString()] = entry;
      const auto shortCommand = getShortCommand(command).toLower().toStdString();
      if (!shortCommand.empty()) {
        commands_[shortCommand] = entry;
      }
      const auto wmClass =
          desktopFile.wmClass().toLower().simplified().replace(" ", "").toStdString();
      if (!wmClass.empty()) {
        wmClasses_[wmClass] = entry;
      }
      const auto name = desktopFile.name().toLower().simplified().replace(" ", "").toStdString();
      if (!name.empty()) {
        names_[name] = entry;
      }
    }
  }

  return true;
}

void ApplicationMenuConfig::reload() {
  clearEntries();
  loadEntries();
  emit configChanged();
}

const ApplicationEntry* ApplicationMenuConfig::findApplication(const std::string& appId) const {
  for (const auto& category : systemCategories_) {
    for (const auto& entry : category.entries) {
      if (entry.appId.toStdString() == appId) {
        return &entry;
      }
    }
  }
  return entries_.count(appId) > 0
      ? entries_.at(appId)
      : shortAppIds_.count(appId) > 0
          ? shortAppIds_.at(appId)
          : commands_.count(appId) > 0
              ? commands_.at(appId)
              : wmClasses_.count(appId) > 0
                  ? wmClasses_.at(appId)
                  : names_.count(appId) > 0
                      ? names_.at(appId)
                      : nullptr;
}

const ApplicationEntry* ApplicationMenuConfig::tryMatchingApplicationId(
    const std::string& appId) const {
  QString id = QString::fromStdString(appId).toLower();
  if (auto* app = findApplication(id.toStdString())) {
    return app;
  }

  id = id.simplified().replace(" ", "");
  if (auto* app = findApplication(id.toStdString())) {
    return app;
  }

  id = id.mid(id.lastIndexOf('.') + 1);
  if (auto* app = findApplication(id.toStdString())) {
    return app;
  }

  // Special fix for Qt6 D-Bus Viewer.
  if (id == "qdbusviewer") {
    if (auto* app = findApplication("org.qt.qdbusviewer6")) {
      return app;
    }
  }

  // Special fix for VirtualBox.
  if (id == "virtualboxvm" || id == "virtualboxmachine" || id == "virtualboxmanager") {
    if (auto* app = findApplication("virtualbox")) {
      return app;
    }
  }

  // Special fix for Google Chrome Flatpak.
  if (id == "google-chrome") {
    if (auto* app = findApplication("com.google.chrome")) {
      return app;
    }
  }

  return nullptr;
}

const std::vector<ApplicationEntry> ApplicationMenuConfig::searchApplications(
    const QString& text, unsigned int maxNumResults) const {
  std::vector<ApplicationEntry> entries;
  for (const auto& category : categories_) {
    for (const auto& entry : category.entries) {
      if (text.length() == 1) {
        if (entry.name.startsWith(text, Qt::CaseInsensitive)) {
          entries.push_back(entry);
        }
      } else {
        if (entry.name.contains(text, Qt::CaseInsensitive) ||
            entry.genericName.contains(text, Qt::CaseInsensitive)) {
          entries.push_back(entry);
        }
      }
      if (entries.size() == maxNumResults) {
        goto end_loop;
      }
    }
  }
end_loop:
  std::sort(entries.begin(), entries.end());
  return entries;
}

}  // namespace crystaldock
