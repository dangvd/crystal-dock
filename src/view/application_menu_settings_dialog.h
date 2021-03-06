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

#ifndef CRYSTALDOCK_APPLICATION_MENU_SETTINGS_DIALOG_H_
#define CRYSTALDOCK_APPLICATION_MENU_SETTINGS_DIALOG_H_

#include <QAbstractButton>
#include <QDialog>

#include <model/multi_dock_model.h>

namespace Ui {
  class ApplicationMenuSettingsDialog;
}

namespace crystaldock {

class ApplicationMenuSettingsDialog : public QDialog {
  Q_OBJECT

 public:
  ApplicationMenuSettingsDialog(QWidget* parent, MultiDockModel* model);
  ~ApplicationMenuSettingsDialog();

  void reload() { loadData(); }

 public slots:
  void accept() override;
  void buttonClicked(QAbstractButton* button);

 private:
  void loadData();
  void saveData();
  void resetData();

  Ui::ApplicationMenuSettingsDialog *ui;

  MultiDockModel* model_;

  friend class ApplicationMenuSettingsDialogTest;
};

}  // namespace crystaldock

#endif  // CRYSTALDOCK_APPLICATION_MENU_SETTINGS_DIALOG_H_
