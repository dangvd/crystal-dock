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

#include <unordered_map>

#include <QString>
#include <QTemporaryDir>
#include <QTest>

#include <desktop/desktop_env.h>
#include <utils/desktop_file.h>

namespace crystaldock {

class ApplicationMenuConfigTest: public QObject {
  Q_OBJECT

 private slots:
  void init() {
    QTemporaryDir configDir;
  }

  void loadEntries_singleDir();
  void loadEntries_multipleDirs();

 private:

  void writeEntry(const QString& filename, const ApplicationEntry& entry,
                  const QString& categories,
                  const std::unordered_map<std::string, std::string>& extraKVs
                      = {}) {
    DesktopFile desktopFile;
    desktopFile.setName(entry.name);
    desktopFile.setGenericName(entry.genericName);
    desktopFile.setIcon(entry.icon);
    desktopFile.setExec(entry.command);
    desktopFile.setType("Application");
    desktopFile.setCategories(categories);
    for (const auto& entry : extraKVs) {
      if (entry.first == "Hidden") {
        desktopFile.setHidden(entry.second == "true");
      } else if (entry.first == "NoDisplay") {
        desktopFile.setNoDisplay(entry.second == "true");
      } else if (entry.first == "OnlyShowIn") {
        desktopFile.setOnlyShowIn(QString::fromStdString(entry.second));
      } else if (entry.first == "NotShowIn") {
        desktopFile.setNotShowIn(QString::fromStdString(entry.second));
      }
    }
    desktopFile.write(filename);
  }
};

void ApplicationMenuConfigTest::loadEntries_singleDir() {
  QTemporaryDir entryDir;
  QVERIFY(entryDir.isValid());
  writeEntry(entryDir.path() + "/1.desktop",
             {"chrome", "Chrome", "Web Browser", "chrome", "chrome", ""},
             "Internet");

  ApplicationMenuConfig ApplicationMenuConfig({ entryDir.path() });

  for (const auto& category : ApplicationMenuConfig.categories_) {
    if (category.name == "Internet") {
      QCOMPARE(static_cast<int>(category.entries.size()), 1);
    } else {
      QCOMPARE(static_cast<int>(category.entries.size()), 0);
    }
  }
}

void ApplicationMenuConfigTest::loadEntries_multipleDirs() {
  QTemporaryDir entryDir1;
  QVERIFY(entryDir1.isValid());
  writeEntry(entryDir1.path() + "/1.desktop",
             {"chrome", "Chrome", "Web Browser", "chrome", "chrome", ""},
             "Internet");
  writeEntry(entryDir1.path() + "/2.desktop",
             {"mail", "Mail", "Email Client", "mail", "mail", ""},
             "Internet;Office");
  writeEntry(entryDir1.path() + "/3.desktop",
             {"adesktop-settings", "ADesktop Settings", "", "adesktop-settings",
              "adesktop-settings", ""},
             "Settings",
             {{"OnlyShowIn", "ADesktop"}});

  // Empty dir
  QTemporaryDir entryDir2;
  QVERIFY(entryDir2.isValid());

  QTemporaryDir entryDir3;
  QVERIFY(entryDir3.isValid());
  writeEntry(entryDir3.path() + "/1.desktop",
             {"systemsettings", "System Settings", "", "systemsettings", "systemsettings", ""},
             "Settings",
             {{"OnlyShowIn", DesktopEnv::getDesktopEnvName().toStdString()}});
  writeEntry(entryDir3.path() + "/2.desktop",
             {"adesktop-settings", "ADesktop Settings", "", "adesktop-settings",
              "adesktop-settings", ""},
             "Settings",
             {{"NotShowIn", DesktopEnv::getDesktopEnvName().toStdString()}});
  writeEntry(entryDir3.path() + "/3.desktop",
             {"chrome", "Chrome - HTML", "Web Browser", "chrome", "chrome", ""},
             "Internet",
             {{"NoDisplay", "true"}});
  writeEntry(entryDir3.path() + "/4.desktop",
             {"chrome", "Chrome - Old", "Web Browser", "chrome", "chrome", ""},
             "Internet",
             {{"Hidden", "true"}});

  ApplicationMenuConfig ApplicationMenuConfig(
      { entryDir1.path(), entryDir1.path() + "/dir-not-exist", entryDir2.path(),
        entryDir3.path() });

  for (const auto& category : ApplicationMenuConfig.categories_) {
    if (category.name == "Internet") {
      QCOMPARE(static_cast<int>(category.entries.size()), 2);
    } else if (category.name == "Settings" || category.name == "Office") {
      QCOMPARE(static_cast<int>(category.entries.size()), 1);
    } else {
      QCOMPARE(static_cast<int>(category.entries.size()), 0);
    }
  }
}

}  // namespace crystaldock

QTEST_MAIN(crystaldock::ApplicationMenuConfigTest)
#include "application_menu_config_test.moc"
