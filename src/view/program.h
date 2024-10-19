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

#ifndef CRYSTALDOCK_PROGRAM_H_
#define CRYSTALDOCK_PROGRAM_H_

#include <vector>

#include <QAction>
#include <QMenu>
#include <QPixmap>
#include <QTimer>

#include "display/window_system.h"

#include "icon_based_dock_item.h"

namespace crystaldock {

struct ProgramTask {
  std::string uuid;
  QString name;  // e.g. home -- Dolphin
  bool demandsAttention;

  ProgramTask(std::string_view uuid2, QString name2, bool demandsAttention2)
    : uuid(uuid2), name(name2), demandsAttention(demandsAttention2) {}
};

class Program : public QObject, public IconBasedDockItem {
  Q_OBJECT

 public:
  Program(DockPanel* parent, MultiDockModel* model, const QString& appId,
          const QString& label, Qt::Orientation orientation, const QString& iconName,
          int minSize, int maxSize, const QString& command, bool isAppMenuEntry, bool pinned);

  Program(DockPanel* parent, MultiDockModel* model, const QString& appId,
          const QString& label, Qt::Orientation orientation, const QString& iconName,
          int minSize, int maxSize);

  Program(DockPanel* parent, MultiDockModel* model, const QString& appId,
          const QString& label, Qt::Orientation orientation, const QPixmap& icon,
          int minSize, int maxSize);

  void init();

  ~Program() override = default;

  void draw(QPainter* painter) const override;

  void mousePressEvent(QMouseEvent* e) override;

  QString getLabel() const override;

  bool addTask(const WindowInfo* task) override;

  bool updateTask(const WindowInfo* task) override;

  bool removeTask(std::string_view uuid) override;

  bool hasTask(std::string_view uuid) override;

  bool beforeTask(const QString& program) override;

  bool shouldBeRemoved() override { return taskCount() == 0 && !pinned_; }

  virtual void maybeResetActiveWindow(QMouseEvent* e) override {
    if (e->button() != Qt::LeftButton) { WindowSystem::resetActiveWindow(); }
  }

  void setDemandsAttention(bool demandsAttention) override;

  int taskCount() const { return static_cast<int>(tasks_.size()); }

  bool active() const { return getActiveTask() >= 0; }

  int getActiveTask() const {
    for (int i = 0; i < static_cast<int>(tasks_.size()); ++i) {
      if (WindowSystem::activeWindow() == tasks_[i].uuid) {
        return i;
      }
    }
    return -1;
  }

  bool pinned() { return pinned_; }
  void pinUnpin();

  void launch();
  static void launch(const QString& command);
  static void lockScreen() { launch("xdg-screensaver lock"); }

  void closeAllWindows();

 private:
  static constexpr int kLaunchingAcknowledgementDurationMs = 3000;  // 3 seconds.

  void createMenu();

  void updateDemandsAttention();

  void updateMenu();

  QString appId_;
  QString command_;
  // Is an entry on the App Menu, exluding system commands such as Lock Screen / Shut Down.
  bool isAppMenuEntry_;
  bool pinned_;
  std::vector<ProgramTask> tasks_;

  // Context (right-click) menu.
  QMenu menu_;
  QAction* pinAction_;
  QAction* closeAction_;

  // Demands attention logic.
  bool demandsAttention_;
  QTimer animationTimer_;
  bool attentionStrong_;

  // For launching acknowledgement.
  bool launching_;

  friend class DockPanel;
};

}  // namespace crystaldock

#endif  // CRYSTALDOCK_PROGRAM_H_
