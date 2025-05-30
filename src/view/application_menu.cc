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
    const auto x = left_ + getWidth() / 2;
    const auto y = top_ + getHeight() / 2;
    if (parent_->isGlass()) {
      const QColor baseColor = model_->activeIndicatorColor();
      // Size (width if horizontal, or height if vertical) of the indicator.
      const auto size = DockPanel::kIndicatorSizeGlass;
      drawIndicator(orientation_, x, parent_->taskIndicatorPos(),
                    parent_->taskIndicatorPos(), y,
                    size, DockPanel::k3DPanelThickness, baseColor, painter);
    } else if (parent_->isFlat2D()) {
      const auto baseColor = model_->activeIndicatorColor2D();
      const auto size = DockPanel::kIndicatorSizeFlat2D;
      drawIndicatorFlat2D(orientation_, x, parent_->taskIndicatorPos(),
                          parent_->taskIndicatorPos(), y,
                          size, baseColor, painter);
    } else {  // Metal 2D.
      const auto baseColor = model_->activeIndicatorColorMetal2D();
      const auto size = DockPanel::kIndicatorSizeMetal2D;
      drawIndicatorMetal2D(parent_->position(), x, parent_->taskIndicatorPos(),
                           parent_->taskIndicatorPos(), y,
                           size, baseColor, painter);
    }
  }
  IconBasedDockItem::draw(painter);
}

void ApplicationMenu::mousePressEvent(QMouseEvent *e) {
  if (e->button() == Qt::LeftButton) {
    // Acknowledge.
    showingMenu_ = true;
    parent_->update();

    resetSearchMenu();
    const int x = parent_->isBottom() && parent_->is3D()
        ? left_ : left_ - parent_->itemSpacing();
    menu_.exec(parent_->mapToGlobal(QPoint(x, top_)));
  } else if (e->button() == Qt::RightButton) {
    contextMenu_.exec(parent_->mapToGlobal(QPoint(left_, top_)));
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

  // We need to limit max number of results to avoid the sub-menu being pushed up too much.
  for (const auto& entry : model_->searchApplications(text, maxNumResults_)) {
    addEntry(entry, searchMenu_);
  }
  if (parent_->isBottom()) {
    // Work-around for sub-menu alignment issue on Wayland.
    patchMenu(maxNumResults_ + 1, model_->applicationMenuIconSize(), searchMenu_);
  }
}

bool ApplicationMenu::eventFilter(QObject* object, QEvent* event) {
  if (event->type() == QEvent::MouseButtonPress) {
    auto* activeItem = (dynamic_cast<QMenu*>(object))->activeAction();
    QMouseEvent* mouseEvent = dynamic_cast<QMouseEvent*>(event);
    if (mouseEvent && mouseEvent->button() == Qt::LeftButton && activeItem) {
      startMousePos_ = mouseEvent->pos();
      draggedEntry_ = activeItem->data().toString();
    }
  } else if (event->type() == QEvent::MouseMove) {
    QMouseEvent* mouseEvent = dynamic_cast<QMouseEvent*>(event);
    if (mouseEvent && mouseEvent->buttons() & Qt::LeftButton) {
      const int distance
          = (mouseEvent->pos() - startMousePos_).manhattanLength();
      if (distance >= QApplication::startDragDistance()
          && !draggedEntry_.isEmpty()) {
        // Start drag.
        QMimeData* mimeData = new QMimeData;
        mimeData->setData("text/uri-list",
                          QUrl::fromLocalFile(draggedEntry_).toEncoded());

        QDrag* drag = new QDrag(this);
        drag->setMimeData(mimeData);
        drag->exec(Qt::CopyAction);
      }
    }
  }

  return QObject::eventFilter(object, event);
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
    if (menu != nullptr && parent_->isBottom()) {
      // Work-around for sub-menu alignment issue on Wayland.
      patchMenu(numSubMenus - i, model_->applicationMenuIconSize(), menu);
    }
  }
  maxNumResults_ = menu_.actions().size() - 2;
}

void ApplicationMenu::addSearchMenu() {
  searchMenu_ = menu_.addMenu(loadIcon("edit-find"), "Search");
  searchMenu_->setAttribute(Qt::WA_TranslucentBackground);
  searchMenu_->setStyle(&style_);
  searchMenu_->setFont(font_);
  searchMenu_->installEventFilter(this);

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
    if (category.name == ApplicationMenuConfig::kUncategorized || category.entries.empty()) {
      continue;
    }

    QMenu* menu = menu_.addMenu(loadIcon(category.icon), category.displayName);
    menu->setAttribute(Qt::WA_TranslucentBackground);
    menu->setStyle(&style_);
    menu->setFont(font_);
    menu->installEventFilter(this);
    for (const auto& entry : category.entries) {
      addEntry(entry, menu);
    }
  }
}

void ApplicationMenu::addEntry(const ApplicationEntry &entry, QMenu *menu) {
  if (entry.hidden) {
    return;
  }

  QAction* action = menu->addAction(loadIcon(entry.icon), entry.name, this,
                  [entry]() {
                    Program::launch(entry.command);
                  });
  action->setData(entry.desktopFile);
}

void ApplicationMenu::resetSearchMenu() {
  searchText_->clear();
  searchText_->setFocus();
  const auto actions = searchMenu_->actions();
  for (int i = 1; i < actions.size(); ++i) {
    searchMenu_->removeAction(actions[i]);
  }
  if (parent_->isBottom()) {
    // Work-around for sub-menu alignment issue on Wayland.
    patchMenu(maxNumResults_ + 1, model_->applicationMenuIconSize(), searchMenu_);
  }
}

QIcon ApplicationMenu::loadIcon(const QString &icon) {
  return QIcon::fromTheme(icon);
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
