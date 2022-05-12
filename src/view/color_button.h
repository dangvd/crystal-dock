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

#ifndef CRYSTALDOCK_COLOR_BUTTON_H_
#define CRYSTALDOCK_COLOR_BUTTON_H_

#include <QColor>
#include <QPushButton>

namespace crystaldock {

class ColorButton : public QPushButton
{
  Q_OBJECT
 public:
  ColorButton(QWidget *parent = nullptr);

  QColor color() { return color_; }
  void setColor(const QColor& icon);

 public slots:
  void showColorDialog();

 private:
  QColor color_;
};

}  // namespace crystaldock

#endif  // CRYSTALDOCK_COLOR_BUTTON_H_
