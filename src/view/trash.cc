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

#include "trash.h"

#include <QApplication>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QMouseEvent>
#include <QPainter>
#include <QProcess>
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <QTextStream>

#include <utils/draw_utils.h>
#include <utils/font_utils.h>

#include "dock_panel.h"
#include "program.h"

namespace crystaldock {

Trash::Trash(DockPanel* parent, MultiDockModel* model, Qt::Orientation orientation,
             int minSize, int maxSize)
    : IconBasedDockItem(parent, model, "Trash", orientation, kEmptyTrashIconName, 
                        minSize, maxSize),
      trashWatcher_(nullptr),
      isEmpty_(true),
      acceptingDrop_(false) {
  
  trashPath_ = getTrashPath();
  trashInfoPath_ = getTrashInfoPath();
  trashFilesPath_ = getTrashFilesPath();
  
  createMenu();
  setupTrashWatcher();
  updateTrashState();
  connect(&menu_, &QMenu::aboutToHide, this,
          [this]() {
            parent_->setShowingPopup(false);
          });
}

void Trash::draw(QPainter* painter) const {
  IconBasedDockItem::draw(painter);
  
  if (acceptingDrop_) {
    painter->save();
    QPen pen(QColor(0, 150, 255, 200), 2);
    painter->setPen(pen);
    painter->setBrush(QBrush(QColor(0, 150, 255, 50)));
    painter->drawRoundedRect(left_, top_, getWidth(), getHeight(), 8, 8);
    painter->restore();
  }
}

void Trash::mousePressEvent(QMouseEvent* e) {
  if (e->button() == Qt::LeftButton) {
    openTrash();
  } else if (e->button() == Qt::RightButton) {
    showPopupMenu(&menu_);
  }
}

QString Trash::getLabel() const {
  return isEmpty_ ? "Trash (Empty)" : "Trash (Full)";
}

void Trash::updateTrashState() {
  bool wasEmpty = isEmpty_;
  isEmpty_ = isTrashEmpty();
  
  if (isEmpty_ != wasEmpty) {
    updateIcon();
    parent_->update();
  }
}

void Trash::emptyTrash() {
  if (isEmpty_) {
    return;
  }
  
  QDir trashFilesDir(trashFilesPath_);
  QDir trashInfoDir(trashInfoPath_);
  
  trashFilesDir.removeRecursively();
  trashInfoDir.removeRecursively();
  
  QDir().mkpath(trashFilesPath_);
  QDir().mkpath(trashInfoPath_);
  
  updateTrashState();
}

void Trash::openTrash() {
  Program::launch("xdg-open trash:/");
}

void Trash::setAcceptDrops(bool accept) {
  acceptingDrop_ = accept;
  parent_->update();
}

bool Trash::canAcceptDrop(const QMimeData* mimeData) const {
  return mimeData->hasUrls();
}

void Trash::dragEnterEvent(QDragEnterEvent* event) {
  if (canAcceptDrop(event->mimeData())) {
    event->acceptProposedAction();
    setAcceptDrops(true);
  }
}

void Trash::dropEvent(QDropEvent* event) {
  setAcceptDrops(false);
  
  if (!canAcceptDrop(event->mimeData())) {
    return;
  }
  
  QStringList filePaths;
  for (const QUrl& url : event->mimeData()->urls()) {
    if (url.isLocalFile()) {
      filePaths << url.toLocalFile();
    }
  }
  
  if (!filePaths.isEmpty()) {
    moveToTrash(filePaths);
    event->acceptProposedAction();
  }
}

void Trash::createMenu() {
  menu_.addSection(label_);
  emptyTrashAction_ = menu_.addAction(QIcon::fromTheme("trash-empty"), "Empty Trash");
  connect(emptyTrashAction_, &QAction::triggered, this, &Trash::emptyTrash);

  menu_.addSeparator();
  parent_->addPanelSettings(&menu_);
}

void Trash::moveToTrash(const QStringList& filePaths) {
  for (const QString& filePath : filePaths) {
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
      continue;
    }
    
    QString fileName = fileInfo.fileName();
    QString destPath = trashFilesPath_ + "/" + fileName;
    QString infoPath = trashInfoPath_ + "/" + fileName + ".trashinfo";
    
    int counter = 1;
    while (QFile::exists(destPath)) {
      QString baseName = fileInfo.baseName();
      QString suffix = fileInfo.completeSuffix();
      if (!suffix.isEmpty()) {
        fileName = QString("%1_%2.%3").arg(baseName).arg(counter).arg(suffix);
      } else {
        fileName = QString("%1_%2").arg(baseName).arg(counter);
      }
      destPath = trashFilesPath_ + "/" + fileName;
      infoPath = trashInfoPath_ + "/" + fileName + ".trashinfo";
      counter++;
    }
    
    if (QFile::rename(filePath, destPath)) {
      QFile infoFile(infoPath);
      if (infoFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&infoFile);
        stream << "[Trash Info]\n";
        stream << "Path=" << filePath << "\n";
        stream << "DeletionDate=" << QDateTime::currentDateTime().toString(Qt::ISODate) << "\n";
        infoFile.close();
      }
    }
  }
  
  updateTrashState();
}

bool Trash::isTrashEmpty() const {
  QDir trashDir(trashFilesPath_);
  return trashDir.isEmpty();
}

QString Trash::getTrashPath() const {
  QString dataHome = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
  return dataHome + "/Trash";
}

QString Trash::getTrashInfoPath() const {
  return trashPath_ + "/info";
}

QString Trash::getTrashFilesPath() const {
  return trashPath_ + "/files";
}

void Trash::updateIcon() {
  setIconName(isEmpty_ ? kEmptyTrashIconName : kFullTrashIconName);
  
  emptyTrashAction_->setEnabled(!isEmpty_);
}

void Trash::setupTrashWatcher() {
  trashWatcher_ = new QFileSystemWatcher(this);
  
  QDir().mkpath(trashFilesPath_);
  QDir().mkpath(trashInfoPath_);
  
  trashWatcher_->addPath(trashFilesPath_);
  trashWatcher_->addPath(trashInfoPath_);
  
  connect(trashWatcher_, &QFileSystemWatcher::directoryChanged,
          this, &Trash::updateTrashState);
}

}  // namespace crystaldock

