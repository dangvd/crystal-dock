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

#include "edit_launchers_dialog.h"
#include "ui_edit_launchers_dialog.h"

#include <QDir>
#include <QFileDialog>
#include <QMimeData>
#include <QUrl>
#include <QVariant>
#include <Qt>
#include <QSettings>

#include <utils/desktop_file.h>

namespace crystaldock {

QDataStream &operator<<(QDataStream &out, const LauncherInfo& launcher) {
  out << launcher.iconName << launcher.appId;
  return out;
}

QDataStream &operator>>(QDataStream &in, LauncherInfo& launcher) {
  in >> launcher.iconName >> launcher.appId;
  return in;
}

LauncherList::LauncherList(EditLaunchersDialog* parent)
    : QListWidget(parent), parent_(parent) {}

void LauncherList::dragEnterEvent(QDragEnterEvent *event) {
  // Internal drag-and-drop.
  LauncherList* source = dynamic_cast<LauncherList*>(event->source());
  if (source != nullptr && source == this) {
    event->acceptProposedAction();
    setDragDropMode(QAbstractItemView::InternalMove);
    return;
  }

  // External drag-and-drop.
  if (event->mimeData()->hasFormat("text/uri-list")) {
    QString fileUrl =
        QString(event->mimeData()->data("text/uri-list")).trimmed();
    if (fileUrl.endsWith(".desktop")) {
      event->acceptProposedAction();
      setDragDropMode(QAbstractItemView::DragDrop);
    }
  }
}

void LauncherList::dragMoveEvent(QDragMoveEvent* event) {
  event->acceptProposedAction();
}

void LauncherList::dropEvent(QDropEvent* event) {
  // External drag-and-drop.
  if (event->mimeData()->hasFormat("text/uri-list")) {
    QString fileUrl =
        QString(event->mimeData()->data("text/uri-list")).trimmed();
    DesktopFile desktopFile(QUrl(fileUrl).toLocalFile());
    parent_->addLauncher(desktopFile.name(), desktopFile.appId(), desktopFile.icon());
  } else {  // Internal drag-and-drop.
    QListWidget::dropEvent(event);
  }
}

EditLaunchersDialog::EditLaunchersDialog(QWidget* parent, MultiDockModel* model,
                                         int dockId)
    : QDialog(parent),
      ui(new Ui::EditLaunchersDialog),
      model_(model),
      dockId_(dockId) {
  ui->setupUi(this);
  launchers_ = new LauncherList(this);
  launchers_->setGeometry(QRect(20, 20, 350, 490));
  launchers_->setSelectionMode(QAbstractItemView::SingleSelection);
  launchers_->setDragEnabled(true);
  launchers_->setAcceptDrops(true);
  launchers_->setDropIndicatorShown(true);
  launchers_->setDragDropMode(QAbstractItemView::DragDrop);
  setWindowFlag(Qt::Tool);

  qRegisterMetaType<LauncherInfo>();

  connect(ui->systemCommands, SIGNAL(currentIndexChanged(int)), this, SLOT(addSystemCommand(int)));
  connect(ui->addSeparator, SIGNAL(clicked()), this, SLOT(addSeparator()));
  connect(ui->addLauncherSeparator, SIGNAL(clicked()), this, SLOT(addLauncherSeparator()));
  connect(ui->remove, SIGNAL(clicked()), this, SLOT(removeSelectedLauncher()));
  connect(ui->removeAll, SIGNAL(clicked()), this, SLOT(removeAllLaunchers()));

  connect(ui->buttonBox, SIGNAL(clicked(QAbstractButton*)), this,
      SLOT(buttonClicked(QAbstractButton*)));

  initSystemCommands();
  loadData();
}

void EditLaunchersDialog::addLauncher(const QString& name,
    const QString& appId, const QString& iconName) {
  QListWidgetItem* listItem;
  if (appId == kSeparatorId) {
    listItem = new QListWidgetItem("--- Separator ---");
  } else if (appId == kLauncherSeparatorId) {
    listItem = new QListWidgetItem("--- Launcher Separator ---");
  } else if (appId == kShowDesktopId) {
    listItem = new QListWidgetItem(
        QIcon::fromTheme(kShowDesktopIcon).pixmap(kListIconSize), kShowDesktopName);
  } else {
    listItem = new QListWidgetItem(
        QIcon::fromTheme(iconName).pixmap(kListIconSize), name);
  }
  listItem->setData(Qt::UserRole,
                    QVariant::fromValue(LauncherInfo(iconName, appId)));
  launchers_->addItem(listItem);
  launchers_->setCurrentItem(listItem);
}

void EditLaunchersDialog::accept() {
  QDialog::accept();
  saveData();
}

void EditLaunchersDialog::buttonClicked(QAbstractButton* button) {
  auto role = ui->buttonBox->buttonRole(button);
  if (role == QDialogButtonBox::ApplyRole) {
    saveData();
  }
}

void EditLaunchersDialog::addSystemCommand(int index) {
  if (index <= 0) {  // Excludes header item.
    return;
  }

  LauncherInfo info = ui->systemCommands->currentData().value<LauncherInfo>();
  addLauncher(ui->systemCommands->currentText(), info.appId, info.iconName);
}

void EditLaunchersDialog::addSeparator() {
  addLauncher("Separator", kSeparatorId, /*iconName=*/"");
}

void EditLaunchersDialog::addLauncherSeparator() {
    addLauncher("Launcher Separator", kLauncherSeparatorId, /*iconName=*/"");
}

void EditLaunchersDialog::removeSelectedLauncher() {
  QListWidgetItem* item = launchers_->takeItem(launchers_->currentRow());
  if (item != nullptr) {
    delete item;
  }
}

void EditLaunchersDialog::removeAllLaunchers() {
  launchers_->clear();
}

void EditLaunchersDialog::initSystemCommands() {
  ui->systemCommands->addItem(getListItemIcon(kShowDesktopIcon), kShowDesktopName,
      QVariant::fromValue(LauncherInfo(kShowDesktopIcon, kShowDesktopId)));
  for (const auto& category : model_->applicationMenuSystemCategories()) {
    for (const auto& entry : category.entries) {
      ui->systemCommands->addItem(
          getListItemIcon(entry.icon), entry.name,
          QVariant::fromValue(LauncherInfo(entry.icon, entry.appId)));
    }
  }
}

void EditLaunchersDialog::loadData() {
  launchers_->clear();
  for (const auto& item : model_->launcherConfigs(dockId_)) {
    addLauncher(item.name, item.appId, item.icon);
  }
  launchers_->setCurrentRow(0);

  ui->systemCommands->setCurrentIndex(0);
}

void EditLaunchersDialog::saveData() {
  const int launcherCount = launchers_->count();
  QStringList launchers;
  for (int i = 0; i < launcherCount; ++i) {
    auto* listItem = launchers_->item(i);
    auto info = listItem->data(Qt::UserRole).value<LauncherInfo>();
    launchers.append(info.appId);
  }
  model_->setLaunchers(dockId_, launchers);
  model_->saveDockConfig(dockId_);
}

}  // namespace crystaldock
