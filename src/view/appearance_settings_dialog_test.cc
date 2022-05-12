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

#include "appearance_settings_dialog.h"
#include "ui_appearance_settings_dialog.h"

#include <cmath>
#include <cstdlib>
#include <memory>

#include <QPushButton>
#include <QTemporaryDir>
#include <QtTest>

namespace qdockx {

class AppearanceSettingsDialogTest: public QObject {
  Q_OBJECT

 private slots:
  void init() {
    QTemporaryDir configDir;
    model_ = std::make_unique<MultiDockModel>(configDir.path());
    model_->setMinIconSize(48);
    model_->setMaxIconSize(128);
    model_->setSpacingFactor(0.5);
    QColor color("white");
    color.setAlphaF(0.42);
    model_->setBackgroundColor(color);
    model_->setShowBorder(true);
    model_->setBorderColor(QColor("white"));
    model_->setTooltipFontSize(20);

    dialog_ = std::make_unique<AppearanceSettingsDialog>(nullptr, model_.get());
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
  static bool compareDouble(double x, double y) {
    static constexpr double kDelta = 0.01;
    return std::abs(x - y) < kDelta;
  }

  std::unique_ptr<MultiDockModel> model_;
  std::unique_ptr<AppearanceSettingsDialog> dialog_;
};

void AppearanceSettingsDialogTest::uiInit() {
  QCOMPARE(dialog_->ui->minSize->value(), 48);
  QCOMPARE(dialog_->ui->maxSize->value(), 128);
  compareDouble(dialog_->ui->spacingFactor->value(), 0.5);
  QCOMPARE(dialog_->backgroundColor_->color(), QColor("white"));
  QCOMPARE(dialog_->ui->backgroundTransparency->value(), 58);
  QCOMPARE(dialog_->ui->showBorder->isChecked(), true);
  QCOMPARE(dialog_->borderColor_->color(), QColor("white"));
  QCOMPARE(dialog_->ui->tooltipFontSize->value(), 20);
}

void AppearanceSettingsDialogTest::ok() {
  dialog_->ui->minSize->setValue(40);
  dialog_->ui->maxSize->setValue(80);
  dialog_->ui->spacingFactor->setValue(0.2);
  dialog_->ui->backgroundTransparency->setValue(90);
  dialog_->backgroundColor_->setColor(QColor("green"));
  dialog_->ui->showBorder->setChecked(false);
  dialog_->borderColor_->setColor(QColor("blue"));
  dialog_->ui->tooltipFontSize->setValue(24);

  QTest::mouseClick(dialog_->ui->buttonBox->button(QDialogButtonBox::Ok),
                    Qt::LeftButton);

  // Tests that the model has been updated.
  QCOMPARE(model_->minIconSize(), 40);
  QCOMPARE(model_->maxIconSize(), 80);
  compareDouble(dialog_->ui->spacingFactor->value(), 0.2);
  QCOMPARE(model_->backgroundColor().rgb(), QColor("green").rgb());
  compareDouble(model_->backgroundColor().alphaF(), 0.1);
  QCOMPARE(model_->showBorder(), false);
  QCOMPARE(model_->borderColor(), QColor("blue"));
  QCOMPARE(model_->tooltipFontSize(), 24);
}

void AppearanceSettingsDialogTest::apply() {
  dialog_->ui->minSize->setValue(40);
  dialog_->ui->maxSize->setValue(80);
  dialog_->ui->spacingFactor->setValue(0.2);
  dialog_->ui->backgroundTransparency->setValue(90);
  dialog_->backgroundColor_->setColor(QColor("green"));
  dialog_->ui->showBorder->setChecked(false);
  dialog_->borderColor_->setColor(QColor("blue"));
  dialog_->ui->tooltipFontSize->setValue(24);

  QTest::mouseClick(dialog_->ui->buttonBox->button(QDialogButtonBox::Apply),
                    Qt::LeftButton);

  // Tests that the model has been updated.
  QCOMPARE(model_->minIconSize(), 40);
  QCOMPARE(model_->maxIconSize(), 80);
  compareDouble(dialog_->ui->spacingFactor->value(), 0.2);
  QCOMPARE(model_->backgroundColor().rgb(), QColor("green").rgb());
  compareDouble(model_->backgroundColor().alphaF(), 0.1);
  QCOMPARE(model_->showBorder(), false);
  QCOMPARE(model_->borderColor(), QColor("blue"));
  QCOMPARE(model_->tooltipFontSize(), 24);
}

void AppearanceSettingsDialogTest::cancel() {
  dialog_->ui->minSize->setValue(40);
  dialog_->ui->maxSize->setValue(80);
  dialog_->ui->spacingFactor->setValue(0.2);
  dialog_->ui->backgroundTransparency->setValue(90);
  dialog_->backgroundColor_->setColor(QColor("green"));
  dialog_->ui->showBorder->setChecked(false);
  dialog_->borderColor_->setColor(QColor("blue"));
  dialog_->ui->tooltipFontSize->setValue(24);

  QTest::mouseClick(dialog_->ui->buttonBox->button(QDialogButtonBox::Cancel),
                    Qt::LeftButton);

  // Tests that the model has not been updated.
  QCOMPARE(model_->minIconSize(), 48);
  QCOMPARE(model_->maxIconSize(), 128);
  compareDouble(dialog_->ui->spacingFactor->value(), 0.5);
  QCOMPARE(model_->backgroundColor().rgb(), QColor("white").rgb());
  compareDouble(model_->backgroundColor().alphaF(), 0.42);
  QCOMPARE(model_->showBorder(), true);
  QCOMPARE(model_->borderColor(), QColor("white"));
  QCOMPARE(model_->tooltipFontSize(), 20);
}

}  // namespace qdockx

QTEST_MAIN(qdockx::AppearanceSettingsDialogTest)
#include "appearance_settings_dialog_test.moc"
