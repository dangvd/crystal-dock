/*
 * This file is part of Crystal Dock.
 * Copyright (C) 2023 Viet Dang (dangvd@gmail.com)
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

#include "dock_panel.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <utility>

#include <QColor>
#include <QCursor>
#include <QGuiApplication>
#include <QIcon>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QPainter>
#include <QProcess>
#include <QScreen>
#include <QSize>
#include <QStringList>
#include <QVariant>

#include "add_panel_dialog.h"
#include "application_menu.h"
#include "clock.h"
#include "desktop_selector.h"
#include "multi_dock_view.h"
#include "program.h"
#include "separator.h"
#include <display/window_system.h>
#include <utils/command_utils.h>
#include <utils/draw_utils.h>

namespace crystaldock {

namespace {
  bool isValidTask(const WindowInfo* task, const MultiDockModel* model) {
    if (task->skipTaskbar || std::string(task->appId) == "crystal-dock") {
      return false;
    }

    if (task->onAllDesktops || !model->currentDesktopTasksOnly()
        || task->desktop == WindowSystem::currentDesktop()) {
      return true;
    }

    return false;
  }
}

const int DockPanel::kTooltipSpacing;
const int DockPanel::kAutoHideSize;

DockPanel::DockPanel(MultiDockView* parent, MultiDockModel* model, int dockId)
    : QWidget(),
      parent_(parent),
      model_(model),
      dockId_(dockId),
      visibility_(PanelVisibility::AlwaysVisible),
      showPager_(false),
      showClock_(false),
      showBorder_(true),
      aboutDialog_(QMessageBox::Information, "About Crystal Dock",
                   QString("<h3>Crystal Dock 2.0 alpha</h3>")
                   + "<p>Copyright (C) 2024 Viet Dang (dangvd@gmail.com)"
                   + "<p><a href=\"https://github.com/dangvd/crystal-dock\">https://github.com/dangvd/crystal-dock</a>"
                   + "<p>License: GPLv3",
                   QMessageBox::Ok, this, Qt::Tool),
      addPanelDialog_(this, model, dockId),
      appearanceSettingsDialog_(this, model),
      editLaunchersDialog_(this, model, dockId),
      applicationMenuSettingsDialog_(this, model),
      wallpaperSettingsDialog_(this, model),
      taskManagerSettingsDialog_(this, model),
      isMinimized_(true),
      isResizing_(false),
      isEntering_(false),
      isLeaving_(false),
      isAnimationActive_(false),
      animationTimer_(std::make_unique<QTimer>(this)) {  
  setAttribute(Qt::WA_TranslucentBackground);
  setWindowFlag(Qt::FramelessWindowHint);
  setMouseTracking(true);

  createMenu();
  loadDockConfig();
  loadAppearanceConfig();
  initUi();
  WindowSystem::setDockType(this, minHeight_);

  connect(animationTimer_.get(), SIGNAL(timeout()), this,
      SLOT(updateAnimation()));
  connect(WindowSystem::self(), SIGNAL(numberOfDesktopsChanged(int)),
      this, SLOT(updatePager()));
  connect(WindowSystem::self(), SIGNAL(currentDesktopChanged(std::string_view)),
          this, SLOT(onCurrentDesktopChanged()));
  connect(WindowSystem::self(), SIGNAL(activeWindowChanged(std::string_view)),
          this, SLOT(update()));
  connect(WindowSystem::self(), SIGNAL(windowAdded(const WindowInfo*)),
          this, SLOT(onWindowAdded(const WindowInfo*)));
  connect(WindowSystem::self(), SIGNAL(windowRemoved(std::string)),
          this, SLOT(onWindowRemoved(std::string)));
  connect(WindowSystem::self(), SIGNAL(windowLeftCurrentDesktop(std::string_view)),
          this, SLOT(onWindowLeftCurrentDesktop(std::string_view)));
  /*
  connect(WindowSystem::self(),
          SIGNAL(windowChanged(std::string_view, NET::Properties, NET::Properties2)),
          this,
          SLOT(onWindowChanged(std::string_view, NET::Properties, NET::Properties2)));
  */
  connect(model_, SIGNAL(appearanceOutdated()), this, SLOT(update()));
  connect(model_, SIGNAL(appearanceChanged()), this, SLOT(reload()));
  connect(model_, SIGNAL(dockLaunchersChanged(int)),
          this, SLOT(onDockLaunchersChanged(int)));
}

void DockPanel::resize(int w, int h) {
  isResizing_ = true;
  QWidget::resize(w, h);
  int x, y;
  if (position_ == PanelPosition::Top) {
    x = (screenGeometry_.width() - w) / 2;
    y = 0;
  } else if (position_ == PanelPosition::Bottom) {
    x = (screenGeometry_.width() - w) / 2;
    y = screenGeometry_.height() - h;
  } else if (position_ == PanelPosition::Left) {
    x = 0;
    y = (screenGeometry_.height() - h) / 2;
  } else {  // Right
    x = screenGeometry_.width() - w;
    y = (screenGeometry_.height() - h) / 2;
  }
  if (isMinimized_) {
    move(x + screenGeometry_.x(), y + screenGeometry_.y());
  }
  if (w == minWidth_ && h == minHeight_) {
    minX_ = x + screenGeometry_.x();
    minY_ = y + screenGeometry_.y();
  }
  // This is to fix the bug that if launched from a KDE Plasma Quicklaunch,
  // the dock panel still doesn't show on all desktops even though
  // we've already called this in the constructor.
  // WindowSystem::setOnAllDesktops(winId(), true);
  isResizing_ = false;
}

