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

#ifndef CRYSTALDOCK_APPEARANCE_SETTINGS_DIALOG_H_
#define CRYSTALDOCK_APPEARANCE_SETTINGS_DIALOG_H_

#include <QDialog>

#include "color_button.h"
#include <model/multi_dock_model.h>

namespace Ui {
  class AppearanceSettingsDialog;
}

namespace crystaldock {

class AppearanceSettingsDialog : public QDialog {
  Q_OBJECT

 public:
  AppearanceSettingsDialog(QWidget* parent, MultiDockModel* model);
  ~AppearanceSettingsDialog();

  void reload() { loadData(); }

 public slots:
  void accept() override;
  void buttonClicked(QAbstractButton* button);
  void onEnableZoomingChanged();

 private:
  void loadData();
  void resetData();
  void saveData();

  Ui::AppearanceSettingsDialog *ui;
  ColorButton* backgroundColor_;
  ColorButton* borderColor_;
  ColorButton* activeIndicatorColor_;
  ColorButton* inactiveIndicatorColor_;

  MultiDockModel* model_;

  int prevMaxIconSize_ = 0;

  friend class AppearanceSettingsDialogTest;
};

}  // namespace crystaldock

#endif  // CRYSTALDOCK_APPEARANCE_SETTINGS_DIALOG_H_
