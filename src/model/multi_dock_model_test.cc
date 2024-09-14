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

#include "multi_dock_model.h"

#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

#include "config_helper.h"
#include <desktop/desktop_env.h>

namespace crystaldock {

class MultiDockModelTest: public QObject {
  Q_OBJECT

 private slots:
  void load_noDock();

  // See comment below.
  // void load_singleDock();
  // void load_multipleDocks();

 private:
  void createDockConfig(const QTemporaryDir& configDir, int fileId) {
    QString dirPath = configDir.path() + "/" + DesktopEnv::getDesktopEnvName();
    if (!QDir(dirPath).exists()) {
      QDir::root().mkpath(dirPath);
    }
    QFile dockConfig(dirPath + "/" + ConfigHelper::dockConfigFile(fileId));
    dockConfig.open(QIODevice::WriteOnly);
  }
};

void MultiDockModelTest::load_noDock() {
  QTemporaryDir configDir;
  MultiDockModel model(configDir.path());
  QCOMPARE(model.dockCount(), 0);
}

// The following tests are disabled for now
// because QGuiApplication::screens() returns an empty list in the test.
// TODO: re-enable these tests.
/*
void MultiDockModelTest::load_singleDock() {
  QTemporaryDir configDir;
  QVERIFY(configDir.isValid());
  createDockConfig(configDir, 1);

  MultiDockModel model(configDir.path());
  QCOMPARE(model.dockCount(), 1);
}

void MultiDockModelTest::load_multipleDocks() {
  QTemporaryDir configDir;
  QVERIFY(configDir.isValid());
  createDockConfig(configDir, 1);
  createDockConfig(configDir, 2);
  createDockConfig(configDir, 4);

  MultiDockModel model(configDir.path());
  QCOMPARE(model.dockCount(), 3);
}
*/

}  // namespace crystaldock

QTEST_MAIN(crystaldock::MultiDockModelTest)
#include "multi_dock_model_test.moc"