void DockPanel::reload() {
  loadAppearanceConfig();
  items_.clear();
  initUi();
  update();
}

void DockPanel::refresh() {
  for (int i = 0; i < itemCount(); ++i) {
    if (items_[i]->shouldBeRemoved()) {
      items_.erase(items_.begin() + i);
      resizeTaskManager();
      return;
    }
  }
}

void DockPanel::delayedRefresh() {
  QTimer::singleShot(100 /* msecs */, this, SLOT(refresh()));
}

void DockPanel::onCurrentDesktopChanged() {
  reloadTasks();
}

void DockPanel::setStrut() {
  switch(visibility_) {
    case PanelVisibility::AlwaysVisible:
      setStrut(isHorizontal() ? minHeight_ : minWidth_);
      break;
    case PanelVisibility::AutoHide:  // fall through
    case PanelVisibility::WindowsCanCover:
      setStrut(1);
      break;
    case PanelVisibility::WindowsGoBelow:  // fall through
    case PanelVisibility::WindowsCanCover_Quiet:
    default:
      setStrut(0);
      break;
  }
}

void DockPanel::togglePager() {
  showPager_ = !showPager_;
  reload();
  saveDockConfig();
}

void DockPanel::setScreen(int screen) {
  screen_ = screen;
  for (int i = 0; i < static_cast<int>(screenActions_.size()); ++i) {
    screenActions_[i]->setChecked(i == screen);
  }
  screenGeometry_ = QGuiApplication::screens()[screen]->geometry();
}

void DockPanel::updateAnimation() {
  for (const auto& item : items_) {
    item->nextAnimationStep();
  }
  ++currentAnimationStep_;
  backgroundWidth_ = startBackgroundWidth_
      + (endBackgroundWidth_ - startBackgroundWidth_)
          * currentAnimationStep_ / numAnimationSteps_;
  backgroundHeight_ = startBackgroundHeight_
      + (endBackgroundHeight_ - startBackgroundHeight_)
          * currentAnimationStep_ / numAnimationSteps_;
  if (currentAnimationStep_ == numAnimationSteps_) {
    animationTimer_->stop();
    isAnimationActive_ = false;
    if (isLeaving_) {
      isLeaving_ = false;
      updateLayout();
    } else {
      showTooltip(mouseX_, mouseY_);
    }
  }
  repaint();
}

void DockPanel::resetCursor() {
  setCursor(QCursor(Qt::ArrowCursor));
}

void DockPanel::showOnlineDocumentation() {
  Program::launch(
      "xdg-open https://github.com/dangvd/crystal-dock/wiki/Documentation");
}

void DockPanel::about() {
  aboutDialog_.exec();
}

void DockPanel::showAppearanceSettingsDialog() {
  appearanceSettingsDialog_.reload();
  appearanceSettingsDialog_.show();
  appearanceSettingsDialog_.raise();
  appearanceSettingsDialog_.activateWindow();
}

void DockPanel::showEditLaunchersDialog() {
  editLaunchersDialog_.reload();
  editLaunchersDialog_.show();
  editLaunchersDialog_.raise();
  editLaunchersDialog_.activateWindow();
}

void DockPanel::showApplicationMenuSettingsDialog() {
  applicationMenuSettingsDialog_.reload();
  applicationMenuSettingsDialog_.show();
  applicationMenuSettingsDialog_.raise();
  applicationMenuSettingsDialog_.activateWindow();
}

void DockPanel::showWallpaperSettingsDialog(int desktop) {
  wallpaperSettingsDialog_.setFor(desktop, screen_);
  wallpaperSettingsDialog_.show();
  wallpaperSettingsDialog_.raise();
  wallpaperSettingsDialog_.activateWindow();
}

void DockPanel::showTaskManagerSettingsDialog() {
  taskManagerSettingsDialog_.reload();
  taskManagerSettingsDialog_.show();
  taskManagerSettingsDialog_.raise();
  taskManagerSettingsDialog_.activateWindow();
}

void DockPanel::addDock() {
  addPanelDialog_.setMode(AddPanelDialog::Mode::Add);
  addPanelDialog_.show();
  addPanelDialog_.raise();
  addPanelDialog_.activateWindow();
}

void DockPanel::cloneDock() {
  addPanelDialog_.setMode(AddPanelDialog::Mode::Clone);
  addPanelDialog_.show();
  addPanelDialog_.raise();
  addPanelDialog_.activateWindow();
}

void DockPanel::removeDock() {
  if (model_->dockCount() == 1) {
    QMessageBox message(QMessageBox::Information, "Remove Panel",
                        "The last panel cannot be removed.",
                        QMessageBox::Ok, this, Qt::Tool);
    message.exec();
    return;
  }

  QMessageBox question(QMessageBox::Question, "Remove Panel",
                       "Do you really want to remove this panel?",
                       QMessageBox::Yes | QMessageBox::No, this, Qt::Tool);
  if (question.exec() == QMessageBox::Yes) {
    deleteLater();
    model_->removeDock(dockId_);
  }
}

