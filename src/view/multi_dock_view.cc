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

#include "multi_dock_view.h"

#include <QGuiApplication>
#include <QMessageBox>

#include <KWindowSystem>

#include "add_panel_dialog.h"

namespace crystaldock {

MultiDockView::MultiDockView(MultiDockModel* model)
    : model_(model),
      desktopEnv_(DesktopEnv::getDesktopEnv()) {
  connect(model_, SIGNAL(dockAdded(int)), this, SLOT(onDockAdded(int)));
  connect(model_, SIGNAL(wallpaperChanged(int)), this,
          SLOT(setWallpaper(int)));
  connect(KWindowSystem::self(), SIGNAL(currentDesktopChanged(int)),
          this, SLOT(setWallpaper()));
  loadData();
}

void MultiDockView::show() {
  for (const auto& dock : docks_) {
    dock.second->show();
  }
  setWallpaper();
}

void MultiDockView::exit() {
  for (const auto& dock : docks_) {
    dock.second->close();
  }
}

void MultiDockView::onDockAdded(int dockId) {
  docks_[dockId] = std::make_unique<DockPanel>(this, model_, dockId);
  docks_[dockId]->show();
}

bool MultiDockView::setWallpaper() {
  if (!model_->hasPager()) {
    return false;
  }

  for (int screen = 0; screen < QGuiApplication::screens().size(); ++screen) {
    if (setWallpaper(screen)) {
      return false;
    }
  }

  return true;
}

bool MultiDockView::setWallpaper(int screen) {
  if (!model_->hasPager()) {
    return false;
  }

  QString wallpaper = model_->wallpaper(KWindowSystem::currentDesktop(), screen);
  if (wallpaper.isEmpty()) {
    return false;  // nothing to do here.
  }

  if (!QFile::exists(wallpaper)) {
    QMessageBox::warning(nullptr,
                         "Error",
                         QString("Failed to load wallpaper from: ") + wallpaper);
    return false;
  }

  return desktopEnv_->setWallpaper(screen, wallpaper);
}

void MultiDockView::loadData() {
  docks_.clear();
  for (int dockId = 1; dockId <= model_->dockCount(); ++dockId) {
    docks_[dockId] = std::make_unique<DockPanel>(this, model_, dockId);
  }

  if (docks_.empty()) {
    AddPanelDialog dialog(nullptr, model_, 0 /* dockId not needed */);
    dialog.setMode(AddPanelDialog::Mode::Welcome);
    dialog.exec();
  }
}

}  // namespace crystaldock
