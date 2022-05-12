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

#include "application_menu_settings_dialog.h"
#include "ui_application_menu_settings_dialog.h"

#include <memory>

#include <QPushButton>
#include <QTemporaryDir>
#include <QtTest>

namespace qdockx {

class ApplicationMenuSettingsDialogTest: public QObject {
  Q_OBJECT

 private slots:
  void init() {
    QTemporaryDir configDir;
    model_ = std::make_unique<MultiDockModel>(configDir.path());
    model_->setApplicationMenuName("Applications");
    model_->setApplicationMenuIcon("start-here-kde");

    dialog_ = std::make_unique<ApplicationMenuSettingsDialog>(nullptr,
                                                              model_.get());
  }

  // Tests UI initialization.
  void uiInit();

  // Tests OK button/logic.
  void ok();

  // Tests Apply button/logic.
  void apply();

  // Tests Cancel button/logic.
  void cancel();

 private:
  std::unique_ptr<MultiDockModel> model_;
  std::unique_ptr<ApplicationMenuSettingsDialog> dialog_;
};

void ApplicationMenuSettingsDialogTest::uiInit() {
  QCOMPARE(dialog_->ui->name->text(), QString("Applications"));
  QCOMPARE(dialog_->icon_->icon(), QString("start-here-kde"));
}

void ApplicationMenuSettingsDialogTest::ok() {
  dialog_->ui->name->setText("Start");
  dialog_->icon_->setIcon("start-here");

  QTest::mouseClick(dialog_->ui->buttonBox->button(QDialogButtonBox::Ok),
                    Qt::LeftButton);

  // Tests that the model has been updated.
  QCOMPARE(model_->applicationMenuName(), QString("Start"));
  QCOMPARE(model_->applicationMenuIcon(), QString("start-here"));
}

void ApplicationMenuSettingsDialogTest::apply() {
  dialog_->ui->name->setText("Start");
  dialog_->icon_->setIcon("start-here");

  QTest::mouseClick(dialog_->ui->buttonBox->button(QDialogButtonBox::Apply),
                    Qt::LeftButton);

  // Tests that the model has been updated.
  QCOMPARE(model_->applicationMenuName(), QString("Start"));
  QCOMPARE(model_->applicationMenuIcon(), QString("start-here"));
}

void ApplicationMenuSettingsDialogTest::cancel() {
  dialog_->ui->name->setText("Start");
  dialog_->icon_->setIcon("start-here");

  QTest::mouseClick(dialog_->ui->buttonBox->button(QDialogButtonBox::Cancel),
                    Qt::LeftButton);

  // Tests that the model has not been updated.
  QCOMPARE(model_->applicationMenuName(), QString("Applications"));
  QCOMPARE(model_->applicationMenuIcon(), QString("start-here-kde"));
}

}  // namespace qdockx

QTEST_MAIN(qdockx::ApplicationMenuSettingsDialogTest)
#include "application_menu_settings_dialog_test.moc"
