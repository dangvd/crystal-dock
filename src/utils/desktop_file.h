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

#ifndef CRYSTALDOCK_DESKTOP_FILE_H
#define CRYSTALDOCK_DESKTOP_FILE_H

#include <QMap>
#include <QString>
#include <QStringList>

namespace crystaldock {

// Follows Desktop Entry Specification:
// https://specifications.freedesktop.org/desktop-entry-spec/latest/index.html
class DesktopFile {
 public:
  DesktopFile() {}
  DesktopFile(const QString &file);
  bool write(const QString &file);

  QString appId() const { return appId_; }

  QString name() const { return values_["Name"]; }
  void setName(const QString& name) { values_["Name"] = name; }

  QString wmClass() const { return values_["StartupWMClass"]; }
  void setWMClass(const QString& wmClass) { values_["StartupWMClass"] = wmClass; }

  QString genericName() const { return values_["GenericName"]; }
  void setGenericName(const QString& genericName) { values_["GenericName"] = genericName; }

  QString icon() const { return values_["Icon"]; }
  void setIcon(const QString& icon) { values_["Icon"] = icon; }

  QString exec() const { return values_["Exec"]; }
  void setExec(const QString& exec) { values_["Exec"] = exec; }

  QString type() const { return values_["Type"]; }
  void setType(const QString& type) { values_["Type"] = type; }

  QStringList categories() const { return values_["Categories"].split(";", Qt::SkipEmptyParts); }
  void setCategories(const QString& categories) { values_["Categories"] = categories; }

  QStringList onlyShowIn() const { return values_["OnlyShowIn"].split(";", Qt::SkipEmptyParts); }
  void setOnlyShowIn(const QString& desktops) { values_["OnlyShowIn"] = desktops; }

  QStringList notShowIn() const { return values_["NotShowIn"].split(";", Qt::SkipEmptyParts); }
  void setNotShowIn(const QString& desktops) { values_["NotShowIn"] = desktops; }

  bool noDisplay() const { return values_["NoDisplay"].toLower() == "true"; }
  void setNoDisplay(bool value) { values_["NoDisplay"] = value ? "true" : "false"; }

  bool hidden() const { return values_["Hidden"].toLower() == "true"; }
  void setHidden(bool value) { values_["Hidden"] = value ? "true" : "false"; }

 private:
  QString appId_;
  QMap<QString, QString> values_;
};

}  // namespace crystaldock

#endif  // CRYSTALDOCK_DESKTOP_FILE_H
