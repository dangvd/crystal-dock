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
#include <QTimer>
#include <QUrl>

#include "display/window_system.h"

#include "dock_panel.h"
#include "program.h"
#include <utils/draw_utils.h>
#include <utils/menu_utils.h>

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
    : IconBasedDockItem(parent, model, "" /* label */, orientation, model->applicationMenuIcon(),
                        minSize, maxSize),
      showingMenu_(false),
      style_(model) {
  menu_.setAttribute(Qt::WA_TranslucentBackground);
  menu_.setStyle(&style_);
  menu_.setStyleSheet(getStyleSheet());

  loadConfig();
  buildMenu();

  createContextMenu();

  connect(&menu_, &QMenu::aboutToShow, this,
          [this]() {
            resetSearchMenu();
            showingMenu_ = true;
            parent_->update();
          });
  connect(&menu_, &QMenu::aboutToHide, this,
          [this]() {
            showingMenu_ = false;
            parent_->update();
          });
  connect(model_, SIGNAL(applicationMenuConfigChanged()),
          this, SLOT(reloadMenu()));
}

void ApplicationMenu::draw(QPainter* painter) const {
  if (showingMenu_) {
    const QColor baseColor = model_->activeIndicatorColor();
    // Size (width if horizontal, or height if vertical) of the indicator.
    const int size = DockPanel::kActiveIndicatorSize;
    drawIndicator(orientation_, left_ + getWidth() / 2, parent_->taskIndicatorPos(),
                  parent_->taskIndicatorPos(), top_ + getHeight() / 2,
                  size, DockPanel::k3DPanelThickness, baseColor, painter);
  }
  IconBasedDockItem::draw(painter);
}

void ApplicationMenu::mousePressEvent(QMouseEvent *e) {
  if (e->button() == Qt::LeftButton) {
    parent_->minimize();
    QTimer::singleShot(500, [this]{
      const int x = parent_->isBottom() && parent_->is3D()
          ? left_ : left_ - parent_->itemSpacing();
      menu_.exec(parent_->mapToGlobal(QPoint(x, top_)));
    });
  } else if (e->button() == Qt::RightButton) {
    parent_->minimize();
    QTimer::singleShot(500, [this]{
      contextMenu_.exec(parent_->mapToGlobal(QPoint(left_, top_)));
    });
  }
}

void ApplicationMenu::reloadMenu() {
  menu_.clear();
  searchMenu_ = nullptr;
  buildMenu();
}

void ApplicationMenu::searchApps(const QString& searchText_) {
  if (searchMenu_ == nullptr) {
    return;
  }

  QString text = searchText_.trimmed();
  if (text.isEmpty()) {
    resetSearchMenu();
    return;
  }

  const auto actions = searchMenu_->actions();
  for (int i = 1; i < actions.size(); ++i) {
    searchMenu_->removeAction(actions[i]);
  }

  for (const auto& entry : model_->searchApplications(text)) {
    addEntry(entry, searchMenu_);
  }
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
  addSearchMenu();
  menu_.addSeparator();
  addToMenu(model_->applicationMenuCategories());
  menu_.addSeparator();
  addToMenu(model_->applicationMenuSystemCategories());
  const auto numSubMenus = menu_.actions().size();
  for (int i = 0; i < numSubMenus; ++i) {
    QMenu* menu = menu_.actions()[i]->menu();
    if (menu != nullptr) {
      patchMenu(static_cast<int>((numSubMenus - i) * 1.25), menu);
    }
  }
}

void ApplicationMenu::addSearchMenu() {
  searchMenu_ = menu_.addMenu(loadIcon("edit-find"), "Search");
  searchMenu_->setAttribute(Qt::WA_TranslucentBackground);
  searchMenu_->setStyle(&style_);
  searchMenu_->setFont(font_);

  searchText_ = new QLineEdit(searchMenu_);
  searchText_->setMinimumWidth(250);
  searchText_->setPlaceholderText("Type here to search");
  // A work-around as using QWidgetAction somehow causes a memory issue
  // when quitting the dock.
  searchMenu_->addAction(loadIcon("edit-find"), "                 ");
  connect(searchText_, SIGNAL(textEdited(const QString&)),
          this, SLOT(searchApps(const QString&)));
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
  menu->addAction(loadIcon(entry.icon), entry.name, this,
                  [entry]() {
                    Program::launch(entry.command);
                  });
}

void ApplicationMenu::resetSearchMenu() {
  searchText_->clear();
  searchText_->setFocus();
  const auto actions = searchMenu_->actions();
  for (int i = 1; i < actions.size(); ++i) {
    searchMenu_->removeAction(actions[i]);
  }
  patchMenu(static_cast<int>(menu_.actions().size() * 1.25), searchMenu_);
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
