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

#ifndef CRYSTALDOCK_APPLICATION_MENU_H_
#define CRYSTALDOCK_APPLICATION_MENU_H_

#include "icon_based_dock_item.h"

#include <QEvent>
#include <QFont>
#include <QLineEdit>
#include <QMenu>
#include <QMouseEvent>
#include <QPoint>
#include <QProxyStyle>
#include <QSize>
#include <QString>

#include <model/application_menu_config.h>
#include <model/multi_dock_model.h>

namespace crystaldock {

class ApplicationMenuStyle : public QProxyStyle {
 public:
  ApplicationMenuStyle(const MultiDockModel* model) : model_(model) {}

  int pixelMetric(PixelMetric metric, const QStyleOption *option = Q_NULLPTR,
                  const QWidget *widget = Q_NULLPTR) const override;

 private:
  const MultiDockModel* model_;
};


// The application menu item on the dock.
//
// Left-clicking the item shows a cascading popup menu that contains entries
// for all applications organized by categories. The menu uses a custom style
// e.g. bigger icon size and the same translucent effect as the dock's.
//
// Supports drag-and-drop as a drag source.
// What it means is that you can drag an application entry from the menu
// to other widgets/applications. It doesn't support drag-and-drop within the
// menu itself.
class ApplicationMenu : public QObject, public IconBasedDockItem {
  Q_OBJECT

 public:
  ApplicationMenu(
      DockPanel* parent,
      MultiDockModel* model,
      Qt::Orientation orientation,
      int minSize,
      int maxSize);
  virtual ~ApplicationMenu() = default;

  void draw(QPainter* painter) const override;
  void mousePressEvent(QMouseEvent* e) override;
  void loadConfig() override;

  QSize getMenuSize() { return menu_.sizeHint(); }

 public slots:
  void reloadMenu();

  void searchApps(const QString& searchText);

 private:
  QString getStyleSheet();

  QIcon loadIcon(const QString& icon);

  // Builds the menu from the application entries;
  void buildMenu();
  void addSearchMenu();
  void addToMenu(const std::vector<Category>& categories);
  void addEntry(const ApplicationEntry& entry, QMenu* menu);

  void resetSearchMenu();

  void createContextMenu();

  MultiDockModel* model_;

  // The cascading popup menu that contains all application entries.
  QMenu menu_;
  bool showingMenu_;

  ApplicationMenuStyle style_;
  QFont font_;

  QMenu* searchMenu_;
  QLineEdit* searchText_;

  // Context (right-click) menu.
  QMenu contextMenu_;
};

}  // namespace crystaldock

#endif // CRYSTALDOCK_APPLICATION_MENU_H_
