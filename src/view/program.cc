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

#include "program.h"

#include <iostream>

#include <QGuiApplication>
#include <QMessageBox>
#include <QProcess>
#include <QSettings>
#include <QTimer>

#include <KWindowSystem>

#include "dock_panel.h"
#include <utils/command_utils.h>
#include <utils/draw_utils.h>

namespace crystaldock {

Program::Program(DockPanel* parent, MultiDockModel* model, const QString& label,
    Qt::Orientation orientation, const QString& iconName, int minSize,
    int maxSize, const QString& command, const QString& taskCommand,
    bool isAppMenuEntry, bool pinned)
    : IconBasedDockItem(parent, label, orientation, iconName, minSize, maxSize),
      model_(model),
      command_(command),
      taskCommand_(taskCommand),
      isAppMenuEntry_(isAppMenuEntry),
      pinned_(pinned),
      demandsAttention_(false),
      attentionStrong_(false) {
  init();
}

Program::Program(DockPanel* parent, MultiDockModel* model, const QString& label,
    Qt::Orientation orientation, const QPixmap& icon, int minSize,
    int maxSize, const QString& taskCommand)
    : IconBasedDockItem(parent, label, orientation, icon, minSize, maxSize),
      model_(model),
      command_(taskCommand),
      taskCommand_(taskCommand),
      isAppMenuEntry_(false),
      pinned_(false),
      demandsAttention_(false),
      attentionStrong_(false) {
  init();
}

void Program::init() {
  createMenu();

  animationTimer_.setInterval(500);
  connect(&animationTimer_, &QTimer::timeout, this, [this]() {
    attentionStrong_ = !attentionStrong_;
    parent_->update();
  });
}

void Program::draw(QPainter *painter) const {
  if ((!tasks_.empty() && active()) || attentionStrong_) {
    drawHighlightedIcon(model_->backgroundColor(), left_, top_, getWidth(), getHeight(),
                        5, size_ / 8, painter);
  } else if (!tasks_.empty()) {
    drawHighlightedIcon(model_->backgroundColor(), left_, top_, getWidth(), getHeight(),
                        5, size_ / 8, painter, 0.25);
  }
  IconBasedDockItem::draw(painter);
}

void Program::mousePressEvent(QMouseEvent* e) {
  if (e->button() == Qt::LeftButton) { // Run the application.
    if (command_ == kShowDesktopCommand) {
      KWindowSystem::setShowingDesktop(!KWindowSystem::showingDesktop());
    } else if (isCommandLockScreen(command_)) {
      parent_->leaveEvent(nullptr);
      QTimer::singleShot(500, []() {
        lockScreen();
      });
    } else {
      if (tasks_.empty()) {
        launch();
      } else {
        const auto mod = QGuiApplication::keyboardModifiers();
        if (mod & Qt::ShiftModifier) {
          launch();
        } else {
          const auto activeTask = getActiveTask();
          if (activeTask >= 0) {
            if (tasks_.size() == 1) {
              KWindowSystem::minimizeWindow(tasks_[0].wId);
            } else {
              // Cycles through tasks.
              auto nextTask = (activeTask < static_cast<int>(tasks_.size() - 1)) ?
                    (activeTask + 1) : 0;
              KWindowSystem::forceActiveWindow(tasks_[nextTask].wId);
            }
          } else {
            for (unsigned i = 0; i < tasks_.size(); ++i) {
              KWindowSystem::forceActiveWindow(tasks_[i].wId);
            }
          }
        }
      }
    }
  } else if (e->button() == Qt::RightButton) {
    menu_.popup(e->globalPos());
  }
}

QString Program::getLabel() const {
  const unsigned taskCount = tasks_.size();
  return (taskCount > 1) ?
      label_ + " (" + QString::number(tasks_.size()) + " instances)" :
      label_;
}

bool Program::addTask(const TaskInfo& task) {
  if (areTheSameCommand(taskCommand_, task.command)) {
    tasks_.push_back(ProgramTask(task.wId, task.name, task.demandsAttention));
    if (task.demandsAttention) {
      setDemandsAttention(true);
    }
    return true;
  }
  return false;
}

bool Program::updateTask(const TaskInfo& task) {
  if (!areTheSameCommand(taskCommand_, task.command)) {
    return false;
  }

  for (auto& existingTask : tasks_) {
    if (existingTask.wId == task.wId) {
      existingTask.demandsAttention = task.demandsAttention;
      updateDemandsAttention();
      return true;
    }
  }

  return false;
}

bool Program::removeTask(WId wId) {
  for (int i = 0; i < static_cast<int>(tasks_.size()); ++i) {
    if (tasks_[i].wId == wId) {
      tasks_.erase(tasks_.begin() + i);
      return true;
    }
  }
  return false;
}

bool Program::hasTask(WId wId) {
  for (const auto& task : tasks_) {
    if (task.wId == wId) {
      return true;
    }
  }
  return false;
}

bool Program::beforeTask(const QString& program) {
  return pinned_ || label_ < program;
}

void Program::launch() {
  launch(command_);
  parent_->showWaitCursor();
  parent_->update();
}

void Program::pinUnpin() {
  pinned_ = !pinned_;
  if (pinned_) {
    model_->addLauncher(parent_->dockId(), LauncherConfig(label_, iconName_, command_));
  } else {  // !pinned
    model_->removeLauncher(parent_->dockId(), command_);
    if (shouldBeRemoved()) {
      parent_->delayedRefresh();
    }
  }
}

void Program::launch(const QString& command) {
  std::cout << "Launching " << command.toStdString() << std::endl;
  QStringList list = QProcess::splitCommand(command);
  if (!QProcess::startDetached(list.at(0), list.mid(1))) {
    QMessageBox warning(QMessageBox::Warning, "Error",
                        QString("Could not run command: ") + command,
                        QMessageBox::Ok, nullptr, Qt::Tool);
    warning.exec();
  }
}

void Program::createMenu() {
  if (isAppMenuEntry_) {
    pinAction_ = menu_.addAction(
        QString("Pinned"), this,
        [this] {
          pinUnpin();
        });
    pinAction_->setCheckable(true);
    pinAction_->setChecked(pinned_);

    menu_.addAction(QIcon::fromTheme("list-add"), QString("&New Instance"), this,
                    [this] { launch(); });

    menu_.addSeparator();
  }

  menu_.addAction(QIcon::fromTheme("configure"), QString("Edit &Launchers"), parent_,
                  [this] { parent_->showEditLaunchersDialog(); });

  if (model_->showTaskManager(parent_->dockId())) {
    menu_.addAction(QIcon::fromTheme("configure"),
                    QString("Task Manager &Settings"),
                    parent_,
                    [this] { parent_->showTaskManagerSettingsDialog(); });
  }

  menu_.addSeparator();
  parent_->addPanelSettings(&menu_);
}

void Program::setDemandsAttention(bool value) {
  if (demandsAttention_ == value) {
    return;
  }

  demandsAttention_ = value;
  if (demandsAttention_) {
    animationTimer_.start();
  } else if (animationTimer_.isActive()) {
    animationTimer_.stop();
  }
}

void Program::updateDemandsAttention() {
  for (const auto& task : tasks_) {
    if (task.demandsAttention) {
      setDemandsAttention(true);
      return;
    }
  }
  setDemandsAttention(false);
}

}  // namespace crystaldock
