/*
 * This file is part of QDockX.
 * Copyright (C) 2022 Viet Dang (dangvd@gmail.com)
 *
 * QDockX is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * QDockX is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with QDockX.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "add_panel_dialog.h"
#include "ui_add_panel_dialog.h"

#include <memory>

#include <QPushButton>
#include <QTemporaryDir>
#include <QtTest>

namespace qdockx {

constexpr int kDockId = 1;

class AddPanelDialogTest: public QObject {
  Q_OBJECT

 private slots:
  // Tests OK button in Add mode.
  void add_ok();

  // Tests Cancel button in Add mode.
  void add_cancel();

  // Tests OK button in Clone mode.
  void clone_ok();

  // Tests Cancel button in Clone mode.
  void clone_cancel();

  // Tests OK button in Welcome mode.
  void welcome_ok();

  // Tests Cancel button in Welcome mode.
  void welcome_cancel();

 private:
  void init(AddPanelDialog::Mode mode) {
    QTemporaryDir configDir;
    model_ = std::make_unique<MultiDockModel>(configDir.path());
    if (mode != AddPanelDialog::Mode::Welcome) {
      model_->addDock();
    }
    int dockId = (mode == AddPanelDialog::Mode::Clone) ? kDockId : 0;
    dialog_ = std::make_unique<AddPanelDialog>(nullptr, model_.get(), dockId);
    dialog_->setMode(mode);
  }

  std::unique_ptr<MultiDockModel> model_;
  std::unique_ptr<AddPanelDialog> dialog_;
};

void AddPanelDialogTest::add_ok() {
  init(AddPanelDialog::Mode::Add);
  QTest::mouseClick(dialog_->ui->buttonBox->button(QDialogButtonBox::Ok),
                    Qt::LeftButton);
  QCOMPARE(model_->dockCount(), 2);
}

void AddPanelDialogTest::add_cancel() {
  init(AddPanelDialog::Mode::Add);
  QTest::mouseClick(dialog_->ui->buttonBox->button(QDialogButtonBox::Cancel),
                    Qt::LeftButton);
  QCOMPARE(model_->dockCount(), 1);
}

void AddPanelDialogTest::clone_ok() {
  init(AddPanelDialog::Mode::Clone);
  QTest::mouseClick(dialog_->ui->buttonBox->button(QDialogButtonBox::Ok),
                    Qt::LeftButton);
  QCOMPARE(model_->dockCount(), 2);
}

void AddPanelDialogTest::clone_cancel() {
  init(AddPanelDialog::Mode::Clone);
  QTest::mouseClick(dialog_->ui->buttonBox->button(QDialogButtonBox::Cancel),
                    Qt::LeftButton);
  QCOMPARE(model_->dockCount(), 1);
}

void AddPanelDialogTest::welcome_ok() {
  init(AddPanelDialog::Mode::Welcome);
  QTest::mouseClick(dialog_->ui->buttonBox->button(QDialogButtonBox::Ok),
                    Qt::LeftButton);
  QCOMPARE(model_->dockCount(), 1);
}

void AddPanelDialogTest::welcome_cancel() {
  init(AddPanelDialog::Mode::Welcome);
  QTest::mouseClick(dialog_->ui->buttonBox->button(QDialogButtonBox::Cancel),
                    Qt::LeftButton);
  QCOMPARE(model_->dockCount(), 0);
}

}  // namespace qdockx

QTEST_MAIN(qdockx::AddPanelDialogTest)
#include "add_panel_dialog_test.moc"
