/*
 * This file is part of Crystal Dock.
 * Copyright (C) 2025 Viet Dang (dangvd@gmail.com)
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

#ifndef CRYSTALDOCK_TRASH_H_
#define CRYSTALDOCK_TRASH_H_

#include <QAction>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileSystemWatcher>
#include <QMenu>
#include <QMimeData>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QUrl>

#include "icon_based_dock_item.h"

namespace crystaldock {

class Trash : public QObject, public IconBasedDockItem {
  Q_OBJECT

 public:
  Trash(DockPanel* parent, MultiDockModel* model, Qt::Orientation orientation,
        int minSize, int maxSize);
  virtual ~Trash() = default;

  void draw(QPainter* painter) const override;
  void mousePressEvent(QMouseEvent* e) override;
  QString getLabel() const override;
  bool beforeTask(const QString& program) override { return false; }

  QString getAppId() const override { return "trash"; }

  void setAcceptDrops(bool accept);
  bool canAcceptDrop(const QMimeData* mimeData) const;

 public slots:
  void updateTrashState();
  void emptyTrash();
  void openTrash();

  void dragEnterEvent(QDragEnterEvent* event);
  void dropEvent(QDropEvent* event);

 private:
  void createMenu();
  void moveToTrash(const QStringList& filePaths);
  bool isTrashEmpty() const;
  QString getTrashPath() const;
  QString getTrashInfoPath() const;
  QString getTrashFilesPath() const;
  void updateIcon();
  void setupTrashWatcher();

  QString trashPath_;
  QString trashInfoPath_;
  QString trashFilesPath_;
  
  QFileSystemWatcher* trashWatcher_;
  
  QMenu menu_;
  QAction* emptyTrashAction_;
  QAction* openTrashAction_;
  QAction* restoreAction_;

  bool isEmpty_;
  bool acceptingDrop_;

  static constexpr const char* kEmptyTrashIconName = "user-trash";
  static constexpr const char* kFullTrashIconName = "user-trash-full";
};

}  // namespace crystaldock

#endif  // CRYSTALDOCK_TRASH_H_
