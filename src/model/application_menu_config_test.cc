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
  void tryMatchingApplicationId();

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
  writeEntry(entryDir.path() + "/chrome.desktop",
             {"chrome", "Chrome", "Web Browser", "chrome", "chrome", ""},
             "Network");

  ApplicationMenuConfig config({ entryDir.path() });

  QCOMPARE(config.entries_.size(), 1);

  for (const auto& category : config.categories_) {
    if (category.name == "Network") {
      QCOMPARE(static_cast<int>(category.entries.size()), 1);
    } else {
      QCOMPARE(static_cast<int>(category.entries.size()), 0);
    }
  }
}

void ApplicationMenuConfigTest::loadEntries_multipleDirs() {
  QTemporaryDir entryDir1;
  QVERIFY(entryDir1.isValid());
  writeEntry(entryDir1.path() + "/chrome.desktop",
             {"chrome", "Chrome", "Web Browser", "chrome", "chrome", ""},
             "Network");
  writeEntry(entryDir1.path() + "/mail.desktop",
             {"mail", "Mail", "Email Client", "mail", "mail", ""},
             "Network;Office");
  writeEntry(entryDir1.path() + "/adesktop-settings.desktop",
             {"adesktop-settings", "ADesktop Settings", "", "adesktop-settings",
              "adesktop-settings", ""},
             "Settings",
             {{"OnlyShowIn", "ADesktop"}});

  // Empty dir
  QTemporaryDir entryDir2;
  QVERIFY(entryDir2.isValid());

  QTemporaryDir entryDir3;
  QVERIFY(entryDir3.isValid());
  writeEntry(entryDir3.path() + "/systemsettings.desktop",
             {"systemsettings", "System Settings", "", "systemsettings", "systemsettings", ""},
             "Settings",
             {{"OnlyShowIn", DesktopEnv::getDesktopEnvName().toStdString()}});
  writeEntry(entryDir3.path() + "/bdesktop-settings.desktop",
             {"bdesktop-settings", "BDesktop Settings", "", "bdesktop-settings",
              "bdesktop-settings", ""},
             "Settings",
             {{"NotShowIn", DesktopEnv::getDesktopEnvName().toStdString()}});
  writeEntry(entryDir3.path() + "/chrome-html.desktop",
             {"chrome-html", "Chrome - HTML", "Web Browser", "chrome", "chrome", ""},
             "Network",
             {{"NoDisplay", "true"}});
  writeEntry(entryDir3.path() + "/chrome-old.desktop",
             {"chrome-old", "Chrome - Old", "Web Browser", "chrome", "chrome", ""},
             "Network",
             {{"Hidden", "true"}});

  ApplicationMenuConfig config(
      { entryDir1.path(), entryDir1.path() + "/dir-not-exist", entryDir2.path(),
        entryDir3.path() });

  QCOMPARE(config.entries_.size(), 3);

  for (const auto& category : config.categories_) {
    if (category.name == "Network") {
      QCOMPARE(static_cast<int>(category.entries.size()), 2);
    } else if (category.name == "Settings") {
      QCOMPARE(static_cast<int>(category.entries.size()), 1);
    } else {
      QCOMPARE(static_cast<int>(category.entries.size()), 0);
    }
  }
}

void ApplicationMenuConfigTest::tryMatchingApplicationId() {
  QTemporaryDir entryDir;
  QVERIFY(entryDir.isValid());
  writeEntry(entryDir.path() + "/firefox.desktop",
             {"firefox", "Firefox", "Web Browser",
              "firefox", "firefox", ""},
             "Network");
  writeEntry(entryDir.path() + "/org.kde.konsole.desktop",
             {"org.kde.konsole", "Konsole", "Terminal",
              "konsole", "konsole", ""},
             "System");
  writeEntry(entryDir.path() + "/google-chrome.desktop",
             {"google-chrome", "Chrome", "Web Browser",
              "google-chrome", "google-chrome", ""},
             "Network");
  writeEntry(entryDir.path() + "/org.kde.krita.desktop",
             {"org.kde.krita", "Krita", "Digital Painting",
              "krita", "Krita", ""},
             "Graphics");
  writeEntry(entryDir.path() + "/gimp.desktop",
             {"gimp", "GIMP", "Image Editor",
              "gimp", "GIMP", ""},
             "Graphics");
  writeEntry(entryDir.path() + "/code-insiders.desktop",
             {"code-insiders", "Code Insiders", "Code Editor",
              "code-insiders", "Code Insiders", ""},
             "Development");
  writeEntry(entryDir.path() + "/seventhstring-transcribe.desktop",
             {"seventhstring-transcribe", "Transcribe!", "Lyrics Transcriber",
              "seventhstring-transcribe", "Transcribe!", ""},
             "AudioVideo");
  writeEntry(entryDir.path() + "/dbeaver-ee.desktop",
             {"dbeaver-ee", "DBeaver Enterprise", "Database Management",
              "dbeaver-ee", "DBeaver Enterprise", ""},
             "Office");

  ApplicationMenuConfig config({ entryDir.path() });

  QCOMPARE(config.entries_.size(), 8);

  QCOMPARE(config.tryMatchingApplicationId("firefox"), "firefox");
  QCOMPARE(config.tryMatchingApplicationId("org.kde.konsole"), "org.kde.konsole");
  QCOMPARE(config.tryMatchingApplicationId("Google-chrome"), "google-chrome");
  QCOMPARE(config.tryMatchingApplicationId("krita"), "org.kde.krita");
  QCOMPARE(config.tryMatchingApplicationId("Gimp-2.10"), "gimp");
  QCOMPARE(config.tryMatchingApplicationId("Code - Insiders"), "code-insiders");
  QCOMPARE(config.tryMatchingApplicationId("Transcribe!"), "seventhstring-transcribe");
  QCOMPARE(config.tryMatchingApplicationId("dbeaverenterprise"), "dbeaver-ee");
}

}  // namespace crystaldock

QTEST_MAIN(crystaldock::ApplicationMenuConfigTest)
#include "application_menu_config_test.moc"
