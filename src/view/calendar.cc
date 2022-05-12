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

#include "calendar.h"

#include <QDate>

#include <KWindowSystem>
#include <netwm_def.h>

namespace crystaldock {

Calendar::Calendar(QWidget* parent)
    : QDialog(parent),
      calendar_(this) {
  KWindowSystem::setState(winId(), NET::SkipTaskbar);
  setWindowTitle(QString("Calendar"));
  calendar_.setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);
  resize(calendar_.sizeHint());
}

void Calendar::toggleCalendar() {
  calendar_.setSelectedDate(QDate::currentDate());
  setVisible(!isVisible());
}

}  // namespace crystaldock
