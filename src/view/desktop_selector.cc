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

#include "desktop_selector.h"

#include <QBrush>
#include <QColor>
#include <QFile>
#include <QIcon>
#include <QPainter>
#include <QPixmap>

#include <KWindowSystem>

#include "dock_panel.h"
#include <utils/draw_utils.h>
#include <utils/font_utils.h>

namespace crystaldock {

DesktopSelector::DesktopSelector(DockPanel* parent, MultiDockModel* model,
                                 Qt::Orientation orientation, int minSize,
                                 int maxSize, int desktop, int screen)
    : IconBasedDockItem(parent, model,
          QString("Desktop ") + QString::number(desktop),
          orientation, "" /* no icon yet */, minSize, maxSize),
      desktopEnv_(DesktopEnv::getDesktopEnv()),
      desktop_(desktop),
      screen_(screen),
      desktopWidth_(parent->screenGeometry().width()),
      desktopHeight_(parent->screenGeometry().height()),
      hasCustomWallpaper_(false) {
  createMenu();
  loadConfig();
}

void DesktopSelector::draw(QPainter* painter) const {
  if (hasCustomWallpaper_) {
    IconBasedDockItem::draw(painter);
  } else {
    // Draw rectangles with desktop numbers if no custom wallpapers set.
    QColor fillColor = model_->backgroundColor().lighter();
    fillColor.setAlphaF(0.42);
    painter->fillRect(left_, top_, getWidth(), getHeight(), QBrush(fillColor));
  }

  if (model_->showDesktopNumber()) {
    painter->setFont(adjustFontSize(getWidth(), getHeight(),
                                    "0" /* reference string */,
                                    0.5 /* scale factor */));
    painter->setRenderHint(QPainter::TextAntialiasing);
    drawBorderedText(left_, top_, getWidth(), getHeight(), Qt::AlignCenter,
                     QString::number(desktop_), 1 /* borderWidth */, Qt::black,
                     Qt::white, painter);
  }

  // Draw the border for the current desktop.
  if (isCurrentDesktop()) {
    painter->setPen(model_->borderColor());
    painter->drawRect(left_ - 1, top_ - 1, getWidth() + 1, getHeight() + 1);
  }
}

void DesktopSelector::mousePressEvent(QMouseEvent* e) {
  if (e->button() == Qt::LeftButton) {
    if (isCurrentDesktop()) {
      KWindowSystem::setShowingDesktop(!KWindowSystem::showingDesktop());
    } else {
      KWindowSystem::setCurrentDesktop(desktop_);
    }
  } else if (e->button() == Qt::RightButton) {
    // In case other DesktopSelectors have changed the config.
    showDesktopNumberAction_->setChecked(model_->showDesktopNumber());
    menu_.popup(e->globalPos());
  }
}

void DesktopSelector::loadConfig() {
  const auto& wallpaper = model_->wallpaper(desktop_, screen_);
  if (!wallpaper.isEmpty() && QFile::exists(wallpaper)) {
    setIconScaled(QPixmap(wallpaper));
    hasCustomWallpaper_ = true;
  }

  showDesktopNumberAction_->setChecked(model_->showDesktopNumber());
}

void DesktopSelector::saveConfig() {
  model_->setShowDesktopNumber(showDesktopNumberAction_->isChecked());
  model_->saveAppearanceConfig(true /* repaintOnly */);
}

void DesktopSelector::setIconScaled(const QPixmap& icon) {
  if (icon.width() * desktopHeight_ != icon.height() * desktopWidth_) {
    QPixmap scaledIcon = icon.scaled(desktopWidth_, desktopHeight_);
    setIcon(scaledIcon);
  } else {
    setIcon(icon);
  }
}

void DesktopSelector::createMenu() {
  if (desktopEnv_->canSetWallpaper()) {
    menu_.addAction(
        QIcon::fromTheme("preferences-desktop-wallpaper"),
        QString("Set Wallpaper for Desktop ") + QString::number(desktop_),
        parent_,
        [this] {
          parent_->showWallpaperSettingsDialog(desktop_);
        });
  }
  showDesktopNumberAction_ = menu_.addAction(
      QString("Show Desktop Number"), this,
      [this] {
        saveConfig();
      });
  showDesktopNumberAction_->setCheckable(true);

  menu_.addSeparator();
  parent_->addPanelSettings(&menu_);
}

}  // namespace crystaldock