void DockPanel::onWindowAdded(const WindowInfo* info) {
  if (info->appId == "crystal-dock") {
    x_ = info->x;
    y_ = info->y;
  }

  if (!showTaskManager()) {
    return;
  }

  if (isValidTask(info, model_)) {
    // Now inserts it.
    addTask(info);
    resizeTaskManager();
  }
}

void DockPanel::onWindowRemoved(std::string uuid) {
  if (!showTaskManager()) {
    return;
  }

  removeTask(uuid);
}

void DockPanel::onWindowLeftCurrentDesktop(std::string_view uuid) {
  if (showTaskManager() && model_->currentDesktopTasksOnly()) {
    removeTask(uuid);
  }
}

/*
void DockPanel::onWindowChanged(std::string_view uuid, NET::Properties properties,
                                NET::Properties2 properties2) {
  if (!showTaskManager()) {
    return;
  }

  if (uuid != winId() && uuid != tooltip_.winId() &&
      taskHelper_.isValidTask(uuid)) {
    auto screen = model_->currentScreenTasksOnly() ? screen_ : -1;
    if (properties & NET::WMDesktop || properties & NET::WMGeometry) {
      if (taskHelper_.isValidTask(uuid, screen, model_->currentDesktopTasksOnly())) {
        addTask(uuid);
        resizeTaskManager();
      } else {
        removeTask(uuid);
      }
    } else if (properties & NET::WMState) {
      updateTask(uuid);
    }
  }
}
*/

void DockPanel::paintEvent(QPaintEvent* e) {
  if (isResizing_) {
    return;  // to avoid potential flicker.
  }

  QPainter painter(this);

  if (isHorizontal()) {
    const int y = (position_ == PanelPosition::Top)
                  ? 0 : height() - backgroundHeight_;
    fillRoundedRect((width() - backgroundWidth_) / 2, y, backgroundWidth_ - 1, backgroundHeight_ - 1,
                    backgroundHeight_ / 16, showBorder_, borderColor_, backgroundColor_, &painter);
  } else {  // Vertical
    const int x =  (position_ == PanelPosition::Left)
                   ? 0 : width() - backgroundWidth_;
    fillRoundedRect(x, (height() - backgroundHeight_) / 2, backgroundWidth_ - 1, backgroundHeight_ - 1,
                    backgroundWidth_ / 16, showBorder_, borderColor_, backgroundColor_, &painter);
  }

  // Draw the items from the end to avoid zoomed items getting clipped by
  // non-zoomed items.
  for (int i = itemCount() - 1; i >= 0; --i) {
    items_[i]->draw(&painter);
  }

  // Draw tooltip.
  if (activeItem_ >= 0 && activeItem_ < static_cast<int>(items_.size())) {
    QFont font;
    font.setPointSize(24);
    painter.setFont(font);
    drawBorderedText(items_[activeItem_]->left_, itemSpacing_ + 10,
                     items_[activeItem_]->getLabel(), 2 /* borderWidth */,
                     Qt::black, Qt::white, &painter);
  }
}

void DockPanel::mouseMoveEvent(QMouseEvent* e) {
  if (isEntering_ && !autoHide()) {
    // Don't do the parabolic zooming if the mouse is near the border.
    // Quite often the user was just scrolling a window etc.
    if ((position_ == PanelPosition::Bottom && e->position().y() < itemSpacing_ / 2) ||
        (position_ == PanelPosition::Top && e->position().y() > height() - itemSpacing_ / 2) ||
        (position_ == PanelPosition::Left && e->position().x() > width() - itemSpacing_ / 2) ||
        (position_ == PanelPosition::Right && e->position().x() < itemSpacing_ / 2)) {
      return;
    }
  }

  if (isAnimationActive_) {
    return;
  }

  if (!isEntering_) {
    showTooltip(e->position().x(), e->position().y());
  }
  updateLayout(e->position().x(), e->position().y());
}

void DockPanel::mousePressEvent(QMouseEvent* e) {
  if (isAnimationActive_) {
    return;
  }

  int i = findActiveItem(e->position().x(), e->position().y());
  if (i < 0 || i >= itemCount()) {
    return;
  }

  items_[i]->mousePressEvent(e);
}

void DockPanel::enterEvent (QEnterEvent* e) {
  isEntering_ = true;
  if (windowsCanCover()) {
    //WindowSystem::keepAbove(winId());
  }
}

void DockPanel::leaveEvent(QEvent* e) {
  if (windowsCanCover()) {
    //WindowSystem::keepBelow(winId());
  }

  if (isMinimized_) {
    return;
  }

  isLeaving_ = true;
  updateLayout();
  activeItem_ = -1;
}

void DockPanel::initUi() {
  initApplicationMenu();
  initPager();
  initLaunchers();
  initTasks();
  initClock();
  initLayoutVars();
  updateLayout();
  setStrut();
}

void DockPanel::addPanelSettings(QMenu* menu) {
  QAction* action = menu->addMenu(&menu_);
  action->setText("&Panel Settings");
  action->setIcon(QIcon::fromTheme("configure"));
}

