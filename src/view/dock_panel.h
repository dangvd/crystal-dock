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

#ifndef CRYSTALDOCK_DOCK_PANEL_H_
#define CRYSTALDOCK_DOCK_PANEL_H_

#include <memory>
#include <vector>

#include <QAction>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPoint>
#include <QRect>
#include <QSize>
#include <QString>
#include <QTimer>
#include <QWidget>

#include "display/window_system.h"

#include "add_panel_dialog.h"
#include "application_menu_settings_dialog.h"
#include "appearance_settings_dialog.h"
#include "dock_item.h"
#include "edit_launchers_dialog.h"
#include "task_manager_settings_dialog.h"
#include "wallpaper_settings_dialog.h"

namespace crystaldock {

class MultiDockView;

// A dock panel. The user can have multiple dock panels at the same time.
class DockPanel : public QWidget {
  Q_OBJECT

 public:
  static constexpr int k3DPanelThickness = 4;
  static constexpr int kActiveIndicatorSize = 32;
  static constexpr int kInactiveIndicatorSize = 18;

  // No pointer ownership.
  DockPanel(MultiDockView* parent, MultiDockModel* model, int dockId);
  virtual ~DockPanel() = default;

  int dockId() const { return dockId_; }
  PanelPosition position() const { return position_; }

  QRect screenGeometry() { return screenGeometry_; }

  void addPanelSettings(QMenu* menu);

  int itemSpacing() { return itemSpacing_; }

  bool isHorizontal() { return orientation_ == Qt::Horizontal; }
  bool isTop() { return position_ == PanelPosition::Top; }
  bool isBottom() { return position_ == PanelPosition::Bottom; }
  bool isLeft() { return position_ == PanelPosition::Left; }

  bool is3D() {
    return panelStyle_ == PanelStyle::Floating3D || panelStyle_ == PanelStyle::NonFloating3D;
  }

  // position of task indicators, y-coordinate if horizontal, x if vertical.
  int taskIndicatorPos();

 public slots:
  // Reloads the items and updates the dock.
  void reload();

  // Checks that the items are still valid, removes an invalid one and updates the dock.
  // Should be called after a program with no task is unpinned.
  // Will return as soon as an invalid one is found.
  void refresh();
  void delayedRefresh();

  void onCurrentDesktopChanged();
  void onCurrentActivityChanged();

  void onDockLaunchersChanged(int dockId) {
    if (dockId_ == dockId) {
      reload();
    }
  }

  void setStrut();

  void updatePosition(PanelPosition position) {
    setPosition(position);
    reload();
    saveDockConfig();
  }

  void updateVisibility(PanelVisibility visibility) {
    setVisibility(visibility);
    reload();
    saveDockConfig();
  }

  void changeFloatingStyle() {
    panelStyle_ = static_cast<PanelStyle>(static_cast<int>(panelStyle_) ^ 1);
    model_->setPanelStyle(panelStyle_);
    model_->saveAppearanceConfig();
  }

  void change3DStyle() {
    panelStyle_ = static_cast<PanelStyle>(static_cast<int>(panelStyle_) ^ 2);
    model_->setPanelStyle(panelStyle_);
    model_->saveAppearanceConfig();
  }

  void toggleApplicationMenu() {
    showApplicationMenu_ = !showApplicationMenu_;
    reload();
    saveDockConfig();
  }

  void togglePager();

  void updatePager() {
    if (showPager_) {
      reload();
    }
  }

  void toggleTaskManager() {
    model_->setShowTaskManager(dockId_, taskManagerAction_->isChecked());
    reload();
    saveDockConfig();
  }

  void toggleClock() {
    showClock_ = !showClock_;
    reload();
    saveDockConfig();
  }

  // Sets the dock on a specific screen given screen index.
  // Thus 0 is screen 1 and so on.
  // This doesn't refresh the dock.
  void setScreen(int screen);
  // Moves the dock to the new screen.
  void changeScreen(int screen);

  // Slot to update zoom animation.
  void updateAnimation();

  void showWaitCursor();
  void resetCursor();

  void showOnlineDocumentation();

  void about();

  // These are for global appearance settings.
  // Dock-specific settings are activated from menu items on the context menu
  // directly.
  void showAppearanceSettingsDialog();
  void showEditLaunchersDialog();
  void showApplicationMenuSettingsDialog();
  void showWallpaperSettingsDialog(int desktop);
  void showTaskManagerSettingsDialog();

  void addDock();
  void cloneDock();
  void removeDock();

  void onWindowAdded(const WindowInfo* info);
  void onWindowRemoved(std::string uuid);
  void onWindowLeftCurrentDesktop(std::string_view uuid);
  void onWindowLeftCurrentActivity(std::string_view uuid);

  void minimize() { leaveEvent(nullptr); }

 protected:
  virtual void paintEvent(QPaintEvent* e) override;
  virtual void mouseMoveEvent(QMouseEvent* e) override;
  virtual void mousePressEvent(QMouseEvent* e) override;
  virtual void enterEvent(QEnterEvent* e) override;
  virtual void leaveEvent(QEvent* e) override;

 private:
  // The space between the tooltip and the dock.
  static constexpr int kTooltipSpacing = 10;

  // Width/height of the panel in Auto Hide mode.
  static constexpr int kAutoHideSize = 1;

