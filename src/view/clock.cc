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

#include "clock.h"

#include <QColor>
#include <QDate>
#include <QFont>
#include <QIcon>
#include <QTime>
#include <QTimer>

#include "dock_panel.h"
#include <utils/draw_utils.h>
#include <utils/font_utils.h>

namespace crystaldock {

constexpr float Clock::kWhRatio;
constexpr float Clock::kDelta;

Clock::Clock(DockPanel* parent, MultiDockModel* model,
             Qt::Orientation orientation, int minSize, int maxSize)
    : IconlessDockItem(parent, model, "" /* label */, orientation, minSize, maxSize,
                       kWhRatio),
      calendar_(parent),
      fontFamilyGroup_(this) {
  createMenu();
  loadConfig();

  QTimer* timer = new QTimer(this);
  connect(timer, SIGNAL(timeout()), this, SLOT(updateTime()));
  timer->start(1000);  // update the time every second.
}

void Clock::draw(QPainter *painter) const {
  const QString timeFormat = model_->use24HourClock() ? "hh:mm" : "hh:mm AP";
  const QString time = QTime::currentTime().toString(timeFormat);
  // The reference time used to calculate the font size.
  const QString referenceTime = QTime(8, 8).toString(timeFormat);

  painter->setFont(adjustFontSize(getWidth(), getHeight(), referenceTime,
                                  model_->clockFontScaleFactor(),
                                  model_->clockFontFamily()));
  painter->setRenderHint(QPainter::TextAntialiasing);

  if (size_ > minSize_) {
    drawBorderedText(left_, top_, getWidth(), getHeight(), Qt::AlignCenter,
                     time, 1 /* borderWidth */, Qt::black, Qt::white, painter);
  } else {
    painter->setPen(Qt::white);
    painter->drawText(left_, top_, getWidth(), getHeight(), Qt::AlignCenter,
                      time);
  }
}

void Clock::mousePressEvent(QMouseEvent *e) {
  if (e->button() == Qt::LeftButton) {
    calendar_.showCalendar();
  } else if (e->button() == Qt::RightButton) {
    // In case other docks have changed the config.
    loadConfig();
    parent_->minimize();
    QTimer::singleShot(500, [this]{ menu_.exec(parent_->mapToGlobal(QPoint(left_, top_))); });
  }
}

QString Clock::getLabel() const {
  return QLocale::system().toString(QDate::currentDate(), QLocale::LongFormat);
}

void Clock::updateTime() {
  parent_->update();
}

void Clock::setFontScaleFactor(float fontScaleFactor) {
  largeFontAction_->setChecked(
      fontScaleFactor > kLargeClockFontScaleFactor - kDelta);
  mediumFontAction_->setChecked(
      fontScaleFactor > kMediumClockFontScaleFactor - kDelta &&
      fontScaleFactor < kMediumClockFontScaleFactor + kDelta);
  smallFontAction_->setChecked(
      fontScaleFactor < kSmallClockFontScaleFactor + kDelta);
}

void Clock::setLargeFont() {
  setFontScaleFactor(kLargeClockFontScaleFactor);
  saveConfig();
}

void Clock::setMediumFont() {
  setFontScaleFactor(kMediumClockFontScaleFactor);
  saveConfig();
}

void Clock::setSmallFont() {
  setFontScaleFactor(kSmallClockFontScaleFactor);
  saveConfig();
}

void Clock::createMenu() {
  use24HourClockAction_ = menu_.addAction(
      QString("Use 24-hour Clock"), this,
      [this] {
        saveConfig();
      });
  use24HourClockAction_->setCheckable(true);

  QMenu* fontSize = menu_.addMenu(QString("Font Size"));
  largeFontAction_ = fontSize->addAction(QString("Large Font"),
                                         this,
                                         SLOT(setLargeFont()));
  largeFontAction_->setCheckable(true);
  mediumFontAction_ = fontSize->addAction(QString("Medium Font"),
                                          this,
                                          SLOT(setMediumFont()));
  mediumFontAction_->setCheckable(true);
  smallFontAction_ = fontSize->addAction(QString("Small Font"),
                                         this,
                                         SLOT(setSmallFont()));
  smallFontAction_->setCheckable(true);

  QMenu* fontFamily = menu_.addMenu(QString("Font Family"));
  for (const auto& family : getBaseFontFamilies()) {
    auto fontFamilyAction = fontFamily->addAction(family, this, [this, family]{
      model_->setClockFontFamily(family);
      model_->saveAppearanceConfig(true /* repaintOnly */);
    });
    fontFamilyAction->setCheckable(true);
    fontFamilyAction->setActionGroup(&fontFamilyGroup_);
    fontFamilyAction->setChecked(family == model_->clockFontFamily());
  }

  menu_.addSeparator();
  parent_->addPanelSettings(&menu_);
}

void Clock::loadConfig() {
  use24HourClockAction_->setChecked(model_->use24HourClock());
  setFontScaleFactor(model_->clockFontScaleFactor());
}

void Clock::saveConfig() {
  model_->setUse24HourClock(use24HourClockAction_->isChecked());
  model_->setClockFontScaleFactor(fontScaleFactor());
  model_->saveAppearanceConfig(true /* repaintOnly */);
}

}  // namespace crystaldock
