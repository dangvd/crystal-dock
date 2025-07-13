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

#include <string>

#include <QBrush>
#include <QColor>
#include <QFile>
#include <QIcon>
#include <QPainter>
#include <QPixmap>
#include <QTimer>

#include "display/window_system.h"

#include "dock_panel.h"
#include <utils/draw_utils.h>
#include <utils/font_utils.h>

namespace crystaldock {

DesktopSelector::DesktopSelector(DockPanel* parent, MultiDockModel* model,
                                 Qt::Orientation orientation, int minSize,
                                 int maxSize, const VirtualDesktopInfo& desktop, int screen)
    : IconBasedDockItem(parent, model, QString::fromStdString(desktop.name),
          orientation, "" /* no icon yet */, minSize, maxSize),
      desktopEnv_(DesktopEnv::getDesktopEnv()),
      desktop_(desktop),
      screen_(screen),
      desktopWidth_(parent->screenGeometry().width()),
      desktopHeight_(parent->screenGeometry().height()),
      hasCustomWallpaper_(false) {
  createMenu();
  loadConfig();
  connect(WindowSystem::self(), SIGNAL(desktopNameChanged(std::string_view, std::string_view)),
          this, SLOT(onDesktopNameChanged(std::string_view, std::string_view)));
  connect(&menu_, &QMenu::aboutToHide, this,
          [this]() {
            parent_->setShowingPopup(false);
          });
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
                     QString::number(desktop_.number), 1 /* borderWidth */, Qt::black,
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
      WindowSystem::setShowingDesktop(!WindowSystem::showingDesktop());
    } else {
      WindowSystem::setCurrentDesktop(desktop_.id);
    }
  } else if (e->button() == Qt::RightButton) {
    // In case other DesktopSelectors have changed the config.
    showDesktopNumberAction_->setChecked(model_->showDesktopNumber());
    showPopupMenu(&menu_);
  }
}

void DesktopSelector::loadConfig() {
  const auto& wallpaper = model_->wallpaper(desktop_.id, screen_);
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

void DesktopSelector::onDesktopNameChanged(
    std::string_view desktopId, std::string_view desktopName) {
  if (desktop_.id == desktopId) {
    setLabel(QString::fromStdString(std::string{desktopName}));
    titleAction_->setText(label_);
  }
}

void DesktopSelector::createMenu() {
  titleAction_ = menu_.addSection(label_);
  if (desktopEnv_->canSetWallpaper()) {
    menu_.addAction(
        QIcon::fromTheme("preferences-desktop-wallpaper"),
        QString("Set Wallpaper for Desktop ") + QString::number(desktop_.number),
        parent_,
        [this] {
          parent_->showWallpaperSettingsDialog(desktop_.number);
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
