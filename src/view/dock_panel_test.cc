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

#include "dock_panel.h"

#include <memory>

#include <QTemporaryDir>
#include <QtTest>

#include <KWindowSystem>

#include "multi_dock_view.h"

namespace qdockx {

constexpr int kDockId = 1;

class DockPanelTest: public QObject {
  Q_OBJECT

 private slots:
  void init() {
    QTemporaryDir configDir;
    model_ = std::make_unique<MultiDockModel>(configDir.path());
    model_->addDock();
    view_ = std::make_unique<MultiDockView>(model_.get());
    dock_ = std::make_unique<DockPanel>(view_.get(), model_.get(), kDockId);
  }

  // Tests setting position.
  void setPosition();

  // Tests setting Auto Hide on/off.
  void autoHide();

  // Tests toggling the application menu.
  void toggleApplicationMenu();

  // Tests toggling the pager.
  void togglePager();

  // Tests toggling the clock.
  void toggleClock();

 private:
  void verifyPosition(PanelPosition position) {
    QCOMPARE(dock_->position_, position);
    QCOMPARE(dock_->orientation_,
             (position == PanelPosition::Bottom
                  || position == PanelPosition::Top)
             ? Qt::Horizontal : Qt::Vertical);
    if (dock_->orientation_ == Qt::Horizontal) {
      QVERIFY(dock_->width() > dock_->height());
    } else {
      QVERIFY(dock_->width() < dock_->height());
    }
    QCOMPARE(dock_->positionTop_->isChecked(), position == PanelPosition::Top);
    QCOMPARE(dock_->positionBottom_->isChecked(),
             position == PanelPosition::Bottom);
    QCOMPARE(dock_->positionLeft_->isChecked(),
             position == PanelPosition::Left);
    QCOMPARE(dock_->positionRight_->isChecked(),
             position == PanelPosition::Right);
  }

  void verifyAutoHide(bool enabled) {
    QCOMPARE(dock_->autoHide(), enabled);
    QCOMPARE(dock_->visibilityAutoHideAction_->isChecked(), enabled);
    if (enabled) {
      QVERIFY(dock_->width() == 1 || dock_->height() == 1);
    }
  }

  void verifyApplicationMenu(bool enabled, int itemCount) {
    QCOMPARE(dock_->showApplicationMenu_, enabled);
    QCOMPARE(dock_->applicationMenuAction_->isChecked(), enabled);
    QCOMPARE(dock_->itemCount(), itemCount);
  }

  void verifyPager(bool enabled, int itemCount) {
    QCOMPARE(dock_->showPager_, enabled);
    QCOMPARE(dock_->pagerAction_->isChecked(), enabled);
    QCOMPARE(dock_->itemCount(), itemCount);
  }

  void verifyClock(bool enabled, int itemCount) {
    QCOMPARE(dock_->showClock_, enabled);
    QCOMPARE(dock_->clockAction_->isChecked(), enabled);
    QCOMPARE(dock_->itemCount(), itemCount);
  }

  std::unique_ptr<MultiDockModel> model_;
  std::unique_ptr<MultiDockView> view_;
  std::unique_ptr<DockPanel> dock_;
};

void DockPanelTest::setPosition() {
  verifyPosition(PanelPosition::Bottom);
  dock_->positionLeft_->trigger();
  verifyPosition(PanelPosition::Left);
  dock_->positionRight_->trigger();
  verifyPosition(PanelPosition::Right);
  dock_->positionTop_->trigger();
  verifyPosition(PanelPosition::Top);
  dock_->positionBottom_->trigger();
  verifyPosition(PanelPosition::Bottom);
}

void DockPanelTest::autoHide() {
  verifyAutoHide(false);
  dock_->visibilityAutoHideAction_->trigger();
  verifyAutoHide(true);
  dock_->visibilityAlwaysVisibleAction_->trigger();
  verifyAutoHide(false);
}

void DockPanelTest::toggleApplicationMenu() {
  const int itemCount = dock_->itemCount();
  dock_->applicationMenuAction_->trigger();
  verifyApplicationMenu(false, itemCount - 1);
  dock_->applicationMenuAction_->trigger();
  verifyApplicationMenu(true, itemCount);
}

void DockPanelTest::togglePager() {
  // TODO(dangvd)
}

void DockPanelTest::toggleClock() {
  const int itemCount = dock_->itemCount();
  dock_->clockAction_->trigger();
  verifyClock(false, itemCount - 1);
  dock_->clockAction_->trigger();
  verifyClock(true, itemCount);
}

}  // namespace qdockx

QTEST_MAIN(qdockx::DockPanelTest)
#include "dock_panel_test.moc"
