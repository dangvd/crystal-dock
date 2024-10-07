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

#ifndef CRYSTALDOCK_EDIT_LAUNCHERS_DIALOG_H_
#define CRYSTALDOCK_EDIT_LAUNCHERS_DIALOG_H_

#include <QAbstractButton>
#include <QDataStream>
#include <QDialog>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QListWidget>
#include <QListWidgetItem>

#include <model/multi_dock_model.h>

namespace Ui {
  class EditLaunchersDialog;
}

namespace crystaldock {

// User data for the items in QListWidget/QComboBox.
struct LauncherInfo {
  // The name(label) is already stored as item text in QListWidget/QComboBox.
  QString iconName;
  QString appId;

  LauncherInfo() {}
  LauncherInfo(QString iconName2, QString appId2)
      : iconName(iconName2), appId(appId2) {}
};

QDataStream &operator<<(QDataStream &out, const LauncherInfo& launcher);
QDataStream &operator>>(QDataStream &in, LauncherInfo& launcher);

class EditLaunchersDialog;

class LauncherList : public QListWidget {
 public:
  explicit LauncherList(EditLaunchersDialog* parent);

 protected:
  void dragEnterEvent(QDragEnterEvent *event) override;
  void dragMoveEvent(QDragMoveEvent* event) override;
  void dropEvent(QDropEvent *event) override;

 private:
  EditLaunchersDialog* parent_;
};

class EditLaunchersDialog : public QDialog {
  Q_OBJECT

 public:
  EditLaunchersDialog(QWidget* parent, MultiDockModel* model, int dockId);
  ~EditLaunchersDialog() = default;

  void reload() { loadData(); }

  void addLauncher(const QString& name, const QString& appId, const QString& iconName);

 public slots:
  void accept() override;
  void buttonClicked(QAbstractButton* button);

  void addSystemCommand(int index);
  void addSeparator();
  void removeSelectedLauncher();
  void removeAllLaunchers();

 private:
  static constexpr int kListIconSize = 48;

  void initSystemCommands();

  void loadData();
  void saveData();

  QIcon getListItemIcon(const QString& iconName) {
    return QIcon::fromTheme(iconName).pixmap(kListIconSize);
  }

  void populateSystemCommands();

  Ui::EditLaunchersDialog* ui;
  LauncherList* launchers_;

  MultiDockModel* model_;
  int dockId_;

  friend class EditLaunchersDialogTest;
};

}  // namespace crystaldock

Q_DECLARE_METATYPE(crystaldock::LauncherInfo);

#endif  // CRYSTALDOCK_EDIT_LAUNCHERS_DIALOG_H_
