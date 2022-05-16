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

#include <utils/math_utils.h>

namespace crystaldock {

ApplicationMenuSettingsDialog::ApplicationMenuSettingsDialog(QWidget* parent,
    MultiDockModel* model)
    : QDialog(parent),
      ui(new Ui::ApplicationMenuSettingsDialog),
      model_(model) {
  ui->setupUi(this);
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
  } else if (role == QDialogButtonBox::ResetRole) {
    resetData();
  }
}

void ApplicationMenuSettingsDialog::loadData() {
  ui->name->setText(model_->applicationMenuName());
  ui->iconSize->setValue(model_->applicationMenuIconSize());
  ui->fontSize->setValue(model_->applicationMenuFontSize());
  ui->backgroundTransparency->setValue(
      alphaFToTransparencyPercent(model_->applicationMenuBackgroundAlpha()));
}

void ApplicationMenuSettingsDialog::resetData() {
  ui->name->setText(kDefaultApplicationMenuName);
  ui->iconSize->setValue(kDefaultApplicationMenuIconSize);
  ui->fontSize->setValue(kDefaultApplicationMenuFontSize);
  ui->backgroundTransparency->setValue(
      alphaFToTransparencyPercent(kDefaultApplicationMenuBackgroundAlpha));
}

void ApplicationMenuSettingsDialog::saveData() {
  model_->setApplicationMenuName(ui->name->text());
  model_->setApplicationMenuIconSize(ui->iconSize->value());
  model_->setApplicationMenuFontSize(ui->fontSize->value());
  model_->setApplicationMenuBackgroundAlpha(
      transparencyPercentToAlphaF(ui->backgroundTransparency->value()));
  model_->saveAppearanceConfig();
}

}  // namespace crystaldock
