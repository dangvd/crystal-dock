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
  backgroundColor_->setGeometry(QRect(260, 210, 80, 40));

  borderColor_ = new ColorButton(this);
  borderColor_->setGeometry(QRect(700, 210, 80, 40));

  activeIndicatorColor_ = new ColorButton(this);
  activeIndicatorColor_->setGeometry(QRect(260, 270, 80, 40));
  inactiveIndicatorColor_ = new ColorButton(this);
  inactiveIndicatorColor_->setGeometry(QRect(700, 270, 80, 40));

  connect(ui->buttonBox, SIGNAL(clicked(QAbstractButton*)),
      this, SLOT(buttonClicked(QAbstractButton*)));
  connect(ui->enableZooming, &QCheckBox::checkStateChanged,
          this, &AppearanceSettingsDialog::onEnableZoomingChanged);

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

void AppearanceSettingsDialog::onEnableZoomingChanged() {
  const auto enableZooming = ui->enableZooming->isChecked();
  ui->maxSize->setEnabled(enableZooming);
  if (enableZooming) {
    ui->maxSize->setValue(prevMaxIconSize_);
  } else {
    prevMaxIconSize_ = ui->maxSize->value();
    ui->maxSize->setValue(ui->minSize->value());
  }
}

void AppearanceSettingsDialog::loadData() {
  const auto enableZooming = model_->minIconSize() < model_->maxIconSize();
  ui->enableZooming->setChecked(enableZooming);
  ui->zoomingAnimationSpeed->setValue(model_->zoomingAnimationSpeed());
  ui->minSize->setValue(model_->minIconSize());
  ui->maxSize->setValue(model_->maxIconSize());
  ui->maxSize->setEnabled(enableZooming);
  prevMaxIconSize_ = model_->maxIconSize();

  ui->spacingFactor->setValue(model_->spacingFactor());
  QColor backgroundColor = model_->isGlass()
      ? model_->backgroundColor()
      : model_->isFlat2D()
          ? model_->backgroundColor2D()
          : model_->backgroundColorMetal2D();
  backgroundColor_->setColor(QColor(backgroundColor.rgb()));
  ui->backgroundTransparency->setValue(alphaFToTransparencyPercent(backgroundColor.alphaF()));
  borderColor_->setColor(model_->isGlass() ? model_->borderColor() : model_->borderColorMetal2D());
  borderColor_->setVisible(!model_->isFlat2D());
  ui->borderColorLabel->setVisible(!model_->isFlat2D());
  activeIndicatorColor_->setColor(
      model_->isGlass() ? model_->activeIndicatorColor()
                        : model_->isFlat2D() ? model_->activeIndicatorColor2D()
                                             : model_->activeIndicatorColorMetal2D());
  inactiveIndicatorColor_->setColor(
      model_->isGlass() ? model_->inactiveIndicatorColor()
                        : model_->isFlat2D() ? model_->inactiveIndicatorColor2D()
                                             : model_->inactiveIndicatorColorMetal2D());
  ui->showTooltip->setChecked(model_->showTooltip());
  ui->tooltipFontSize->setValue(model_->tooltipFontSize());
  ui->floatingMargin->setValue(model_->floatingMargin());
  ui->floatingMargin->setEnabled(model_->isFloating());
  ui->bouncingLauncherIcon->setChecked(model_->bouncingLauncherIcon());
  ui->hoverGlow->setChecked(model_->hoverGlow());
  ui->hoverGlowAlpha->setValue(model_->hoverGlowAlpha());
}

void AppearanceSettingsDialog::resetData() {
  const auto enableZooming = kDefaultMinSize < kDefaultMaxSize;
  ui->enableZooming->setChecked(enableZooming);
  ui->zoomingAnimationSpeed->setValue(kDefaultZoomingAnimationSpeed);
  ui->minSize->setValue(kDefaultMinSize);
  ui->maxSize->setValue(kDefaultMaxSize);
  ui->maxSize->setEnabled(enableZooming);

  ui->spacingFactor->setValue(kDefaultSpacingFactor);
  backgroundColor_->setColor(QColor(
      model_->isGlass() ? kDefaultBackgroundColor
                       : model_->isFlat2D() ? kDefaultBackgroundColor2D
                                            : kDefaultBackgroundColorMetal2D));
  ui->backgroundTransparency->setValue(alphaFToTransparencyPercent(
      model_->isMetal2D() ? kDefaultBackgroundAlphaMetal2D : kDefaultBackgroundAlpha));
  borderColor_->setColor(QColor(model_->isGlass()
      ? kDefaultBorderColor : kDefaultBorderColorMetal2D));
  activeIndicatorColor_->setColor(QColor(
      model_->isGlass() ? kDefaultActiveIndicatorColor
                        : model_->isFlat2D() ? kDefaultActiveIndicatorColor2D
                                          : kDefaultActiveIndicatorColorMetal2D));
  inactiveIndicatorColor_->setColor(QColor(
      model_->isGlass() ? kDefaultInactiveIndicatorColor
                        : model_->isFlat2D() ? kDefaultInactiveIndicatorColor2D
                                            : kDefaultInactiveIndicatorColorMetal2D));
  ui->showTooltip->setChecked(kDefaultShowTooltip);
  ui->tooltipFontSize->setValue(kDefaultTooltipFontSize);
  ui->floatingMargin->setValue(kDefaultFloatingMargin);
  ui->bouncingLauncherIcon->setChecked(kDefaultBouncingLauncherIcon);
  ui->hoverGlow->setChecked(kDefaultHoverGlow);
  ui->hoverGlowAlpha->setValue(kDefaultHoverGlowAlpha);
}

void AppearanceSettingsDialog::saveData() {
  model_->setMinIconSize(ui->minSize->value());
  model_->setMaxIconSize(ui->maxSize->value());
  model_->setZoomingAnimationSpeed(ui->zoomingAnimationSpeed->value());

  model_->setSpacingFactor(ui->spacingFactor->value());
  QColor backgroundColor(backgroundColor_->color());
  backgroundColor.setAlphaF(transparencyPercentToAlphaF(ui->backgroundTransparency->value()));
  if (model_->isGlass()) {
    model_->setBackgroundColor(backgroundColor);
  } else if (model_->isFlat2D()) {
    model_->setBackgroundColor2D(backgroundColor);
  } else {
    model_->setBackgroundColorMetal2D(backgroundColor);
  }
  model_->setBorderColor(borderColor_->color());
  if (model_->isGlass()) {
    model_->setActiveIndicatorColor(activeIndicatorColor_->color());
  } else if (model_->isFlat2D()) {
    model_->setActiveIndicatorColor2D(activeIndicatorColor_->color());
  } else {
    model_->setActiveIndicatorColorMetal2D(activeIndicatorColor_->color());
  }
  if (model_->isGlass()) {
    model_->setInactiveIndicatorColor(inactiveIndicatorColor_->color());
  } else if (model_->isFlat2D()) {
    model_->setInactiveIndicatorColor2D(inactiveIndicatorColor_->color());
  } else {
    model_->setInactiveIndicatorColorMetal2D(inactiveIndicatorColor_->color());
  }
  model_->setShowTooltip(ui->showTooltip->isChecked());
  model_->setTooltipFontSize(ui->tooltipFontSize->value());
  model_->setFloatingMargin(ui->floatingMargin->value());
  model_->setBouncingLauncherIcon(ui->bouncingLauncherIcon->isChecked());
  model_->setHoverGlow(ui->hoverGlow->isChecked());
  model_->setHoverGlowAlpha(ui->hoverGlowAlpha->value());
  model_->saveAppearanceConfig();
}

}  // namespace crystaldock