void DockPanel::createMenu() {
  menu_.addAction(QIcon::fromTheme("list-add"), QString("&Add Panel"),
      this, SLOT(addDock()));
  menu_.addAction(QIcon::fromTheme("edit-copy"), QString("&Clone Panel"),
      this, SLOT(cloneDock()));
  menu_.addAction(QIcon::fromTheme("edit-delete"), QString("&Remove Panel"),
      this, SLOT(removeDock()));
  menu_.addSeparator();

  menu_.addAction(
      QIcon::fromTheme("configure"), QString("Appearance &Settings"), this,
      SLOT(showAppearanceSettingsDialog()));

  QMenu* position = menu_.addMenu(QString("&Position"));
  positionTop_ = position->addAction(QString("&Top"), this,
      [this]() { updatePosition(PanelPosition::Top); });
  positionTop_->setCheckable(true);
  positionBottom_ = position->addAction(QString("&Bottom"), this,
      [this]() { updatePosition(PanelPosition::Bottom); });
  positionBottom_->setCheckable(true);
  positionLeft_ = position->addAction(QString("&Left"), this,
      [this]() { updatePosition(PanelPosition::Left); });
  positionLeft_->setCheckable(true);
  positionRight_ = position->addAction(QString("&Right"), this,
      [this]() { updatePosition(PanelPosition::Right); });
  positionRight_->setCheckable(true);

  const int numScreens = QGuiApplication::screens().size();
  if (numScreens > 1) {
    QMenu* screen = menu_.addMenu(QString("Scr&een"));
    for (int i = 0; i < numScreens; ++i) {
      QAction* action = screen->addAction(
          "Screen " + QString::number(i + 1), this,
          [this, i]() {
            setScreen(i);
            reload();
            saveDockConfig();
          });
      action->setCheckable(true);
      screenActions_.push_back(action);
    }
  }

  QMenu* visibility = menu_.addMenu(QString("&Visibility"));
  visibilityAlwaysVisibleAction_ = visibility->addAction(
      QString("Always &Visible"), this,
      [this]() { updateVisibility(PanelVisibility::AlwaysVisible); });
  visibilityAlwaysVisibleAction_->setCheckable(true);
  visibilityAutoHideAction_ = visibility->addAction(
      QString("Auto &Hide"), this,
      [this]() { updateVisibility(PanelVisibility::AutoHide); });
  visibilityAutoHideAction_->setCheckable(true);
  visibilityWindowsCanCoverAction_ = visibility->addAction(
      QString("Windows Can &Cover"), this,
      [this]() { updateVisibility(PanelVisibility::WindowsCanCover); });
  visibilityWindowsCanCoverAction_->setCheckable(true);
  visibilityWindowsCanCoverQuietAction_ = visibility->addAction(
      QString("Windows Can Cover (&Quiet)"), this,
      [this]() { updateVisibility(PanelVisibility::WindowsCanCover_Quiet); });
  visibilityWindowsCanCoverQuietAction_->setCheckable(true);
  visibilityWindowsGoBelowAction_ = visibility->addAction(
      QString("Windows Go &Below"), this,
      [this]() { updateVisibility(PanelVisibility::WindowsGoBelow); });
  visibilityWindowsGoBelowAction_->setCheckable(true);

  QMenu* extraComponents = menu_.addMenu(QString("&Optional Features"));
  applicationMenuAction_ = extraComponents->addAction(QString("Application Menu"), this,
      SLOT(toggleApplicationMenu()));
  applicationMenuAction_->setCheckable(true);
  pagerAction_ = extraComponents->addAction(QString("Pager"), this,
      SLOT(togglePager()));
  pagerAction_->setCheckable(true);
  taskManagerAction_ = extraComponents->addAction(QString("Task Manager"), this,
      SLOT(toggleTaskManager()));
  taskManagerAction_->setCheckable(true);
  clockAction_ = extraComponents->addAction(QString("Clock"), this,
      SLOT(toggleClock()));
  clockAction_->setCheckable(true);

  menu_.addSeparator();
  menu_.addAction(QIcon::fromTheme("help-contents"),
                  QString("Online &Documentation"),
                  this, SLOT(showOnlineDocumentation()));
  menu_.addAction(QIcon::fromTheme("help-about"), QString("A&bout Crystal Dock"),
      this, SLOT(about()));
  menu_.addSeparator();
  menu_.addAction(QString("E&xit"), parent_, SLOT(exit()));
}

void DockPanel::setPosition(PanelPosition position) {
  position_ = position;
  orientation_ = (position_ == PanelPosition::Top ||
      position_ == PanelPosition::Bottom)
      ? Qt::Horizontal : Qt::Vertical;
  positionTop_->setChecked(position == PanelPosition::Top);
  positionBottom_->setChecked(position == PanelPosition::Bottom);
  positionLeft_->setChecked(position == PanelPosition::Left);
  positionRight_->setChecked(position == PanelPosition::Right);
}

