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

#include "task_helper.h"

#include <algorithm>
#include <iostream>
#include <regex>
#include <utility>

#include <QDBusInterface>
#include <QDBusReply>
#include <QGuiApplication>
#include <QScreen>

#include <KWindowSystem>

namespace crystaldock {

namespace {

QString getProgram(const KWindowInfo& info) {
  return QString(info.windowClassName());
}

QString getCommand(const KWindowInfo& info) {
  QString command = QString(info.windowClassClass()).toLower();
  if (command.endsWith(".py")) {
    // To fix cases like cinnamon-settings
    return command.left(command.length() - 3);
  }
  if (command.startsWith("org.gnome.")) {
    // To fix cases like org.gnome.nautilus
    return command.mid(command.lastIndexOf('.') + 1);
  }
  return command;
}

}  // namespace

bool TaskInfo::operator<(const TaskInfo& taskInfo) const {
  if (program == taskInfo.program) {
    // If same program, sort by creation time.
    bool found = false;
    for (const auto window : KWindowSystem::windows()) {
      if (window == wId) {
        found = true;
      } else if (window == taskInfo.wId) {
        return found;
      }
    }
    return true;
  }
  return program < taskInfo.program;
}

TaskHelper::TaskHelper()
    : currentDesktop_(KWindowSystem::currentDesktop()) {
  connect(KWindowSystem::self(), &KWindowSystem::currentDesktopChanged,
          this, &TaskHelper::onCurrentDesktopChanged);
}

std::vector<TaskInfo> TaskHelper::loadTasks(int screen, bool currentDesktopOnly) {
  std::vector<TaskInfo> tasks;
  for (const auto wId : KWindowSystem::windows()) {
    if (isValidTask(wId, screen, currentDesktopOnly)) {
      tasks.push_back(getTaskInfo(wId));
    }
  }

  std::stable_sort(tasks.begin(), tasks.end());
  return tasks;
}

bool TaskHelper::isValidTask(WId wId) {
  if (!KWindowSystem::hasWId(wId)) {
    return false;
  }

  KWindowInfo info(wId, NET::WMState | NET::WMWindowType, NET::WM2WindowClass);
  if (!info.valid()) {
    return false;
  }

  const auto windowType = info.windowType(NET::DockMask | NET::DesktopMask);
  if (windowType != NET::Normal && windowType != NET::Unknown) {
    return false;
  }

  const auto state = info.state();
  if (state & NET::SkipTaskbar) {
    return false;
  }

  // Filters out Crystal Dock dialogs.
  return getCommand(info) != "crystaldock";
}

bool TaskHelper::isValidTask(WId wId, int screen, bool currentDesktopOnly) {
  if (!isValidTask(wId)) {
    return false;
  }

  if (screen >= 0 && getScreen(wId) != screen) {
    return false;
  }

  if (currentDesktopOnly) {
    KWindowInfo info(wId, NET::WMDesktop);
    if (!info.valid() || (info.desktop() != currentDesktop_ && !info.onAllDesktops())) {
      return false;
    }
  }
  
  return true;
}

/* static */ TaskInfo TaskHelper::getBasicTaskInfo(WId wId) {
  KWindowInfo info(wId, NET::Properties(), NET::WM2WindowClass);
  return TaskInfo(wId, getProgram(info));
}

TaskInfo TaskHelper::getTaskInfo(WId wId) const {
  static constexpr int kIconLoadSize = 128;
  KWindowInfo info(wId, NET::WMVisibleName | NET::WMState, NET::WM2WindowClass);

  const auto program = getProgram(info);
  const auto command = getCommand(info);
  const auto name = info.visibleName();
  QPixmap icon = KWindowSystem::icon(wId, kIconLoadSize, kIconLoadSize, true /* scale */);

  return TaskInfo(wId, program, command, name, icon, info.state() == NET::DemandsAttention);
}

int TaskHelper::getScreen(WId wId) {
  const auto screens = QGuiApplication::screens();
  const auto screenCount = screens.size();
  if (screenCount == 1) {
    return 0;
  }

  KWindowInfo info(wId, NET::WMFrameExtents);
  const auto& geometry = info.frameGeometry();
  for (int screen = 0; screen < screenCount; ++screen) {
    const auto& screenGeometry = screens[screen]->geometry();
    if (screenGeometry.intersects(geometry)) {
      return screen;
    }
  }
  return -1;
}

}  // namespace crystaldock
