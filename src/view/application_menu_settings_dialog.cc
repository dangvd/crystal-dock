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

#include "application_menu_settings_dialog.h"
#include "ui_application_menu_settings_dialog.h"

namespace crystaldock {

ApplicationMenuSettingsDialog::ApplicationMenuSettingsDialog(QWidget* parent,
    MultiDockModel* model)
    : QDialog(parent),
      ui(new Ui::ApplicationMenuSettingsDialog),
      icon_(new IconButton(this)),
      model_(model) {
  ui->setupUi(this);
  icon_->setGeometry(QRect(140, 80, 80, 80));
  setWindowFlag(Qt::Tool);

  connect(ui->buttonBox, SIGNAL(clicked(QAbstractButton*)),
      this, SLOT(buttonClicked(QAbstractButton*)));

  loadData();
}

ApplicationMenuSettingsDialog::~ApplicationMenuSettingsDialog() {
  delete ui;
}

void ApplicationMenuSettingsDialog::accept() {
  QDialog::accept();
  saveData();
}

void ApplicationMenuSettingsDialog::buttonClicked(QAbstractButton *button) {
  auto role = ui->buttonBox->buttonRole(button);
  if (role == QDialogButtonBox::ApplyRole) {
    saveData();
  }
}

void ApplicationMenuSettingsDialog::loadData() {
  ui->name->setText(model_->applicationMenuName());
  icon_->setIcon(model_->applicationMenuIcon());
  ui->strut->setChecked(model_->applicationMenuStrut());
}

void ApplicationMenuSettingsDialog::saveData() {
  model_->setApplicationMenuName(ui->name->text());
  model_->setApplicationMenuIcon(icon_->icon());
  model_->setApplicationMenuStrut(ui->strut->isChecked());
  model_->saveAppearanceConfig();
}

}  // namespace crystaldock
