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

#include "wallpaper_settings_dialog.h"
#include "ui_wallpaper_settings_dialog.h"

#include <algorithm>

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QGuiApplication>
#include <QPixmap>
#include <QScreen>
#include <QVariant>

#include "display/window_system.h"

namespace crystaldock {

WallpaperSettingsDialog::WallpaperSettingsDialog(QWidget* parent,
                                                 MultiDockModel* model)
    : QDialog(parent),
      ui(new Ui::WallpaperSettingsDialog),
      model_(model),
      desktopEnv_(DesktopEnv::getDesktopEnv()),
      currentDir_(QDir::homePath()),
      multiScreen_(false) {
  ui->setupUi(this);
  setWindowFlag(Qt::Tool);

  // Populate screen list.
  const int screenCount = WindowSystem::screens().size();
  for (int i = 1; i <= screenCount; ++i) {
    ui->screen->addItem(QString::number(i));
  }
  ui->screen->setCurrentIndex(0);

  // Adjust the UI for single/multi-screen.
  multiScreen_ = (screenCount > 1) && desktopEnv_->supportSeparateSreenWallpapers();
  ui->screenLabel->setVisible(multiScreen_);
  ui->screen->setVisible(multiScreen_);

  adjustUiForScreen();

  connect(ui->desktop, SIGNAL(currentIndexChanged(int)),
          this, SLOT(reload()));
  connect(ui->browse, SIGNAL(clicked()), this, SLOT(browseWallpaper()));
  connect(ui->buttonBox, SIGNAL(clicked(QAbstractButton*)),
      this, SLOT(buttonClicked(QAbstractButton*)));

  if (multiScreen_) {
    connect(ui->screen, SIGNAL(currentIndexChanged(int)),
            this, SLOT(reload()));
  }
}

WallpaperSettingsDialog::~WallpaperSettingsDialog() {
  delete ui;
}

void WallpaperSettingsDialog::setFor(int desktop, int screen) {
  populateDesktopList();
  ui->desktop->setCurrentIndex(desktop - 1);
  if (multiScreen_) {
    ui->screen->setCurrentIndex(screen);
    adjustUiForScreen();
  }
  loadData();
}

void WallpaperSettingsDialog::populateDesktopList() {
  ui->desktop->clear();
  for (const auto& desktop : WindowSystem::desktops()) {
    ui->desktop->addItem(QString::number(desktop.number),
                         QVariant(QString::fromStdString(desktop.id)));
  }
}

void WallpaperSettingsDialog::accept() {
  QDialog::accept();
  saveData();
}

void WallpaperSettingsDialog::buttonClicked(QAbstractButton *button) {
  auto role = ui->buttonBox->buttonRole(button);
  if (role == QDialogButtonBox::ApplyRole) {
    saveData();
  }
}

void WallpaperSettingsDialog::browseWallpaper() {
  const QString& wallpaper = QFileDialog::getOpenFileName(
        this,
        QString("Select Wallpaper Image"),
        currentDir_,
        QString("Image Files (*.png *.jpg *.bmp)"));
  if (wallpaper.isEmpty()) {
    return;
  }

  wallpaper_ = wallpaper;
  ui->preview->setPixmap(QPixmap(wallpaper_));
  currentDir_ = QFileInfo(wallpaper_).dir().absolutePath();
}

void WallpaperSettingsDialog::adjustUiForScreen() {
  const auto screenGeometry = WindowSystem::screens()[screen()]->geometry();
  const int w = ui->preview->width();
  const int h = w * screenGeometry.height() / screenGeometry.width();
  const int delta = h - ui->preview->height();
  ui->preview->resize(w, h);
  ui->previewHolder->resize(ui->previewHolder->width(),
                            ui->previewHolder->height() + delta);
  ui->buttonBox->move(ui->buttonBox->x(), ui->buttonBox->y() + delta);
  resize(width(), height() + delta);
}

void WallpaperSettingsDialog::reload() {
  if (multiScreen_) {
    adjustUiForScreen();
  }
  loadData();
}

int WallpaperSettingsDialog::screen() const {
  return ui->screen->currentIndex();
}

std::string WallpaperSettingsDialog::desktop() const {
  return ui->desktop->currentData().toString().toStdString();
}

void WallpaperSettingsDialog::loadData() {
  wallpaper_ = model_->wallpaper(desktop(), screen());
  ui->preview->setPixmap(QPixmap(wallpaper_));
}

void WallpaperSettingsDialog::saveData() {
  if (!wallpaper_.isEmpty() &&
      (wallpaper_ != model_->wallpaper(desktop(), screen()))) {
    const int screenCount = WindowSystem::screens().size();
    if (desktopEnv_->supportSeparateSreenWallpapers()) {
      model_->setWallpaper(desktop(), screen(), wallpaper_);
    } else {
      for (int screen = 0; screen < screenCount; ++screen) {
        model_->setWallpaper(desktop(), screen, wallpaper_);
      }
    }
    model_->saveAppearanceConfig();
    if (desktop() == WindowSystem::currentDesktop()) {
      if (desktopEnv_->supportSeparateSreenWallpapers()) {
        model_->notifyWallpaperChanged(screen());
      } else {
        for (int screen = 0; screen < screenCount; ++screen) {
          model_->notifyWallpaperChanged(screen);
        }
      }
    }
  }
}

}  // namespace crystaldock
