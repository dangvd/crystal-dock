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

#include "task_manager_settings_dialog.h"
#include "ui_task_manager_settings_dialog.h"

#include <QGuiApplication>

namespace crystaldock {

TaskManagerSettingsDialog::TaskManagerSettingsDialog(QWidget* parent, MultiDockModel* model) :
  QDialog(parent),
  ui(new Ui::TaskManagerSettingsDialog),
  model_(model),
  isSingleScreen_(true) {
  ui->setupUi(this);

  // Adjust the UI for single/multi-screen.
  isSingleScreen_ = (QGuiApplication::screens().size() == 1);
  ui->showCurrentScreenOnly->setVisible(!isSingleScreen_);
  if (isSingleScreen_) {
    ui->buttonBox->move(40, 200);
    resize(600, 260);
  }
  connect(ui->buttonBox, SIGNAL(clicked(QAbstractButton*)),
      this, SLOT(buttonClicked(QAbstractButton*)));

  loadData();
}

TaskManagerSettingsDialog::~TaskManagerSettingsDialog() {
  delete ui;
}

void TaskManagerSettingsDialog::accept() {
  QDialog::accept();
  saveData();
}

void TaskManagerSettingsDialog::buttonClicked(QAbstractButton* button) {
  auto role = ui->buttonBox->buttonRole(button);
  if (role == QDialogButtonBox::ApplyRole) {
    saveData();
  }
}

void TaskManagerSettingsDialog::loadData() {
  ui->showCurrentDesktopOnly->setChecked(model_->currentDesktopTasksOnly());
  if (!isSingleScreen_) {
    ui->showCurrentScreenOnly->setChecked(model_->currentScreenTasksOnly());
  }
}

void TaskManagerSettingsDialog::saveData() {
  model_->setCurrentDesktopTasksOnly(ui->showCurrentDesktopOnly->isChecked());
  if (!isSingleScreen_) {
    model_->setCurrentScreenTasksOnly(ui->showCurrentScreenOnly->isChecked());
  }
  model_->saveAppearanceConfig();
}

}  // namespace crystaldock
