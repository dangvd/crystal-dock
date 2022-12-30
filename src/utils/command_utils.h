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

#ifndef CRYSTALDOCK_COMMAND_UTILS_H_
#define CRYSTALDOCK_COMMAND_UTILS_H_

#include <filesystem>

#include <QString>

namespace crystaldock {

static constexpr char kSeparatorCommand[] = "SEPARATOR";
static constexpr char kShowDesktopCommand[] = "SHOW_DESKTOP";
static constexpr char kLockScreenCommand[] = "xdg-screensaver lock";

inline QString filterFieldCodes(const QString& command) {
  QString filtered = command.contains('%')
      ? command.left(command.indexOf('%') - 1)
      : command;

  if (filtered.startsWith("env")) {
    int i = filtered.lastIndexOf("=");
    filtered = filtered.mid(filtered.indexOf(" ", i) + 1);
  }

  return filtered;
}

inline bool isCommandLockScreen(const QString& command) {
  return command == kLockScreenCommand;
}

inline std::string getTaskCommand(const std::string& appCommand) {
  namespace fs = std::filesystem;
  auto command = appCommand.substr(0, appCommand.find_first_of(' '));
  command = fs::is_symlink(command) ?
      fs::path(fs::read_symlink(command)).filename() :
      fs::path(command).filename();
  return QString::fromStdString(command).toLower().toStdString();
}

inline QString getTaskCommand(const QString& appCommand) {
  return QString::fromStdString(getTaskCommand(appCommand.toStdString()));
}

// This needs to be synced with ApplicationMenuConfig::findApplication()
inline bool areTheSameCommand(const QString& appTaskCommand, const QString& taskCommand) {
  if (appTaskCommand == taskCommand) {
    return true;
  }

  // Fix for Synaptic.
  if (taskCommand == "synaptic" && appTaskCommand == "synaptic-pkexec") {
    return true;
  }

    // Fix for GNOME Nautilus etc.
  if ("org.gnome." + appTaskCommand == taskCommand) {
    return true;
  }

  return false;
}

}  // namespace crystaldock

#endif  // CRYSTALDOCK_COMMAND_UTILS_H_
