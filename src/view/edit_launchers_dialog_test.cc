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

#include "edit_launchers_dialog.h"
#include "ui_edit_launchers_dialog.h"

#include <memory>

#include <QPushButton>
#include <QTemporaryDir>
#include <QtTest>

#include <QSettings>
#include <QString>

namespace qdockx {

constexpr int kDockId = 1;
constexpr int kDefaultLauncherCount = 10;

class EditLaunchersDialogTest: public QObject {
  Q_OBJECT

 private slots:
  void init() {
    QTemporaryDir configDir;
    model_ = std::make_unique<MultiDockModel>(configDir.path());
    model_->addDock();
    dialog_ = std::make_unique<EditLaunchersDialog>(nullptr, model_.get(),
                                                    kDockId);
  }

  // Tests OK button/logic.
  void ok();

  // Tests Apply button/logic.
  void apply();

  // Tests Cancel button/logic.
  void cancel();

 private:
  int launcherCount() {
    return static_cast<int>(model_->dockLauncherConfigs(kDockId).size());
  }

  std::unique_ptr<MultiDockModel> model_;
  std::unique_ptr<EditLaunchersDialog> dialog_;
};

void EditLaunchersDialogTest::ok() {
  dialog_->addLauncher("Text Editor", "kate", "kate");
  QTest::mouseClick(dialog_->ui->buttonBox->button(QDialogButtonBox::Ok),
                    Qt::LeftButton);

  // Verify.
  QCOMPARE(launcherCount(), kDefaultLauncherCount + 1);
}

void EditLaunchersDialogTest::apply() {
  dialog_->addLauncher("Text Editor", "kate", "kate");
  QTest::mouseClick(dialog_->ui->buttonBox->button(QDialogButtonBox::Apply),
                    Qt::LeftButton);

  // Verify.
  QCOMPARE(launcherCount(), kDefaultLauncherCount + 1);
}

void EditLaunchersDialogTest::cancel() {
  dialog_->addLauncher("Text Editor", "kate", "kate");
  QTest::mouseClick(dialog_->ui->buttonBox->button(QDialogButtonBox::Cancel),
                    Qt::LeftButton);

  // Verify.
  QCOMPARE(launcherCount(), kDefaultLauncherCount);
}

}  // namespace qdockx

QTEST_MAIN(qdockx::EditLaunchersDialogTest)
#include "edit_launchers_dialog_test.moc"
