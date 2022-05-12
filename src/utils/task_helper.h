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

#ifndef CRYSTALDOCK_TASK_HELPER_H_
#define CRYSTALDOCK_TASK_HELPER_H_

#include <vector>

#include <QObject>
#include <QPixmap>
#include <QString>

namespace crystaldock {

struct TaskInfo {
  WId wId;
  QString program;  // e.g. Dolphin
  QString command;  // e.g. dolphin
  QString name;  // e.g. home -- Dolphin
  QPixmap icon;
  bool demandsAttention;

  TaskInfo(WId wId2, const QString& program2) : wId(wId2), program(program2) {}
  TaskInfo(WId wId2, const QString& program2, const QString&command2, const QString& name2,
           const QPixmap& icon2, bool demandsAttention2)
      : wId(wId2), program(program2), command(command2), name(name2), icon(icon2),
        demandsAttention(demandsAttention2) {}
  TaskInfo(const TaskInfo& taskInfo) = default;
  TaskInfo& operator=(const TaskInfo& taskInfo) = default;

  bool operator<(const TaskInfo& taskInfo) const;
};

class TaskHelper : public QObject {
  Q_OBJECT

 public:
  TaskHelper();

  // Loads running tasks.
  //
  // Args:
  //   screen: screen index to load, or -1 if loading for all screens.
  std::vector<TaskInfo> loadTasks(int screen, bool currentDesktopOnly);

  // Whether the task is valid for showing on the task manager.
  bool isValidTask(WId wId);

  // Whether the task is valid for showing on the task manager on specific screen.
  bool isValidTask(WId wId, int screen, bool currentDesktopOnly = true);

  static TaskInfo getBasicTaskInfo(WId wId);

  TaskInfo getTaskInfo(WId wId) const;

  // Gets the screen that a task is running on.
  int getScreen(WId wId);

 public slots:
  void onCurrentDesktopChanged(int desktop) {
    currentDesktop_ = desktop;
  }

 private:
  // KWindowSystem::currentDesktop() is buggy sometimes, for example,
  // on windowAdded() event, so we store it here ourselves.
  int currentDesktop_;
};

}  // namespace crystaldock

#endif  // CRYSTALDOCK_TASK_HELPER_H_
