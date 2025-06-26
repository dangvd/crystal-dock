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
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPoint>
#include <QRect>
#include <QSize>
#include <QString>
#include <QTimer>
#include <QWheelEvent>
#include <QWidget>

#include <display/window_system.h>
#include <model/multi_dock_model.h>

#include "add_panel_dialog.h"
#include "application_menu_settings_dialog.h"
#include "appearance_settings_dialog.h"
#include "dock_item.h"
#include "edit_launchers_dialog.h"
#include "task_manager_settings_dialog.h"
#include "trash.h"
#include "wallpaper_settings_dialog.h"

namespace crystaldock {

class MultiDockView;
class VolumeControl;

// A dock panel. The user can have multiple dock panels at the same time.
class DockPanel : public QWidget {
  Q_OBJECT

 public:
  static constexpr int kIconLoadSize = 128;
  // For certain actions like Lock Screen, we need to delay execution for a bit
  // to avoid graphical issues.
  static constexpr int kExecutionDelayMs = 300;

  static constexpr int k3DPanelThickness = 4;
  static constexpr int kIndicatorSizeGlass = 10;  // for Glass 2D/3D.
  static constexpr int kIndicatorSizeFlat2D = 6;
  static constexpr int kIndicatorSizeMetal2D = 8;
  static constexpr int kIndicatorSpacing = 3;
  static constexpr int kIndicatorMarginGlass2D = 4;
  static constexpr float kSpacingMultiplier = 0.5;  // for Glass 2D/3D and Flat 2D.
  static constexpr float kSpacingMultiplierMetal2D = 0.33;

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
    return panelStyle_ == PanelStyle::Glass3D_Floating || panelStyle_ == PanelStyle::Glass3D_NonFloating;
  }

  bool isGlass2D() {
    return panelStyle_ == PanelStyle::Glass2D_Floating || panelStyle_ == PanelStyle::Glass2D_NonFloating;
  }

  bool isGlass() { return is3D() || isGlass2D(); }

  bool isFlat2D() {
    return panelStyle_ == PanelStyle::Flat2D_Floating || panelStyle_ == PanelStyle::Flat2D_NonFloating;
  }

  bool isMetal2D() {
    return panelStyle_ == PanelStyle::Metal2D_Floating || panelStyle_ == PanelStyle::Metal2D_NonFloating;
  }

  // position of task indicators, y-coordinate if horizontal, x if vertical.
  int taskIndicatorPos();

  // Gets number of items for an application. Useful when Group Tasks By Application is Off.
  int itemCount(const QString& appId);

  // Update pinned status of an application. Useful when Group Tasks By Application is Off.
  void updatePinnedStatus(const QString& appId, bool pinned);

  // Sets whether the dock is showing some popup menu.
  void setShowingPopup(bool showingPopup);

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

  void updatePosition(PanelPosition position);

  void updateVisibility(PanelVisibility visibility);

  void setAutoHide(bool on = true);

  void changeFloatingStyle() {
    panelStyle_ = static_cast<PanelStyle>(static_cast<int>(panelStyle_) ^ 1);
    model_->setPanelStyle(panelStyle_);
    model_->saveAppearanceConfig();
  }

