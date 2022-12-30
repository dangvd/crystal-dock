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

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QStringBuilder>
#include <QUrl>

#include <utils/command_utils.h>
#include <utils/desktop_file.h>

namespace crystaldock {

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
  static constexpr int kNumCategories = 11;
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
  commandsToEntries_.clear();
  namesToEntries_.clear();
  filesToEntries_.clear();
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

  if (desktopFile.noDisplay() || desktopFile.hidden()) {
    return false;
  }

  if (desktopFile.type() != "Application") {
    return false;
  }

  QString desktopEnvName = DesktopEnv::getDesktopEnvName();
  if (!desktopFile.onlyShowIn().empty() && !desktopFile.onlyShowIn().contains(desktopEnvName)) {
    // Fix for Cinnamon.
    if ((desktopEnvName != "X-Cinnamon" && desktopEnvName != "Cinnamon") ||
        !desktopFile.onlyShowIn().contains("GNOME")) {
      return false;
    }
  }

  if (!desktopFile.notShowIn().empty() && desktopFile.notShowIn().contains(desktopEnvName)) {
    return false;
  }

  auto categories = desktopFile.categories();
  if (categories.isEmpty()) {
    return false;
  }

  for (int i = 0; i < categories.size(); ++i) {
    const std::string category = categories[i].toStdString();
    if (categoryMap_.count(category) > 0 &&
        namesToEntries_.count(desktopFile.name().toStdString()) == 0) {
      const QString command = filterFieldCodes(desktopFile.exec());
      ApplicationEntry newEntry(desktopFile.name(),
                                desktopFile.genericName(),
                                desktopFile.icon(),
                                command,
                                file);
      auto& entries = categories_[categoryMap_[category]].entries;
      auto next = std::lower_bound(entries.begin(), entries.end(), newEntry);
      entries.insert(next, newEntry);

      auto* entry = &(*--next);
      entries_[newEntry.taskCommand.toStdString()] = entry;
      commandsToEntries_[newEntry.command.toStdString()] = entry;
      namesToEntries_[desktopFile.name().toStdString()] = entry;
      filesToEntries_[QFileInfo(file).fileName().toStdString()] = entry;
    }
  }

  return true;
}

void ApplicationMenuConfig::reload() {
  clearEntries();
  loadEntries();
  emit configChanged();
}

// This needs to be synced with command_utils.h/areTheSameCommand()
const ApplicationEntry* ApplicationMenuConfig::findApplication(
    const std::string& command) const {
  if (entries_.count(command) > 0) {
    return entries_.at(command);
  }

  // Fix for Synaptic.
  if (command == "synaptic") {
    const auto command2 = "synaptic-pkexec";
    if (entries_.count(command2) > 0) {
      return entries_.at(command2);
    }
  }

  // Fix for GNOME Nautilus etc.
  if (command.find("org.gnome.") != std::string::npos) {
    const auto command2 = command.substr(10);
    if (entries_.count(command2) > 0) {
      return entries_.at(command2);
    }
  }

  return nullptr;
}

const ApplicationEntry* ApplicationMenuConfig::findApplicationFromFile(
    const std::string& desktopFile) const {
  return (filesToEntries_.count(desktopFile) > 0) ? filesToEntries_.at(desktopFile) : nullptr;
}

const std::vector<ApplicationEntry> ApplicationMenuConfig::searchApplications(const QString& text) const {
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
    }
  }
  std::sort(entries.begin(), entries.end());
  return entries;
}

bool ApplicationMenuConfig::isAppMenuEntry(const std::string& command) const {
  // Fix for Synaptic.
  if (command == "synaptic") {
    return entries_.count("synaptic-pkexec") > 0;
  }
  return commandsToEntries_.count(command) > 0;
}

}  // namespace crystaldock
