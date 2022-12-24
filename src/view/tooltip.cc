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

#include "tooltip.h"

#include <QApplication>
#include <QFontMetrics>
#include <QPainter>

#include <KWindowSystem>
#include <kx11extras.h>
#include <netwm_def.h>

#include <utils/draw_utils.h>

namespace crystaldock {

const int Tooltip::kPadding;

Tooltip::Tooltip() : QWidget(), font_(QApplication::font()) {
  setAttribute(Qt::WA_TranslucentBackground);
  KWindowSystem::setType(winId(), NET::Dock);
  KX11Extras::setOnAllDesktops(winId(), true);
}

void Tooltip::setFontColor(const QColor& color) {
  fontColor_ = color;
}

void Tooltip::setBackgroundColor(const QColor& color) {
  backgroundColor_ = color;
}

void Tooltip::setFontFace(const QString& fontFace) {
  font_.setFamily(fontFace);
}

void Tooltip::setFontItalic(bool val) {
  font_.setItalic(val);
}

void Tooltip::setFontBold(bool val) {
  font_.setBold(val);
}

void Tooltip::setFontSize(int size) {
  font_.setPointSize(size);
}

void Tooltip::setText(const QString& text) {
  text_ = text;
  updateLayout();
}

void Tooltip::updateLayout() {
  QFontMetrics metrics(font_);
  int w = metrics.horizontalAdvance(text_) + 2 * kPadding;
  int h = metrics.height() + 2 * kPadding;

  resize(w,h);
  update();
}

void Tooltip::paintEvent(QPaintEvent* e) {
  QPainter painter(this);
  painter.setRenderHint(QPainter::TextAntialiasing);
  QFontMetrics metrics(font_);
  const int kDeltaY = metrics.height() / 2;
  painter.setFont(font_);
  drawBorderedText(kPadding, kPadding + kDeltaY, text_, 2 /* borderWidth */,
                   backgroundColor_, fontColor_, &painter);
}

}  // namespace crystaldock