  bool autoHide() { return visibility_ == PanelVisibility::AutoHide; }

  bool isFloating() {
    return panelStyle_ == PanelStyle::Floating3D || panelStyle_ == PanelStyle::Floating2D;
  }

  void setPosition(PanelPosition position);

  void setVisibility(PanelVisibility visibility);

  void setPanelStyle(PanelStyle panelStyle);

  int itemCount() const { return static_cast<int>(items_.size()); }

  int applicationMenuItemCount() const { return showApplicationMenu_ ? 1 : 0; }

  int launcherItemCount() const {
    return model_->launcherConfigs(dockId_).size();
  }

  int pagerItemCount() const {
    return showPager_ ? WindowSystem::numberOfDesktops() : 0;
  }

  int clockItemCount() const {
    return showClock_ ? 1 : 0;
  }

  bool showTaskManager() { return model_->showTaskManager(dockId_); }

  void initUi();

  void createMenu();

  void loadDockConfig();
  void saveDockConfig();
  void loadAppearanceConfig();

  void initLaunchers();
  void initApplicationMenu();
  void initPager();
  void initTasks();
  void reloadTasks();

  // Returns true if it changes the dock layout (i.e. adding a new program icon).
  bool addTask(const WindowInfo* task);
  void removeTask(std::string_view uuid);
  void updateTask(const WindowInfo* task);
  void initClock();

  void initLayoutVars();

  // Updates width, height, items's size and position when the mouse is outside
  // the dock.
  void updateLayout();

  // Updates width, height, items's size and position given the mouse position.
  void updateLayout(int x, int y);

  // Resizes the task manager part of the panel. This needs to not interfere
  // with the zooming.
  void resizeTaskManager();

  void setStrut(int width);

  // Updates the active item given the mouse position.
  void updateActiveItem(int x, int y);

  // Returns the size given the distance to the mouse.
  int parabolic(int x);

  MultiDockView* parent_;

  // The model.
  MultiDockModel* model_;
  int dockId_;

  // Config variables.

  PanelPosition position_;
  int screen_;  // the screen (as screen index) that the dock is on.
  PanelVisibility visibility_;
  bool showApplicationMenu_;
  bool showPager_;
  bool showClock_;
  int minSize_;
  int maxSize_;
  float spacingFactor_;  // item spacing as ratio of minSize, in (0, 1) range.
  QColor backgroundColor_;  // including alpha.
  bool showBorder_;
  QColor borderColor_;  // no alpha.
  int tooltipFontSize_;
  PanelStyle panelStyle_;

  // Non-config variables.

  int tooltipSize_;  // height if horizontal, width if vertical.
  int itemSpacing_;  // space between items.
  int margin3D_;
  int floatingMargin_;  // margin around the dock in floating mode.
  int minWidth_;
  int maxWidth_;
  int minHeight_;
  int maxHeight_;
  int parabolicMaxX_;
  QRect screenGeometry_;  // the geometry of the screen that the dock is on.

  // Number of animation steps when zooming in and out.
  int numAnimationSteps_;
  // Animation speed, between 0 and 31. The higher the faster.
  int animationSpeed_;

  Qt::Orientation orientation_;

  // The list of all dock items.
  std::vector<std::unique_ptr<DockItem>> items_;
  int activeItem_ = -1;

  // Context (right-click) menu.
  QMenu menu_;
  QAction* positionTop_;
  QAction* positionBottom_;
  QAction* positionLeft_;
  QAction* positionRight_;
  QAction* visibilityAlwaysVisibleAction_;
  QAction* visibilityAutoHideAction_;
  QAction* visibilityAlwaysOnTopAction_;
  QAction* applicationMenuAction_;
  QAction* pagerAction_;
  QAction* taskManagerAction_;
  QAction* clockAction_;
  QAction* floatingStyleAction_;
  QAction* style3DAction_;
  // Actions to set the dock on a specific screen.
  std::vector<QAction*> screenActions_;

  QMessageBox aboutDialog_;
  AddPanelDialog addPanelDialog_;
  AppearanceSettingsDialog appearanceSettingsDialog_;
  EditLaunchersDialog editLaunchersDialog_;
  ApplicationMenuSettingsDialog applicationMenuSettingsDialog_;
  WallpaperSettingsDialog wallpaperSettingsDialog_;
  TaskManagerSettingsDialog taskManagerSettingsDialog_;

  bool isMinimized_;
  bool isEntering_;
  bool isLeaving_;
  bool isAnimationActive_;
  std::unique_ptr<QTimer> animationTimer_;
  int currentAnimationStep_;
  int backgroundWidth_;
  int startBackgroundWidth_;
  int endBackgroundWidth_;
  int backgroundHeight_;
  int startBackgroundHeight_;
  int endBackgroundHeight_;

  // For recording the mouse position before doing entering animation
  // so that we can show the correct tooltip at the end of it.
  int mouseX_;
  int mouseY_;

  // For activation delay.
  qint64 enterTime_;
  int lastMouseX_;
  int lastMouseY_;

  friend class Program;  // for leaveEvent.
  friend class DockPanelTest;
  friend class ConfigDialogTest;
  friend class EditLaunchersDialogTest;
};

}  // namespace crystaldock

#endif  // CRYSTALDOCK_DOCK_PANEL_H_
