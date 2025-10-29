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
                             QPainter* painter, bool simplified = false) {
  painter->setPen(borderColor);
  const int delta = simplified ? 2 * borderWidth : 1;
  for (int i = -borderWidth; i <= borderWidth; i += delta) {
    for (int j = -borderWidth; j <= borderWidth; j += delta) {
      if (i != 0 || j != 0) {
        painter->drawText(x + i, y + j, text);
      }
    }
  }

  painter->setPen(textColor);
  painter->drawText(x, y, text);
}

inline void drawBorderedText(int x, int y, int width, int height, int flags,
                             const QString& text, int borderWidth,
                             QColor borderColor, QColor textColor,
                             QPainter* painter, bool simplified = false) {
  painter->setPen(borderColor);
  const int delta = simplified ? 2 * borderWidth : 1;
  for (int i = -borderWidth; i <= borderWidth; i += delta) {
    for (int j = -borderWidth; j <= borderWidth; j += delta) {
      if (i != 0 || j != 0) {
        painter->drawText(x + i, y + j, width, height, flags, text);
      }
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

inline void drawDarkenedIcon(const QPixmap& icon, int left, int top, QPainter* painter,
                             float darkenFactor = 0.6) {
  QImage iconImage = icon.toImage().convertToFormat(QImage::Format_ARGB32);

  for (int y = 0; y < iconImage.height(); ++y) {
    QRgb* line = reinterpret_cast<QRgb*>(iconImage.scanLine(y));
    for (int x = 0; x < iconImage.width(); ++x) {
      QRgb pixel = line[x];
      int alpha = qAlpha(pixel);
      if (alpha > 0) {
        int r = qRed(pixel) * darkenFactor;
        int g = qGreen(pixel) * darkenFactor;
        int b = qBlue(pixel) * darkenFactor;
        line[x] = qRgba(r, g, b, alpha);
      }
    }
  }
  
  painter->drawImage(left, top, iconImage);
}

inline void drawGlowingIcon(const QPixmap& icon, int left, int top, QPainter* painter,
                            QColor glowColor, float glowAlpha) {
  QImage iconImage = icon.toImage().convertToFormat(QImage::Format_ARGB32);
  
  painter->save();
  painter->setCompositionMode(QPainter::CompositionMode_SourceAtop);
  
  QImage brightOverlay = iconImage.copy();

  for (int y = 0; y < brightOverlay.height(); ++y) {
    QRgb* line = reinterpret_cast<QRgb*>(brightOverlay.scanLine(y));
    for (int x = 0; x < brightOverlay.width(); ++x) {
      QRgb pixel = line[x];
      int pixelAlpha = qAlpha(pixel);
      if (pixelAlpha > 0) {
        int finalAlpha = pixelAlpha * glowAlpha;
        line[x] = qRgba(glowColor.red(), glowColor.green(), glowColor.blue(), finalAlpha);
      }
    }
  }
  
  painter->drawImage(left, top, brightOverlay);
  painter->restore();
}

inline void fillRoundedRect(int x, int y, int width, int height, int radius, bool showBorder,
                            QColor borderColor, QColor fillColor, QPainter* painter) {
  painter->setRenderHint(QPainter::Antialiasing);
  QPainterPath border;
  border.addRoundedRect(x + 0.5, y + 0.5, width, height, radius, radius);
  painter->fillPath(border, QBrush(fillColor));
  if (showBorder) {
    painter->setPen(borderColor);
    painter->drawPath(border);
  }
  painter->setRenderHint(QPainter::Antialiasing, false);
}

inline void draw3dDockPanel(int x, int y, int width, int height, int radius,
                            QColor borderColor, QColor fillColor, QPainter* painter) {
  painter->setRenderHint(QPainter::Antialiasing);
  QPainterPath surface;
  QPolygonF polygon;
  polygon << QPointF(x + height / 2, y + height / 2)
          << QPointF(x + width - height / 2, y + height / 2)
          << QPointF(x + width, y + height) << QPointF(x, y + height);
  surface.addPolygon(polygon);
  surface.closeSubpath();
  painter->fillPath(surface, QBrush(fillColor));
  painter->setPen(borderColor);
  painter->drawPath(surface);

  QPainterPath side;
  QPolygonF polygon2;
  polygon2 << QPointF(x + height / 2, y + height / 2)
          << QPointF(x + height / 2, y + height / 2 + 2)
          << QPointF(x, y + height + 2) << QPointF(x, y + height) ;
  side.addPolygon(polygon2);
  side.closeSubpath();
  QPolygonF polygon3;
  polygon3 << QPointF(x + width - height / 2, y + height / 2)
          << QPointF(x + width - height / 2, y + height / 2 + 2)
          << QPointF(x + width, y + height + 2) << QPointF(x + width, y + height);
  side.addPolygon(polygon3);
  side.closeSubpath();
  painter->fillPath(side, QBrush(fillColor));

  painter->fillRect(x + height / 2, y + height / 2, width - height, 2, QBrush(fillColor));
  painter->fillRect(x, y + height, width, 3, QBrush(borderColor));
  painter->setRenderHint(QPainter::Antialiasing, false);
}

inline void fillCircle(int x, int y, int width, int height, QColor bgColor, QPainter* painter) {
  QColor fillColor = bgColor;
  fillColor.setAlphaF(1.0);
  painter->setRenderHint(QPainter::Antialiasing);
  QPainterPath border;
  border.addEllipse(x + 0.5, y + 0.5, width, height);
  painter->fillPath(border, QBrush(fillColor));
  painter->setRenderHint(QPainter::Antialiasing, false);
}

inline void drawIndicator(Qt::Orientation orientation, int hx, int hy, int vx, int vy,
                          int size, int thickness, QColor baseColor, QPainter* painter) {
  for (int i = 0; i <= size; i++) {
    int brightness = 100 - (2 * i - size) * (2 * i - size) * 100 / (size * size);
    if (brightness < 10) { brightness = 10; }
    QColor color = baseColor.lighter(brightness * 16 / 10);
    if (orientation == Qt::Horizontal) {
      painter->fillRect(hx - size / 2 + i, hy, 1, thickness, color);
      painter->fillRect(hx - size / 2 + i, hy, 1, 1, color.darker(300));
      painter->fillRect(hx - size / 2 + i, hy + thickness, 1, 1, color.darker(300));
    } else {  // Vertical.
      painter->fillRect(vx, vy - size / 2 + i, thickness, 1, color);
      painter->fillRect(vx, vy - size / 2 + i, 1, 1, color.darker(300));
      painter->fillRect(vx + thickness, vy - size / 2 + i, 1, 1, color.darker(300));
    }
  }
}

inline void drawIndicatorFlat2D(Qt::Orientation orientation, int hx, int hy, int vx, int vy,
                                int size, QColor baseColor, QPainter* painter) {
  if (orientation == Qt::Horizontal) {
    fillCircle(hx - size / 2, hy, size, size, baseColor, painter);
  } else {
    fillCircle(vx, vy - size / 2, size, size, baseColor, painter);
  }
}

inline void drawIndicatorMetal2D(PanelPosition panelPosition, int hx, int hy, int vx, int vy,
                                 int size, QColor baseColor, QPainter* painter) {
  painter->setPen(baseColor);
  painter->setBrush(baseColor);
  if (panelPosition == PanelPosition::Top) {
    QPoint points[3] = {
      QPoint(hx, hy + size / 2),
      QPoint(hx - size / 2, hy),
      QPoint(hx + size / 2, hy)
    };
    painter->drawPolygon(points, 3);
  } else if (panelPosition == PanelPosition::Bottom) {
    QPoint points[3] = {
      QPoint(hx, hy),
      QPoint(hx - size / 2, hy + size / 2),
      QPoint(hx + size / 2, hy + size / 2)
    };
    painter->drawPolygon(points, 3);
  } else if (panelPosition == PanelPosition::Left) {
    QPoint points[3] = {
      QPoint(vx + size / 2, vy),
      QPoint(vx, vy - size / 2),
      QPoint(vx, vy + size / 2)
    };
    painter->drawPolygon(points, 3);
  } else if (panelPosition == PanelPosition::Right) {
    QPoint points[3] = {
      QPoint(vx, vy),
      QPoint(vx + size / 2, vy - size / 2),
      QPoint(vx + size / 2, vy + size / 2)
    };
    painter->drawPolygon(points, 3);
  }
  painter->setBrush(Qt::transparent);
}

inline void drawFallbackIcon(int left, int top, int size,
                             QColor borderColor, QColor fillColor, QPainter* painter) {
  painter->setRenderHint(QPainter::Antialiasing);
  painter->setPen(borderColor);
  painter->setBrush(fillColor);
  painter->drawEllipse(left, top, size, size);
  painter->setBrush(Qt::NoBrush);
  painter->setPen(fillColor.lighter());
  painter->drawEllipse(left + 1, top + 1, size - 2, size - 2);
  painter->setPen(fillColor);
  painter->drawEllipse(left + 2, top + 2, size - 4, size - 4);
  painter->setRenderHint(QPainter::Antialiasing, false);
}

}  // namespace crystaldock

#endif  // CRYSTALDOCK_DRAW_UTILS_H_