void DockPanel::setVisibility(PanelVisibility visibility) {
  visibility_ = visibility;
  switch(visibility_) {
    case PanelVisibility::AlwaysVisible:  // fall through
    case PanelVisibility::AutoHide:  // fall through
    case PanelVisibility::WindowsGoBelow:
      //WindowSystem::keepAbove(winId());
      break;
    case PanelVisibility::WindowsCanCover:  // fall through
    case PanelVisibility::WindowsCanCover_Quiet:
      //WindowSystem::keepBelow(winId());
      break;
    default:
      break;
  }

  visibilityAlwaysVisibleAction_->setChecked(
      visibility_ == PanelVisibility::AlwaysVisible);
  visibilityAutoHideAction_->setChecked(
      visibility_ == PanelVisibility::AutoHide);
  visibilityWindowsCanCoverAction_->setChecked(
      visibility_ == PanelVisibility::WindowsCanCover);
  visibilityWindowsCanCoverQuietAction_->setChecked(
      visibility_ == PanelVisibility::WindowsCanCover_Quiet);
  visibilityWindowsGoBelowAction_->setChecked(
      visibility_ == PanelVisibility::WindowsGoBelow);
}

void DockPanel::loadDockConfig() {
  setPosition(model_->panelPosition(dockId_));
  setScreen(model_->screen(dockId_));
  setVisibility(model_->visibility(dockId_));

  showApplicationMenu_ = model_->showApplicationMenu(dockId_);
  applicationMenuAction_->setChecked(showApplicationMenu_);

  showPager_ = model_->showPager(dockId_);
  pagerAction_->setChecked(showPager_);

  taskManagerAction_->setChecked(model_->showTaskManager(dockId_));

  showClock_ = model_->showClock(dockId_);
  clockAction_->setChecked(showClock_);
}

void DockPanel::saveDockConfig() {
  model_->setPanelPosition(dockId_, position_);
  model_->setScreen(dockId_, screen_);
  model_->setVisibility(dockId_, visibility_);
  model_->setShowApplicationMenu(dockId_, showApplicationMenu_);
  model_->setShowPager(dockId_, showPager_);
  model_->setShowTaskManager(dockId_, taskManagerAction_->isChecked());
  model_->setShowClock(dockId_, showClock_);
  model_->saveDockConfig(dockId_);
}

void DockPanel::loadAppearanceConfig() {
  minSize_ = model_->minIconSize();
  maxSize_ = model_->maxIconSize();
  spacingFactor_ = model_->spacingFactor();
  backgroundColor_ = model_->backgroundColor();
  showBorder_ = model_->showBorder();
  borderColor_ = model_->borderColor();
  tooltipFontSize_ = model_->tooltipFontSize();
}

void DockPanel::initApplicationMenu() {
  if (showApplicationMenu_) {
    items_.push_back(std::make_unique<ApplicationMenu>(
        this, model_, orientation_, minSize_, maxSize_));
  }
}

void DockPanel::initLaunchers() {
  for (const auto& launcherConfig : model_->dockLauncherConfigs(dockId_)) {
    if (launcherConfig.command == kSeparatorCommand) {
      items_.push_back(std::make_unique<Separator>(this, model_, orientation_, minSize_, maxSize_));
    } else {
      items_.push_back(std::make_unique<Program>(
          this, model_, launcherConfig.appId, launcherConfig.name, orientation_,
          launcherConfig.icon, minSize_, maxSize_, launcherConfig.command,
          model_->isAppMenuEntry(launcherConfig.command), /*pinned=*/true));
    }
  }
}

void DockPanel::initPager() {
  if (showPager_) {
    for (const auto& desktop : WindowSystem::desktops()) {
      items_.push_back(std::make_unique<DesktopSelector>(
          this, model_, orientation_, minSize_, maxSize_, desktop, screen_));
    }
  }
}

void DockPanel::initTasks() {
  if (!showTaskManager()) {
    return;
  }

  for (const auto* task : WindowSystem::windows()) {
    if (isValidTask(task, model_)) {
      addTask(task);
    }
  }
}

void DockPanel::reloadTasks() {
  if (!showTaskManager()) {
    return;
  }

  const int itemsToKeep = applicationMenuItemCount() + pagerItemCount();
  items_.resize(itemsToKeep);
  initLaunchers();
  initTasks();
  initClock();
  resizeTaskManager();
}

void DockPanel::addTask(const WindowInfo* task) {
  // Checks is the task already exists.
  for (auto& item : items_) {
    if (item->hasTask(task->uuid)) {
      return;
    }
  }

  // Tries adding the task to existing programs.
  for (auto& item : items_) {
    if (item->addTask(task)) {
      return;
    }
  }

  // Adds a new program.
  const QString appId = QString::fromStdString(task->appId);
  auto app = model_->findApplication(task->appId);
  const QString program = app ? app->name : QString::fromStdString(task->appId);
  int i = 0;
  for (; i < itemCount() && items_[i]->beforeTask(program); ++i);
  if (app && QIcon::hasThemeIcon(app->icon)) {
    items_.insert(items_.begin() + i, std::make_unique<Program>(
        this, model_, appId, program, orientation_, app->icon, minSize_,
        maxSize_, app->command, /*isAppMenuEntry=*/true, /*pinned=*/false));
  } else {
    items_.insert(items_.begin() + i, std::make_unique<Program>(
        this, model_, appId, program, orientation_, QPixmap(), minSize_, maxSize_));
  }
  items_[i]->addTask(task);
}

