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

#include "appearance_settings_dialog.h"
#include "ui_appearance_settings_dialog.h"

#include <utils/math_utils.h>

namespace crystaldock {

AppearanceSettingsDialog::AppearanceSettingsDialog(QWidget* parent,
                                                   MultiDockModel* model)
    : QDialog(parent),
      ui(new Ui::AppearanceSettingsDialog),
      model_(model) {
  ui->setupUi(this);
  setWindowFlag(Qt::Tool);

  backgroundColor_ = new ColorButton(this);
  backgroundColor_->setGeometry(QRect(260, 150, 80, 40));

  borderColor_ = new ColorButton(this);
  borderColor_->setGeometry(QRect(660, 150, 80, 40));

  activeIndicatorColor_ = new ColorButton(this);
  activeIndicatorColor_->setGeometry(QRect(260, 210, 80, 40));
  inactiveIndicatorColor_ = new ColorButton(this);
  inactiveIndicatorColor_->setGeometry(QRect(660, 210, 80, 40));

  connect(ui->buttonBox, SIGNAL(clicked(QAbstractButton*)),
      this, SLOT(buttonClicked(QAbstractButton*)));

  loadData();
}

AppearanceSettingsDialog::~AppearanceSettingsDialog() {
  delete ui;
}

void AppearanceSettingsDialog::accept() {
  QDialog::accept();
  saveData();
}

void AppearanceSettingsDialog::buttonClicked(QAbstractButton* button) {
  auto role = ui->buttonBox->buttonRole(button);
  if (role == QDialogButtonBox::ApplyRole) {
    saveData();
  } else if (role == QDialogButtonBox::ResetRole) {
    resetData();
  }
}

void AppearanceSettingsDialog::loadData() {
  ui->minSize->setValue(model_->minIconSize());
  ui->maxSize->setValue(model_->maxIconSize());
  ui->spacingFactor->setValue(model_->spacingFactor());
  QColor backgroundColor = model_->backgroundColor();
  backgroundColor_->setColor(QColor(backgroundColor.rgb()));
  ui->backgroundTransparency->setValue(alphaFToTransparencyPercent(backgroundColor.alphaF()));
  ui->showBorder->setChecked(model_->showBorder());
  borderColor_->setColor(model_->borderColor());
  activeIndicatorColor_->setColor(model_->activeIndicatorColor());
  inactiveIndicatorColor_->setColor(model_->inactiveIndicatorColor());
  ui->tooltipFontSize->setValue(model_->tooltipFontSize());
  ui->floatingMargin->setValue(model_->floatingMargin());
  ui->floatingMargin->setEnabled(model_->panelStyle() == PanelStyle::Floating3D ||
                                 model_->panelStyle() == PanelStyle::Floating2D);
}

void AppearanceSettingsDialog::resetData() {
  ui->minSize->setValue(kDefaultMinSize);
  ui->maxSize->setValue(kDefaultMaxSize);
  ui->spacingFactor->setValue(kDefaultSpacingFactor);
  backgroundColor_->setColor(QColor(kDefaultBackgroundColor));
  ui->backgroundTransparency->setValue(alphaFToTransparencyPercent(kDefaultBackgroundAlpha));
  ui->showBorder->setChecked(kDefaultShowBorder);
  borderColor_->setColor(QColor(kDefaultBorderColor));
  activeIndicatorColor_->setColor(QColor(kDefaultActiveIndicatorColor));
  inactiveIndicatorColor_->setColor(QColor(kDefaultInactiveIndicatorColor));
  ui->tooltipFontSize->setValue(kDefaultTooltipFontSize);
  ui->floatingMargin->setValue(kDefaultFloatingMargin);
}

void AppearanceSettingsDialog::saveData() {
  model_->setMinIconSize(ui->minSize->value());
  model_->setMaxIconSize(ui->maxSize->value());
  model_->setSpacingFactor(ui->spacingFactor->value());
  QColor backgroundColor(backgroundColor_->color());
  backgroundColor.setAlphaF(transparencyPercentToAlphaF(ui->backgroundTransparency->value()));
  model_->setBackgroundColor(backgroundColor);
  model_->setShowBorder(ui->showBorder->isChecked());
  model_->setBorderColor(borderColor_->color());
  model_->setActiveIndicatorColor(activeIndicatorColor_->color());
  model_->setInactiveIndicatorColor(inactiveIndicatorColor_->color());
  model_->setTooltipFontSize(ui->tooltipFontSize->value());
  model_->setFloatingMargin(ui->floatingMargin->value());
  model_->saveAppearanceConfig();
}

}  // namespace crystaldock