  void changePanelStyle(PanelStyle style) {
    model_->setPanelStyle(style);
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

  void toggleTrash() {
    showTrash_ = !showTrash_;
    reload();
    saveDockConfig();
  }

  void toggleVolumeControl() {
    showVolumeControl_ = !showVolumeControl_;
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
  void onWindowRemoved(void* window);
  void onWindowLeftCurrentDesktop(void* window);
  void onWindowLeftCurrentActivity(void* window);
  void onWindowGeometryChanged(const WindowInfo* task);
  void onWindowStateChanged(const WindowInfo* info);
  void onWindowTitleChanged(const WindowInfo* info);
  void onActiveWindowChanged();

  void minimize() { leaveEvent(nullptr); }

 protected:
  virtual void paintEvent(QPaintEvent* e) override;
  virtual void mouseMoveEvent(QMouseEvent* e) override;
  virtual void mousePressEvent(QMouseEvent* e) override;
  virtual void wheelEvent(QWheelEvent* e) override;
  virtual void enterEvent(QEnterEvent* e) override;
  virtual void leaveEvent(QEvent* e) override;
  virtual void dragEnterEvent(QDragEnterEvent* e) override;
  virtual void dragMoveEvent(QDragMoveEvent* e) override;
  virtual void dropEvent(QDropEvent* e) override;

 private:
  // The space between the tooltip and the dock.
  static constexpr int kTooltipSpacing = 10;

  bool autoHide() const { return visibility_ == PanelVisibility::AutoHide; }
  bool intellihide() const { return visibility_ == PanelVisibility::IntelligentAutoHide; }

  bool isFloating() const {
    return panelStyle_ == PanelStyle::Glass3D_Floating ||
        panelStyle_ == PanelStyle::Glass2D_Floating ||
        panelStyle_ == PanelStyle::Flat2D_Floating ||
        panelStyle_ == PanelStyle::Metal2D_Floating;
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

  int trashItemCount() const {
    return showTrash_ ? 1 : 0;
  }

  int volumeControlItemCount() const {
    return showVolumeControl_ ? 1 : 0;
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
  void removeTask(void* window);
  void updateTask(const WindowInfo* task);
  bool isValidTask(const WindowInfo* task);
  bool hasTask(void* window);

  void initClock();

  void initTrash();

  void initVolumeControl();

  void initLayoutVars();

  QRect getMinimizedDockGeometry();

  // Updates width, height, items's size and position when the mouse is outside
  // the dock.
  void updateLayout();

  // Updates width, height, items's size and position given the mouse position.
  void updateLayout(int x, int y);

  // Checks if the mouse has actually entered the dock panel's visibility area.
  bool checkMouseEnter(int x, int y);

  // Should the dock hide in Intelligent Auto Hide mode?
  bool intellihideShouldHide(void* excluding_window = nullptr);

  // Hides/unhides the dock in Intelligent Auto Hide mode if necessary.
  void intellihideHideUnhide(void* excluding_window = nullptr);

  // Is the dock empty?
  // The dock is empty if it has no dock items (separators excluded).
  bool isEmpty();

  // Resizes the task manager part of the panel. This needs to not interfere
  // with the zooming.
  void resizeTaskManager();

  void setStrut(int width);

  // Sets the visibility and mouse event region mask appropriately.
  void setMask();

  // Updates the active item given the mouse position.
  void updateActiveItem(int x, int y);

  // Drawing logic for each dock style.
  void drawGlass3D(QPainter& painter);
  void draw2D(QPainter& painter);  // for 2D styles.

  void drawTooltip(QPainter& painter);

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
  bool showTrash_;
  bool showVolumeControl_;
  int minSize_;
  int maxSize_;
  float spacingFactor_;  // item spacing as ratio of minSize, in (0, 1) range.
  QColor backgroundColor_;  // including alpha.
  QColor borderColor_;  // no alpha.
  int tooltipFontSize_;
  PanelStyle panelStyle_;

  // Non-config variables.

  int tooltipSize_;  // height (tooltip only shown in horizontal positions).
  int itemSpacing_;  // space between items.
  int margin3D_;
  int floatingMargin_;  // margin around the dock in floating mode.

  // Width/height of the dock area when minimized.
  int minWidth_;
  int minHeight_;
  // Width/height of the dock background area (e.g. the background panel in 2D) when minimized.
  int minBackgroundWidth_;
  int minBackgroundHeight_;
  // Width/height of the dock area when maximized (zoomed in).
  int maxWidth_;
  int maxHeight_;

  int parabolicMaxX_;
  QRect screenGeometry_;  // the geometry of the screen that the dock is on.
  wl_output* screenOutput_;

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
  QAction* visibilityIntelligentAutoHideAction_;
  QAction* visibilityAutoHideAction_;
  QAction* visibilityAlwaysOnTopAction_;
  QAction* applicationMenuAction_;
  QAction* pagerAction_;
  QAction* taskManagerAction_;
  QAction* clockAction_;
  QAction* trashAction_;
  QAction* volumeControlAction_;
  QAction* floatingStyleAction_;
  QAction* glass3DStyleAction_;
  QAction* glass2DStyleAction_;
  QAction* flat2DStyleAction_;
  QAction* metal2DStyleAction_;
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
  // This is needed for Intelligent Auto Hide mode because it could be either be visible
  // or hidden when minimized. Whereas for Auto Hide mode, it is always hidden when minimized.
  bool isHidden_;
  bool isEntering_;
  bool isLeaving_;
  bool isAnimationActive_;
  bool isShowingPopup_;
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

  friend class Program;  // for leaveEvent.
  friend class DockPanelTest;
  friend class ConfigDialogTest;
  friend class EditLaunchersDialogTest;
};

}  // namespace crystaldock

#endif  // CRYSTALDOCK_DOCK_PANEL_H_