void DockPanel::removeTask(std::string_view uuid) {
  for (int i = 0; i < itemCount(); ++i) {
    if (items_[i]->removeTask(uuid)) {
      if (items_[i]->shouldBeRemoved()) {
        items_.erase(items_.begin() + i);
        resizeTaskManager();
      }
      return;
    }
  }
}

void DockPanel::updateTask(const WindowInfo* task) {
  for (auto& item : items_) {
    if (item->updateTask(task)) {
      return;
    }
  }
}

void DockPanel::initClock() {
  if (showClock_) {
    items_.push_back(std::make_unique<Clock>(
        this, model_, orientation_, minSize_, maxSize_));
  }
}

void DockPanel::initLayoutVars() {
  itemSpacing_ = static_cast<int>(minSize_ * spacingFactor_);
  parabolicMaxX_ = static_cast<int>(2.5 * (minSize_ + itemSpacing_));
  numAnimationSteps_ = 20;
  animationSpeed_ = 16;

  tooltipSize_ = 50;

  const int distance = minSize_ + itemSpacing_;
  // The difference between minWidth_ and maxWidth_
  // (horizontal mode) or between minHeight_ and
  // maxHeight_ (vertical mode).
  int delta = 0;
  if (itemCount() >= 5) {
    delta = parabolic(0) + 2 * parabolic(distance) +
        2 * parabolic(2 * distance) - 5 * minSize_;
  } else if (itemCount() == 4) {
    delta = parabolic(0) + 2 * parabolic(distance) +
        parabolic(2 * distance) - 4 * minSize_;
  } else if (itemCount() == 3) {
    delta = parabolic(0) + 2 * parabolic(distance) - 3 * minSize_;
  } else if (itemCount() == 2) {
    delta = parabolic(0) + parabolic(distance) - 2 * minSize_;
  } else if (itemCount() == 1) {
    delta = parabolic(0) - minSize_;
  }

  if (orientation_ == Qt::Horizontal) {
    minWidth_ = 0;
    for (const auto& item : items_) {
      minWidth_ += (item->getMinWidth() + itemSpacing_);
    }
    minHeight_ = autoHide() ? kAutoHideSize : distance;
    maxWidth_ = minWidth_ + delta;
    maxHeight_ = itemSpacing_ + maxSize_ + tooltipSize_;
  } else {  // Vertical
    minHeight_ = 0;
    for (const auto& item : items_) {
      minHeight_ += (item->getMinHeight() + itemSpacing_);
    }
    minWidth_ = autoHide() ? kAutoHideSize : distance;
    maxHeight_ = minHeight_ + delta;
    maxWidth_ = itemSpacing_ + maxSize_ + tooltipSize_;
  }
}

void DockPanel::updateLayout() {
  const int distance = minSize_ + itemSpacing_;
  if (isLeaving_) {
    for (const auto& item : items_) {
      item->setAnimationStartAsCurrent();
      if (isHorizontal()) {
        startBackgroundWidth_ = backgroundWidth_;
        startBackgroundHeight_ = distance;
      } else {  // Vertical
        startBackgroundHeight_ = backgroundHeight_;
        startBackgroundWidth_ = distance;
      }
    }
  }

  for (int i = 0; i < itemCount(); ++i) {
    items_[i]->size_ = minSize_;
    if (isHorizontal()) {
      items_[i]->left_ = (i == 0) ? itemSpacing_ / 2
          : items_[i - 1]->left_ + items_[i - 1]->getMinWidth() + itemSpacing_;
      items_[i]->top_ = itemSpacing_ / 2;
      items_[i]->minCenter_ = items_[i]->left_ + items_[i]->getMinWidth() / 2;
    } else {  // Vertical
      items_[i]->left_ = itemSpacing_ / 2;
      items_[i]->top_ = (i == 0) ? itemSpacing_ / 2
          : items_[i - 1]->top_ + items_[i - 1]->getMinHeight() + itemSpacing_;
      items_[i]->minCenter_ = items_[i]->top_ + items_[i]->getMinHeight() / 2;
    }
  }
  if (isHorizontal()) {
    backgroundWidth_ = minWidth_;
    backgroundHeight_ = distance;
  } else {  // Vertical
    backgroundHeight_ = minHeight_;
    backgroundWidth_ = distance;
  }

  if (isLeaving_) {
    for (const auto& item : items_) {
      item->endSize_ = item->size_;
      if (isHorizontal()) {
        item->endLeft_ = item->left_ + (screenGeometry_.width() - minWidth_) / 2
            - x() + screenGeometry_.x();
        if (position_ == PanelPosition::Top) {
          item->endTop_ = item->top_ + minHeight_ - distance;
        } else {  // Bottom
          item->endTop_ = item->top_ + (maxHeight_ - minHeight_);
        }
      } else {  // Vertical
        item->endTop_ = item->top_ + (screenGeometry_.height() - minHeight_) / 2
            - y() + screenGeometry_.y();
        if (position_ == PanelPosition::Left) {
          item->endLeft_ = item->left_ + minWidth_ - distance;
        } else {  // Right
          item->endLeft_ = item->left_ + (maxWidth_ - minWidth_);
        }
      }
      item->startAnimation(numAnimationSteps_);
    }
    if (isHorizontal()) {
      endBackgroundWidth_ = minWidth_;
      backgroundWidth_ = startBackgroundWidth_;
      endBackgroundHeight_ = autoHide() ? kAutoHideSize : distance;
      backgroundHeight_ = startBackgroundHeight_;
    } else {  // Vertical
      endBackgroundHeight_ = minHeight_;
      backgroundHeight_ = startBackgroundHeight_;
      endBackgroundWidth_ = autoHide() ? kAutoHideSize : distance;
      backgroundWidth_ = startBackgroundWidth_;
    }
    currentAnimationStep_ = 0;
    isAnimationActive_ = true;
    animationTimer_->start(32 - animationSpeed_);
  } else {
    isMinimized_ = true;
    resize(minWidth_, minHeight_);
    update();
  }
}

