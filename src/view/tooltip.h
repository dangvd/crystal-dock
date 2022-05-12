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

#ifndef CRYSTALDOCK_TOOLTIP_H_
#define CRYSTALDOCK_TOOLTIP_H_

#include <QColor>
#include <QFont>
#include <QPaintEvent>
#include <QString>
#include <QWidget>

namespace crystaldock {

// Tooltip with translucent background.
class Tooltip : public QWidget {
 public:
  Tooltip();

  void setText(const QString& text);
  void setFontFace(const QString& fontFace);
  void setFontItalic(bool val);
  void setFontBold(bool val);
  void setFontSize(int size);
  void setFontColor(const QColor& color);
  void setBackgroundColor(const QColor& color);

  void updateLayout();

 protected:
  virtual void paintEvent(QPaintEvent* e) override;

 private:
  // Additional padding around the text e.g. to avoid clipping.
  static const int kPadding = 10;

  QString text_;
  QFont font_;
  QColor fontColor_;
  QColor backgroundColor_;
};

}  // namespace crystaldock

#endif  // CRYSTALDOCK_TOOLTIP_H_
