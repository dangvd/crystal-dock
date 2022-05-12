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

namespace crystaldock {

AddPanelDialog::AddPanelDialog(QWidget* parent, MultiDockModel* model,
                               int dockId)
    : QDialog(parent),
      ui(new Ui::AddPanelDialog),
      model_(model),
      dockId_(dockId),
      isSingleScreen_(true) {
  ui->setupUi(this);

  // Populate screen list.
  const int screenCount = QGuiApplication::screens().size();
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

  const int deltaY = (mode == Mode::Clone) ? 220 : 0;
  ui->positionLabel->move(40, 280 - deltaY);
  ui->position->move(290, 270 - deltaY);
  ui->screenLabel->move(40, 320 - deltaY);
  ui->screen->move(290, 320 - deltaY);
  ui->buttonBox->move(20, 390 - deltaY);
  resize(440, 450 - deltaY);

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
    model_->addDock(
        position, screen, ui->showApplicationMenu->isChecked(),
        ui->showPager->isChecked(), ui->showTaskManager->isChecked(),
        ui->showClock->isChecked());
  }
}

}  // namespace crystaldock
