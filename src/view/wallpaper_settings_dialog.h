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

#ifndef CRYSTALDOCK_WALLPAPER_SETTINGS_DIALOG_H_
#define CRYSTALDOCK_WALLPAPER_SETTINGS_DIALOG_H_

#include <QAbstractButton>
#include <QDialog>
#include <QString>

#include <model/multi_dock_model.h>

namespace Ui {
  class WallpaperSettingsDialog;
}

namespace crystaldock {

class WallpaperSettingsDialog : public QDialog {
  Q_OBJECT

 public:
  WallpaperSettingsDialog(QWidget* parent, MultiDockModel* model);
  ~WallpaperSettingsDialog();

  void setFor(int desktop, int screen);

 public slots:
  void populateDesktopList();

  void accept() override;
  void buttonClicked(QAbstractButton* button);

  void browseWallpaper();

  void adjustUiForScreen();

  void reload();

 private:
  // Gets screen (0-based).
  int screen() const;

  // Gets desktop (1-based).
  int desktop() const;

  void loadData();
  void saveData();

  Ui::WallpaperSettingsDialog *ui;

  MultiDockModel* model_;

  // Path to wallpaper file.
  QString wallpaper_;

  // Remember the current directory of the session when opening the file dialog
  // for browsing wallpapers.
  QString currentDir_;

  bool multiScreen_;
};

}  // namespace crystaldock

#endif // CRYSTALDOCK_WALLPAPER_SETTINGS_DIALOG_H_
