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

#ifndef CRYSTALDOCK_DRAW_UTILS_H_
#define CRYSTALDOCK_DRAW_UTILS_H_

#include <QBrush>
#include <QColor>
#include <QPainter>
#include <QPainterPath>
#include <QRect>
#include <QString>

namespace crystaldock {

inline void drawBorderedText(int x, int y, const QString& text, int borderWidth,
                             QColor borderColor, QColor textColor,
                             QPainter* painter) {
  painter->setPen(borderColor);
  for (int i = -borderWidth; i <= borderWidth; ++i) {
    for (int j = -borderWidth; j <= borderWidth; ++j) {
      painter->drawText(x + i, y + j, text);
    }
  }

  painter->setPen(textColor);
  painter->drawText(x, y, text);
}

inline void drawBorderedText(int x, int y, int width, int height, int flags,
                             const QString& text, int borderWidth,
                             QColor borderColor, QColor textColor,
                             QPainter* painter) {
  painter->setPen(borderColor);
  for (int i = -borderWidth; i <= borderWidth; ++i) {
    for (int j = -borderWidth; j <= borderWidth; ++j) {
      painter->drawText(x + i, y + j, width, height, flags, text);
    }
  }

  painter->setPen(textColor);
  painter->drawText(x, y, width, height, flags, text);
}

inline void drawHighlightedIcon(QColor bgColor, int left, int top, int width, int height,
                                int padding, int roundedRectRadius, QPainter* painter,
                                float alpha = 0.42) {
  painter->setRenderHint(QPainter::Antialiasing);
  QColor fillColor = bgColor.lighter(500);
  fillColor.setAlphaF(alpha);
  QPainterPath path;
  path.addRoundedRect(
      QRect(left - padding, top - padding, width + 2 * padding, height + 2 * padding),
      roundedRectRadius, roundedRectRadius);
  painter->fillPath(path, QBrush(fillColor));
  painter->setRenderHint(QPainter::Antialiasing, false);
}

inline void fillRoundedRect(int x, int y, int width, int height, int radius, bool showBorder,
                            QColor borderColor, QColor fillColor, QPainter* painter) {
  painter->setRenderHint(QPainter::Antialiasing);
  QPainterPath border;
  border.addRoundedRect(x, y, width, height, radius, radius);
  painter->fillPath(border, QBrush(fillColor));
  if (showBorder) {
    painter->setPen(borderColor);
    painter->drawPath(border);
  }
  painter->setRenderHint(QPainter::Antialiasing, false);
}

}  // namespace crystaldock

#endif  // CRYSTALDOCK_DRAW_UTILS_H_
