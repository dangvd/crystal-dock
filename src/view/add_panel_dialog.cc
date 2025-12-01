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

#include "add_panel_dialog.h"
#include "ui_add_panel_dialog.h"

#include <QGuiApplication>

#include <display/window_system.h>

namespace crystaldock {

namespace {

void moveY(QWidget* widget, int deltaY) {
  widget->move(widget->x(), widget->y() + deltaY);
}

void resizeHeight(QWidget* widget, int deltaHeight) {
  widget->resize(widget->width(), widget->height() + deltaHeight);
}

}  // namespace

AddPanelDialog::AddPanelDialog(QWidget* parent, MultiDockModel* model,
                               int dockId)
    : QDialog(parent),
      ui(new Ui::AddPanelDialog),
      model_(model),
      dockId_(dockId),
      isSingleScreen_(true) {
  ui->setupUi(this);
  setWindowFlag(Qt::Tool);

  // Populate screen list.
  const int screenCount = WindowSystem::screens().size();
  for (int i = 1; i <= screenCount; ++i) {
    ui->screen->addItem(QString::number(i));
  }
  ui->screen->setCurrentIndex(0);

  // Adjust the UI for single/multi-screen.
  isSingleScreen_ = (screenCount == 1);
  if (isSingleScreen_) {
    ui->screenLabel->setVisible(false);
    ui->screen->setVisible(false);
  }
}

AddPanelDialog::~AddPanelDialog() {
  delete ui;
}

void AddPanelDialog::setMode(Mode mode) {
  mode_ = mode;
  // reset positions of fields and size.
  ui->showTaskManager->move(120, 180);
  ui->showTrash->move(120, 220);
  ui->showWifiManager->move(120, 260);
  ui->showVolumeControl->move(120, 300);
  ui->showBatteryIndicator->move(120, 340);
  ui->showKeyboardLayout->move(120, 380);
  ui->showVersionChecker->move(120, 420);
  ui->showClock->move(120, 460);
  ui->styleLabel->move(90, 520);
  ui->style->move(320, 505);
  ui->positionLabel->move(90, 560);
  ui->position->move(320, 550);
  ui->screenLabel->move(90, 600);
  ui->screen->move(320, 595);
  ui->buttonBox->move(70, 670);
  resize(540, 730);

  setWindowTitle((mode_ == Mode::Add)
                 ? QString("Add Panel") : (mode_ == Mode::Clone)
                    ? QString("Clone Panel") : "Welcome to Crystal Dock!");

  ui->headerLabel->setText((mode == Mode::Welcome)
                           ? QString("Please set up your first panel.")
                           : QString("Please set up your new panel."));

  ui->showApplicationMenu->setChecked(mode == Mode::Welcome);
  ui->showPager->setChecked(false);
  ui->showTaskManager->setChecked(mode == Mode::Welcome);
  ui->showTrash->setChecked(mode == Mode::Welcome);
  ui->showWifiManager->setChecked(mode == Mode::Welcome);
  ui->showVolumeControl->setChecked(mode == Mode::Welcome);
  ui->showBatteryIndicator->setChecked(mode == Mode::Welcome);
  ui->showKeyboardLayout->setChecked(mode == Mode::Welcome);
  ui->showVersionChecker->setChecked(mode == Mode::Welcome);
  ui->showClock->setChecked(mode == Mode::Welcome);

  ui->componentsLabel->setVisible(mode != Mode::Clone);
  ui->showApplicationMenu->setVisible(mode != Mode::Clone);
  ui->showPager->setVisible(mode != Mode::Clone);
  ui->showTaskManager->setVisible(mode != Mode::Clone);
  ui->showTrash->setVisible(mode != Mode::Clone);
  ui->showWifiManager->setVisible(mode != Mode::Clone);
  ui->showVolumeControl->setVisible(mode != Mode::Clone);
  ui->showBatteryIndicator->setVisible(mode != Mode::Clone);
  ui->showKeyboardLayout->setVisible(mode != Mode::Clone);
  ui->showVersionChecker->setVisible(mode != Mode::Clone);
  ui->showClock->setVisible(mode != Mode::Clone);

  if (mode != Mode::Clone && !WindowSystem::hasVirtualDesktopManager()) {
    ui->showPager->setChecked(false);
    ui->showPager->setVisible(false);
    constexpr int kDeltaY = -40;
    moveY(ui->showTaskManager, kDeltaY);
    moveY(ui->showTrash, kDeltaY);
    moveY(ui->showWifiManager, kDeltaY);
    moveY(ui->showVolumeControl, kDeltaY);
    moveY(ui->showBatteryIndicator, kDeltaY);
    moveY(ui->showKeyboardLayout, kDeltaY);
    moveY(ui->showVersionChecker, kDeltaY);
    moveY(ui->showClock, kDeltaY);
    moveY(ui->styleLabel, kDeltaY);
    moveY(ui->style, kDeltaY);
    moveY(ui->positionLabel, kDeltaY);
    moveY(ui->position, kDeltaY);
    moveY(ui->screenLabel, kDeltaY);
    moveY(ui->screen, kDeltaY);
    moveY(ui->buttonBox, kDeltaY);
    resizeHeight(this, kDeltaY);
  }

  ui->styleLabel->setVisible(mode == Mode::Welcome);
  ui->style->setVisible(mode == Mode::Welcome);
  if (mode == Mode::Welcome) {
    ui->buttonBox->setStandardButtons(QDialogButtonBox::Ok);
    ui->screenLabel->setVisible(false);
    ui->screen->setVisible(false);
  } else {
    ui->buttonBox->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  }

  if (mode == Mode::Clone) {
    constexpr int kDeltaY = -460;
    moveY(ui->positionLabel, kDeltaY);
    moveY(ui->position, kDeltaY);
    moveY(ui->screenLabel, kDeltaY);
    moveY(ui->screen, kDeltaY);
    moveY(ui->buttonBox, kDeltaY);
    resizeHeight(this, kDeltaY);
  } else if (mode != Mode::Welcome) {
    ui->styleLabel->setVisible(false);
    ui->style->setVisible(false);
    constexpr int kDeltaY = -40;
    moveY(ui->positionLabel, kDeltaY);
    moveY(ui->position, kDeltaY);
    moveY(ui->screenLabel, kDeltaY);
    moveY(ui->screen, kDeltaY);
    moveY(ui->buttonBox, kDeltaY);
    resizeHeight(this, kDeltaY);
  }

  if (isSingleScreen_) {
    constexpr int kScreenDeltaY = -40;
    moveY(ui->buttonBox, kScreenDeltaY);
    resizeHeight(this, kScreenDeltaY);
  }
}

void AddPanelDialog::accept() {
  QDialog::accept();
  auto position = static_cast<PanelPosition>(ui->position->currentIndex());
  auto screen = ui->screen->currentIndex();
  if (mode_ == Mode::Clone) {
    model_->cloneDock(dockId_, position, screen);
  } else {
    if (mode_ == Mode::Welcome) {
      const auto& style = ui->style->currentText();
      model_->setPanelStyle(style == "Glass 3D"
          ? PanelStyle::Glass3D_Floating
          : style == "Glass 2D"
              ? PanelStyle::Glass2D_Floating
              : style == "Flat 2D"
                  ? PanelStyle::Flat2D_Floating
                  : PanelStyle::Metal2D_NonFloating);
    }
    model_->addDock(
        position, screen, ui->showApplicationMenu->isChecked(),
        ui->showPager->isChecked(), ui->showTaskManager->isChecked(),
        ui->showTrash->isChecked(), ui->showWifiManager->isChecked(),
        ui->showVolumeControl->isChecked(), ui->showBatteryIndicator->isChecked(),
        ui->showKeyboardLayout->isChecked(), ui->showVersionChecker->isChecked(),
        ui->showClock->isChecked());
    model_->maybeAddDockForMultiScreen();
  }
}

}  // namespace crystaldock
