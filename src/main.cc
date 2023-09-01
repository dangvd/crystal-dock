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

#include <iostream>

#include <QApplication>
#include <QDir>
#include <QIcon>
#include <QSharedMemory>

#include <model/multi_dock_model.h>
#include <view/multi_dock_view.h>

int main(int argc, char** argv) {
  QApplication app(argc, argv);

  // Forces single-instance.
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

  QApplication::setWindowIcon(QIcon::fromTheme("user-desktop"));

  crystaldock::MultiDockModel model(QDir::homePath() + "/.crystal-dock");
  crystaldock::MultiDockView view(&model);
  
  if (!view.checkPlatformSupported()) {
    return -1;
  }
  
  view.show();
  return app.exec();
}