void DockPanel::updateLayout(int x, int y) {
  const int distance = minSize_ + itemSpacing_;
  if (isEntering_) {
    for (const auto& item : items_) {
      item->startSize_ = item->size_;
      if (isHorizontal()) {
        item->startLeft_ = item->left_ + (maxWidth_ - minWidth_) / 2;
        if (position_ == PanelPosition::Top) {
          item->startTop_ = item->top_ + minHeight_ - distance;
        } else {  // Bottom
          item->startTop_ = item->top_ + (maxHeight_ - minHeight_);
        }
      } else {  // Vertical
        item->startTop_ = item->top_ + (maxHeight_ - minHeight_) / 2;
        if (position_ == PanelPosition::Left) {
          item->startLeft_ = item->left_ + minWidth_ - distance;
        } else {  // Right
          item->startLeft_ = item->left_ + (maxWidth_ - minWidth_);
        }
      }
    }
    if (isHorizontal()) {
      startBackgroundWidth_ = minWidth_;
      startBackgroundHeight_ = autoHide() ? kAutoHideSize : distance;
    } else {  // Vertical
      startBackgroundHeight_ = minHeight_;
      startBackgroundWidth_ = autoHide() ? kAutoHideSize : distance;
    }
  }

  int first_update_index = -1;
  int last_update_index = 0;
  if (isHorizontal()) {
    items_[0]->left_ = itemSpacing_ / 2;
  } else {  // Vertical
    items_[0]->top_ = itemSpacing_ / 2;
  }
  for (int i = 0; i < itemCount(); ++i) {
    int delta;
    if (isHorizontal()) {
      delta = std::abs(items_[i]->minCenter_ - x + (width() - minWidth_) / 2);
    } else {  // Vertical
      delta = std::abs(items_[i]->minCenter_ - y + (height() - minHeight_) / 2);
    }
    if (delta < parabolicMaxX_) {
      if (first_update_index == -1) {
        first_update_index = i;
      }
      last_update_index = i;
    }
    items_[i]->size_ = parabolic(delta);
    if (position_ == PanelPosition::Top) {
      items_[i]->top_ = itemSpacing_ / 2;
    } else if (position_ == PanelPosition::Bottom) {
      items_[i]->top_ = itemSpacing_ / 2 + tooltipSize_ + maxSize_ - items_[i]->getHeight();
    } else if (position_ == PanelPosition::Left) {
      items_[i]->left_ = itemSpacing_ / 2;
    } else {  // Right
      items_[i]->left_ = itemSpacing_ / 2 + maxSize_ - items_[i]->getWidth();
    }
    if (i > 0) {
      if (isHorizontal()) {
        items_[i]->left_ = items_[i - 1]->left_ + items_[i - 1]->getWidth()
            + itemSpacing_;
      } else {  // Vertical
        items_[i]->top_ = items_[i - 1]->top_ + items_[i - 1]->getHeight()
            + itemSpacing_;
      }
    }
  }
  for (int i = itemCount() - 1; i >= last_update_index + 1; --i) {
    if (isHorizontal()) {
      items_[i]->left_ = (i == itemCount() - 1)
          ? maxWidth_ - itemSpacing_ / 2 - items_[i]->getMinWidth()
          : items_[i + 1]->left_ - items_[i]->getMinWidth() - itemSpacing_;
    } else {  // Vertical
      items_[i]->top_ = (i == itemCount() - 1)
          ? maxHeight_ - itemSpacing_ / 2 - items_[i]->getMinHeight()
          : items_[i + 1]->top_ - items_[i]->getMinHeight() - itemSpacing_;
    }
  }
  if (first_update_index == 0 && last_update_index < itemCount() - 1) {
    for (int i = last_update_index; i >= first_update_index; --i) {
      if (isHorizontal()) {
        items_[i]->left_ = items_[i + 1]->left_ - items_[i]->getWidth()
            - itemSpacing_;
      } else {  // Vertical
        items_[i]->top_ = items_[i + 1]->top_ - items_[i]->getHeight()
            - itemSpacing_;
      }
    }
  }

  if (isEntering_) {
    for (const auto& item : items_) {
      item->setAnimationEndAsCurrent();
      item->startAnimation(numAnimationSteps_);
    }
    if (isHorizontal()) {
      endBackgroundWidth_ = maxWidth_;
      backgroundWidth_ = startBackgroundWidth_;
      endBackgroundHeight_ = distance;
      backgroundHeight_ = startBackgroundHeight_;
      mouseX_ = x + (maxWidth_ - minWidth_) / 2;
    } else {  // Vertical
      endBackgroundHeight_ = maxHeight_;
      backgroundHeight_ = startBackgroundHeight_;
      endBackgroundWidth_ = distance;
      backgroundWidth_ = startBackgroundWidth_;
      mouseY_ = y + (maxHeight_ - minHeight_) / 2;
    }

    currentAnimationStep_ = 0;
    isAnimationActive_ = true;
    isEntering_ = false;
    animationTimer_->start(32 - animationSpeed_);
  } else {
    mouseX_ = x;
    mouseY_ = y;
  }

  resize(maxWidth_, maxHeight_);
  isMinimized_ = false;
  update();
}

