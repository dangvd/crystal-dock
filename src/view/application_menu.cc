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

#include "application_menu.h"

#include <algorithm>

#include <QApplication>
#include <QDrag>
#include <QFont>
#include <QMimeData>
#include <QSettings>
#include <QStringBuilder>
#include <QUrl>

#include <KWindowSystem>

#include "dock_panel.h"
#include "program.h"
#include <utils/draw_utils.h>

namespace crystaldock {

int ApplicationMenuStyle::pixelMetric(
    PixelMetric metric, const QStyleOption *option, const QWidget *widget)
    const {
  if (metric == QStyle::PM_SmallIconSize) {
    return model_->applicationMenuIconSize();
  }
  return QProxyStyle::pixelMetric(metric, option, widget);
}

ApplicationMenu::ApplicationMenu(
    DockPanel *parent, MultiDockModel* model, Qt::Orientation orientation,
    int minSize, int maxSize)
    : IconBasedDockItem(parent, "" /* label */, orientation, model->applicationMenuIcon(),
                        minSize, maxSize),
      model_(model),
      showingMenu_(false),
      style_(model) {
  menu_.setAttribute(Qt::WA_TranslucentBackground);
  menu_.setStyle(&style_);
  menu_.setStyleSheet(getStyleSheet());

  loadConfig();
  buildMenu();

  createContextMenu();

  connect(&menu_, &QMenu::aboutToShow, this,
          [this]() { showingMenu_ = true; } );
  connect(&menu_, &QMenu::aboutToHide, this,
          [this]() { showingMenu_ = false; } );
  connect(model_, SIGNAL(applicationMenuConfigChanged()),
          this, SLOT(reloadMenu()));
}

void ApplicationMenu::draw(QPainter* painter) const {
  if (showingMenu_) {
    drawHighlightedIcon(model_->backgroundColor(), left_, top_, getWidth(), getHeight(),
                        minSize_ / 4 - 4, size_ / 8, painter);
  }
  IconBasedDockItem::draw(painter);
}

void ApplicationMenu::mousePressEvent(QMouseEvent *e) {
  if (e->button() == Qt::LeftButton) {
    menu_.popup(parent_->applicationMenuPosition(getMenuSize()));
  } else if (e->button() == Qt::RightButton) {
    contextMenu_.popup(e->globalPos());
  }
}

void ApplicationMenu::reloadMenu() {
  menu_.clear();
  buildMenu();
}

QString ApplicationMenu::getStyleSheet() {
  QColor bgColor = model_->backgroundColor();
  bgColor.setAlphaF(model_->applicationMenuBackgroundAlpha());
  bgColor = bgColor.darker();
  QColor borderColor = model_->borderColor();
  return " \
QMenu { \
  background-color: " % bgColor.name(QColor::HexArgb) % ";"
" margin: 1px; \
  padding: 2px; \
  border: 1px transparent; \
  border-radius: 3px; \
} \
\
QMenu::item { \
  font: bold; \
  color: white; \
  background-color: transparent; \
  padding: 4px 45px 4px 45px; \
} \
\
QMenu::item:selected { \
  background-color: " % bgColor.name(QColor::HexArgb) % ";"
" border: 1px solid " % borderColor.name() % ";"
" border-radius: 3px; \
} \
\
QMenu::separator { \
  margin: 5px; \
  height: 1px; \
  background: " % borderColor.name() % ";"
"}";
}

void ApplicationMenu::loadConfig() {
  setLabel(model_->applicationMenuName());
  font_ = menu_.font();
  font_.setPointSize(model_->applicationMenuFontSize());
  font_.setBold(true);
  menu_.setFont(font_);
}

void ApplicationMenu::buildMenu() {
  addToMenu(model_->applicationMenuCategories());
  menu_.addSeparator();
  addToMenu(model_->applicationMenuSystemCategories());
}

void ApplicationMenu::addToMenu(const std::vector<Category>& categories) {
  for (const auto& category : categories) {
    if (category.entries.empty()) {
      continue;
    }

    QMenu* menu = menu_.addMenu(loadIcon(category.icon), category.displayName);
    menu->setAttribute(Qt::WA_TranslucentBackground);
    menu->setStyle(&style_);
    menu->setFont(font_);
    for (const auto& entry : category.entries) {
      addEntry(entry, menu);
    }
  }
}

void ApplicationMenu::addEntry(const ApplicationEntry &entry, QMenu *menu) {
  QAction* action = menu->addAction(loadIcon(entry.icon), entry.name, this,
                  [&entry]() {
                    Program::launch(entry.command);
                  });
  action->setData(entry.desktopFile);
}

QIcon ApplicationMenu::loadIcon(const QString &icon) {
  return QIcon::fromTheme(icon).pixmap(model_->applicationMenuIconSize());
}

void ApplicationMenu::createContextMenu() {
  contextMenu_.addAction(QIcon::fromTheme("configure"),
                         QString("Application Menu &Settings"),
                         parent_,
                         [this] { parent_->showApplicationMenuSettingsDialog(); });
  contextMenu_.addSeparator();
  parent_->addPanelSettings(&contextMenu_);
}

}  // namespace crystaldock
