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

#include <filesystem>
#include <iostream>

#include <QApplication>
#include <QDir>
#include <QIcon>
#include <QSharedMemory>
#include <QStringList>

#include <model/multi_dock_model.h>
#include <view/multi_dock_view.h>

namespace {

void maybeCopyOldConfigOnFirstRun(const QString& configDir) {
  // On the first run, copies the dock config from the old config location if it exists.
  // This is for backward compatibility.
  if (!QDir::home().exists(configDir)) {
    const auto oldConfigDir = QDir::homePath() + "/.crystal-dock-2";
    if (QDir::home().exists(oldConfigDir)) {
      std::filesystem::copy(oldConfigDir.toStdString(), configDir.toStdString(),
          std::filesystem::copy_options::recursive);
      std::cout << "Copied config from " << oldConfigDir.toStdString() << " to "
                << configDir.toStdString() << std::endl;
    }
  }
}

void maybeCopyPresetConfigOnFirstRun(const QString& configDir) {
  // On the first run, copies the dock config from XDG_CONFIG_DIRS if it exists.
  // This is mainly for package managers to pre-configure the dock.
  if (!QDir::home().exists(configDir)) {
    QStringList xdgConfigDirs =
        qEnvironmentVariable("XDG_CONFIG_DIRS").split(":", Qt::SkipEmptyParts);
    for (const auto& xdgConfigDir : xdgConfigDirs) {
      const auto srcDir = xdgConfigDir + "/.crystal-dock-2";
      if (QDir::home().exists(srcDir)) {
        std::filesystem::copy(srcDir.toStdString(), configDir.toStdString(),
            std::filesystem::copy_options::recursive);
        std::cout << "Copied config from " << srcDir.toStdString() << " to "
                  << configDir.toStdString() << std::endl;
        return;
      }
    }
  }
}

}

int main(int argc, char** argv) {
  QApplication app(argc, argv);

  // Enforces single instance.
  QSharedMemory sharedMemory;
  sharedMemory.setKey("crystal-dock-key");
  if (!sharedMemory.create(1 /*byte*/)) {
    // The failure might have been caused by a previous crash.
    sharedMemory.attach();
    sharedMemory.detach();
    // Now try again.
    if (!sharedMemory.create(1 /*byte*/)) {
      std::cerr << "Another instance is already running." << std::endl;
      return -1;
    }
  }

  if (!crystaldock::MultiDockView::checkPlatformSupported(app)) {
    return -1;
  }

  QApplication::setWindowIcon(QIcon::fromTheme("user-desktop"));

  auto configHome = qEnvironmentVariable("XDG_CONFIG_HOME").trimmed();
  if (configHome.isEmpty()) { configHome = QDir::homePath() + "/.config"; }
  const auto configDir = configHome + "/crystal-dock";
  maybeCopyOldConfigOnFirstRun(configDir);
  maybeCopyPresetConfigOnFirstRun(configDir);
  crystaldock::MultiDockModel model(configDir);
  crystaldock::MultiDockView view(&model);

  view.show();
  return app.exec();
}
