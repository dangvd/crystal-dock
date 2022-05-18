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

#include "program.h"
#include <utils/command_utils.h>

namespace crystaldock {

QDataStream &operator<<(QDataStream &out, const LauncherInfo& launcher) {
  out << launcher.iconName << launcher.command;
  return out;
}

QDataStream &operator>>(QDataStream &in, LauncherInfo& launcher) {
  in >> launcher.iconName >> launcher.command;
  return in;
}

EditLaunchersDialog::EditLaunchersDialog(QWidget* parent, MultiDockModel* model,
                                         int dockId)
    : QDialog(parent),
      ui(new Ui::EditLaunchersDialog),
      model_(model),
      dockId_(dockId) {
  ui->setupUi(this);
  setWindowFlag(Qt::Tool);

  qRegisterMetaType<LauncherInfo>();
  qRegisterMetaTypeStreamOperators<LauncherInfo>("LauncherInfo");

  connect(ui->addSystemCommand, SIGNAL(currentIndexChanged(int)), this, SLOT(addSystemCommand(int)));
  connect(ui->addSeparator, SIGNAL(clicked()), this, SLOT(addSeparator()));
  connect(ui->remove, SIGNAL(clicked()), this, SLOT(removeSelectedLauncher()));
  connect(ui->removeAll, SIGNAL(clicked()), this, SLOT(removeAllLaunchers()));

  connect(ui->buttonBox, SIGNAL(clicked(QAbstractButton*)), this,
      SLOT(buttonClicked(QAbstractButton*)));

  loadData();
}

void EditLaunchersDialog::addLauncher(const QString& name,
    const QString& command, const QString& iconName) {
  QListWidgetItem* item = new QListWidgetItem(getListItemIcon(iconName), name);
  item->setData(Qt::UserRole, QVariant::fromValue(
      LauncherInfo(iconName, command)));
  ui->launchers->addItem(item);
  ui->launchers->setCurrentItem(item);
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

  LauncherInfo info = ui->addSystemCommand->currentData().value<LauncherInfo>();
  addLauncher(ui->addSystemCommand->currentText(), info.command, info.iconName);
}

void EditLaunchersDialog::addSeparator() {
  addLauncher("Separator", "SEPARATOR", /*iconName=*/"");
}

void EditLaunchersDialog::removeSelectedLauncher() {
  QListWidgetItem* item = ui->launchers->takeItem(ui->launchers->currentRow());
  if (item != nullptr) {
    delete item;
  }
}

void EditLaunchersDialog::removeAllLaunchers() {
  ui->launchers->clear();
}

void EditLaunchersDialog::loadData() {
  ui->launchers->clear();
  for (const auto& item : model_->dockLauncherConfigs(dockId_)) {
    QPixmap icon = QIcon::fromTheme(item.icon).pixmap(kListIconSize);
    QListWidgetItem* listItem = new QListWidgetItem(
          icon, item.name);
    listItem->setData(Qt::UserRole, QVariant::fromValue(
                        LauncherInfo(item.icon, item.command)));
    ui->launchers->addItem(listItem);
  }
  ui->launchers->setCurrentRow(0);
}

void EditLaunchersDialog::saveData() {
  const int launcherCount = ui->launchers->count();
  std::vector<LauncherConfig> launcherConfigs;
  launcherConfigs.reserve(launcherCount);
  for (int i = 0; i < launcherCount; ++i) {
    auto* listItem = ui->launchers->item(i);
    auto info = listItem->data(Qt::UserRole).value<LauncherInfo>();
    launcherConfigs.push_back(LauncherConfig(
                                listItem->text(), info.iconName, info.command));
  }
  model_->setDockLauncherConfigs(dockId_, launcherConfigs);
  model_->saveDockLauncherConfigs(dockId_);
}

}  // namespace crystaldock
