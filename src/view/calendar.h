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

#ifndef CRYSTALDOCK_CALENDAR_H_
#define CRYSTALDOCK_CALENDAR_H_

#include <QCalendarWidget>
#include <QDialog>

namespace crystaldock {

// A calendar widget. This is shown when the user clicks on the clock.
class Calendar : public QDialog {
 public:
  Calendar(QWidget* parent);

  // Toggles showing the calendar.
  //
  // This also resets the selected date to the current date.
  void showCalendar();

 private:
  QCalendarWidget calendar_;
};

}  // namespace crystaldock

#endif  // CRYSTALDOCK_CALENDAR_H_