void DockPanel::resizeTaskManager() {
  // Re-calculate panel's size.
  initLayoutVars();

  if (isMinimized_) {
    updateLayout();
    return;
  } else {
    // Need to call QWidget::resize(), not DockPanel::resize(), in order not to
    // mess up the zooming.
    QWidget::resize(maxWidth_, maxHeight_);
    if (isHorizontal()) {
      backgroundWidth_ = maxWidth_;
    } else {
      backgroundHeight_ = maxHeight_;
    }
  }

  const int itemsToKeep = (showApplicationMenu_ ? 1 : 0) +
      (showPager_ ? WindowSystem::numberOfDesktops() : 0);
  int left = 0;
  int top = 0;
  for (int i = 0; i < itemCount(); ++i) {
    if (isHorizontal()) {
      left = (i == 0) ? itemSpacing_ / 2
                      : left + items_[i - 1]->getMinWidth() + itemSpacing_;
      if (i >= itemsToKeep) {
        items_[i]->minCenter_ = left + items_[i]->getMinWidth() / 2;
      }
    } else {  // Vertical
      top = (i == 0) ? itemSpacing_ / 2
                     : top + items_[i - 1]->getMinHeight() + itemSpacing_;
      if (i >= itemsToKeep) {
        items_[i]->minCenter_ = top + items_[i]->getMinHeight() / 2;
      }
    }
  }

  int last_update_index = 0;
  for (int i = itemsToKeep; i < itemCount(); ++i) {
    int delta;
    if (isHorizontal()) {
      delta = std::abs(items_[i]->minCenter_ - mouseX_ +
                       (width() - minWidth_) / 2);
    } else {  // Vertical
      delta = std::abs(items_[i]->minCenter_ - mouseY_ +
                       (height() - minHeight_) / 2);
    }
    if (delta < parabolicMaxX_) {
      last_update_index = i;
    }
    items_[i]->size_ = parabolic(delta);
    if (position_ == PanelPosition::Top) {
      items_[i]->top_ = itemSpacing_ / 2;
    } else if (position_ == PanelPosition::Bottom) {
      items_[i]->top_ = itemSpacing_ / 2 + maxSize_ - items_[i]->getHeight();
    } else if (position_ == PanelPosition::Left) {
      items_[i]->left_ = itemSpacing_ / 2;
    } else {  // Right
      items_[i]->left_ = itemSpacing_ / 2 + maxSize_ - items_[i]->getWidth();
    }
    if (i > 0) {
      if (isHorizontal()) {
        items_[i]->left_ = items_[i - 1]->left_ + items_[i - 1]->getWidth()
            + itemSpacing_;
      } else {  // Vertical
        items_[i]->top_ = items_[i - 1]->top_ + items_[i - 1]->getHeight()
            + itemSpacing_;
      }
    }
  }

  for (int i = itemCount() - 1;
       i >= std::max(itemsToKeep, last_update_index + 1); --i) {
    if (isHorizontal()) {
      items_[i]->left_ = (i == itemCount() - 1)
          ? maxWidth_ - itemSpacing_ / 2 - items_[i]->getMinWidth()
          : items_[i + 1]->left_ - items_[i]->getMinWidth() - itemSpacing_;
    } else {  // Vertical
      items_[i]->top_ = (i == itemCount() - 1)
          ? maxHeight_ - itemSpacing_ / 2 - items_[i]->getMinHeight()
          : items_[i + 1]->top_ - items_[i]->getMinHeight() - itemSpacing_;
    }
  }

  update();
}

void DockPanel::setStrut(int width) {
  return;
}

int DockPanel::findActiveItem(int x, int y) {
  int i = 0;
  while (i < itemCount() &&
      ((orientation_ == Qt::Horizontal && items_[i]->left_ < x) ||
      (orientation_ == Qt::Vertical && items_[i]->top_ < y))) {
    ++i;
  }
  return i - 1;
}

void DockPanel::showTooltip(int x, int y) {
  activeItem_ = findActiveItem(x, y);
  update();
}

void DockPanel::showTooltip(int i) {
}

void DockPanel::showWaitCursor() {
  setCursor(QCursor(Qt::WaitCursor));
  QTimer::singleShot(1000 /* msecs */, this, SLOT(resetCursor()));
}

int DockPanel::parabolic(int x) {
  // Assume x >= 0.
  if (x > parabolicMaxX_) {
    return minSize_;
  } else {
    return maxSize_ -
        (x * x * (maxSize_ - minSize_)) / (parabolicMaxX_ * parabolicMaxX_);
  }
}

}  // namespace crystaldock
