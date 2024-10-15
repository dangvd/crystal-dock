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

  setWindowTitle((mode_ == Mode::Add)
                 ? QString("Add Panel") : (mode_ == Mode::Clone)
                    ? QString("Clone Panel") : "Welcome to Crystal Dock!");

  ui->headerLabel->setText((mode == Mode::Welcome)
                           ? QString("Please set up your first panel.")
                           : QString("Please set up your new panel."));

  ui->showApplicationMenu->setChecked(mode == Mode::Welcome);
  ui->showPager->setChecked(mode == Mode::Welcome);
  ui->showTaskManager->setChecked(mode == Mode::Welcome);
  ui->showClock->setChecked(mode == Mode::Welcome);

  ui->componentsLabel->setVisible(mode != Mode::Clone);
  ui->showApplicationMenu->setVisible(mode != Mode::Clone);
  ui->showPager->setVisible(mode != Mode::Clone);
  ui->showTaskManager->setVisible(mode != Mode::Clone);
  ui->showClock->setVisible(mode != Mode::Clone);

  ui->styleLabel->setVisible(mode == Mode::Welcome);
  ui->style->setVisible(mode == Mode::Welcome);
  if (mode == Mode::Welcome) {
    ui->buttonBox->setStandardButtons(QDialogButtonBox::Ok);
    ui->screenLabel->setVisible(false);
    ui->screen->setVisible(false);
  } else {
    ui->buttonBox->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  }

  const int deltaY = (mode == Mode::Clone) ? 220 : 0;
  ui->positionLabel->move(90, 320 - deltaY);
  ui->position->move(320, 310 - deltaY);
  ui->screenLabel->move(90, 360 - deltaY);
  ui->screen->move(320, 355 - deltaY);
  ui->buttonBox->move(70, 430 - deltaY);
  resize(540, 490 - deltaY);

  // Adjust the UI for single/multi-screen.
  if (isSingleScreen_) {
    constexpr int kScreenDeltaY = 45;
    ui->buttonBox->move(ui->buttonBox->x(), ui->buttonBox->y() - kScreenDeltaY);
    resize(width(), height() - kScreenDeltaY);
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
      model_->setPanelStyle(
          style == "Glass 3D" ? PanelStyle::Glass3D_Floating
                              : style == "Flat 2D" ? PanelStyle::Flat2D_Floating
                                                   : PanelStyle::Metal2D_NonFloating);
    }
    model_->addDock(
        position, screen, ui->showApplicationMenu->isChecked(),
        ui->showPager->isChecked(), ui->showTaskManager->isChecked(),
        ui->showClock->isChecked());
    model_->maybeAddDockForMultiScreen();
  }
}

}  // namespace crystaldock
