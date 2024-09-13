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
#include <QFont>
#include <QFontMetrics>
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

#include <LayerShellQt/Window>

#include "add_panel_dialog.h"
#include "application_menu.h"
#include "clock.h"
#include "desktop_selector.h"
#include "multi_dock_view.h"
#include "program.h"
#include "separator.h"
#include <display/window_system.h>
#include <utils/draw_utils.h>

namespace crystaldock {

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
                   QString("<h3>Crystal Dock 2.3 RC1</h3>")
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

  connect(animationTimer_.get(), SIGNAL(timeout()), this,
      SLOT(updateAnimation()));
  connect(WindowSystem::self(), SIGNAL(numberOfDesktopsChanged(int)),
      this, SLOT(updatePager()));
  connect(WindowSystem::self(), SIGNAL(currentDesktopChanged(std::string_view)),
          this, SLOT(onCurrentDesktopChanged()));
  connect(WindowSystem::self(), SIGNAL(windowStateChanged(std::string_view)),
          this, SLOT(update()));
  connect(WindowSystem::self(), SIGNAL(windowAdded(const WindowInfo*)),
          this, SLOT(onWindowAdded(const WindowInfo*)));
  connect(WindowSystem::self(), SIGNAL(windowRemoved(std::string)),
          this, SLOT(onWindowRemoved(std::string)));
  connect(WindowSystem::self(), SIGNAL(windowLeftCurrentDesktop(std::string_view)),
          this, SLOT(onWindowLeftCurrentDesktop(std::string_view)));
  connect(WindowSystem::self(), SIGNAL(windowLeftCurrentActivity(std::string_view)),
          this, SLOT(onWindowLeftCurrentActivity(std::string_view)));
  connect(WindowSystem::self(), SIGNAL(windowGeometryChanged(const WindowInfo*)),
          this, SLOT(onWindowGeometryChanged(const WindowInfo*)));
  connect(WindowSystem::self(), SIGNAL(currentActivityChanged(std::string_view)),
          this, SLOT(onCurrentActivityChanged()));
  connect(model_, SIGNAL(appearanceOutdated()), this, SLOT(update()));
  connect(model_, SIGNAL(appearanceChanged()), this, SLOT(reload()));
  connect(model_, SIGNAL(dockLaunchersChanged(int)),
          this, SLOT(onDockLaunchersChanged(int)));
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

void DockPanel::onCurrentActivityChanged() {
  reloadTasks();
}

void DockPanel::setStrut() {
  int strut = 0;
  switch(visibility_) {
    case PanelVisibility::AlwaysVisible:
      strut = isHorizontal() ? minHeight_ : minWidth_;
      if (isFloating()) { strut += 2 * floatingMargin_; }
      setStrut(strut);
      break;
    case PanelVisibility::AutoHide:
      setStrut(1);
      break;
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
  screenGeometry_ = WindowSystem::screens()[screen]->geometry();
  WindowSystem::setScreen(this, screen);
}

void DockPanel::changeScreen(int screen) {
  if (screen_ == screen) {
    return;
  }
  model_->cloneDock(dockId_, position_, screen);
  deleteLater();
  model_->removeDock(dockId_);
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
  if (!showTaskManager()) {
    return;
  }

  if (isValidTask(info)) {
    if (addTask(info)) {
      resizeTaskManager();
    } else {
      update();
    }
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

void DockPanel::onWindowLeftCurrentActivity(std::string_view uuid) {
  if (showTaskManager()) {
    removeTask(uuid);
  }
}

void DockPanel::onWindowGeometryChanged(const WindowInfo* task) {
  QRect windowGeometry(task->x, task->y, task->width, task->height);
  if (hasTask(task->uuid)) {
    if (!windowGeometry.intersects(screenGeometry_)) {
      removeTask(task->uuid);
    }
  } else {
    if (windowGeometry.intersects(screenGeometry_) && isValidTask(task)) {
      addTask(task);
    }
  }
}

int DockPanel::taskIndicatorPos() {
  if (isHorizontal()) {
    int y = isTop()
        ? isFloating() ? floatingMargin_ : 0
        : isFloating() ? maxHeight_ - floatingMargin_ - k3DPanelThickness
                       : maxHeight_ - k3DPanelThickness;
    if (isBottom() && is3D()) { y -= 2; }
    return y;
  } else {
    return isLeft()
        ? isFloating() ? floatingMargin_ : 0
                       : isFloating() ? maxWidth_ - k3DPanelThickness - floatingMargin_
                                      : maxWidth_ - k3DPanelThickness;
  }
}

void DockPanel::paintEvent(QPaintEvent* e) {
  QPainter painter(this);

  if (visibility_ == PanelVisibility::AutoHide && isMinimized_) {
    painter.setPen(borderColor_);
    if (isHorizontal()) {
      const int y = isTop() ? 0 : maxHeight_ - 1;
      painter.drawLine((maxWidth_ - minWidth_) / 2, y, (maxWidth_ + minWidth_) / 2, y);
    } else {  // Vertical.
      const int x = isLeft() ? 0 : maxWidth_ - 1;
      painter.drawLine(x, (maxHeight_ - minHeight_) / 2, x, (maxHeight_ + minHeight_) / 2);
    }
    return;
  }

  if (isHorizontal()) {
    int y = isTop()
        ? isFloating() ? floatingMargin_ : 0
        : isFloating() ? maxHeight_ - backgroundHeight_ - floatingMargin_
                       : maxHeight_ - backgroundHeight_;
    if (is3D() && isBottom()) {  // 3D styles only apply to bottom dock.
      y -= k3DPanelThickness;
      draw3dDockPanel(
          (maxWidth_ - backgroundWidth_) / 2, y, backgroundWidth_ - 1, backgroundHeight_ - 1,
           backgroundHeight_ / 16, borderColor_, backgroundColor_, &painter);
    } else {
      fillRoundedRect(
          (maxWidth_ - backgroundWidth_) / 2, y, backgroundWidth_ - 1, backgroundHeight_ - 1,
           backgroundHeight_ / 16, showBorder_, borderColor_, backgroundColor_, &painter);
    }
  } else {  // Vertical
    const int x =  isLeft()
        ? isFloating() ? floatingMargin_ : 0
        : isFloating() ? maxWidth_ - backgroundWidth_ - floatingMargin_
                       : maxWidth_ - backgroundWidth_;
    fillRoundedRect(x, (maxHeight_ - backgroundHeight_) / 2, backgroundWidth_ - 1, backgroundHeight_ - 1,
                    backgroundWidth_ / 16, showBorder_, borderColor_, backgroundColor_, &painter);
  }

  if (isBottom() && is3D()) {
    QImage mainImage(width(), height(), QImage::Format_ARGB32);
    mainImage.fill(0);
    QPainter mainPainter(&mainImage);
    // Draw the items from the end to avoid zoomed items getting clipped by
    // non-zoomed items.
    for (int i = itemCount() - 1; i >= 0; --i) {
      items_[i]->draw(&mainPainter);
    }
    painter.drawImage(0, 0, mainImage);

    int y = height() - itemSpacing_ - k3DPanelThickness;
    if (isFloating()) { y -= floatingMargin_; }
    QImage toMirrorImage = mainImage.copy(0, y - itemSpacing_ + 2, width(), itemSpacing_ - 2);
    QImage mirrorImage = toMirrorImage.mirrored();
    painter.setOpacity(0.3);
    painter.drawImage(0, y, mirrorImage);
    painter.setOpacity(1.0);
  } else {
    // Draw the items from the end to avoid zoomed items getting clipped by
    // non-zoomed items.
    for (int i = itemCount() - 1; i >= 0; --i) {
      items_[i]->draw(&painter);
    }
  }

  // Draw tooltip.
  if (!isAnimationActive_ && activeItem_ >= 0 &&
      activeItem_ < static_cast<int>(items_.size())) {
    if (isHorizontal()) {
      const auto& item = items_[activeItem_];
      QFont font;
      font.setPointSize(model_->tooltipFontSize());
      font.setBold(true);
      QFontMetrics metrics(font);
      const auto tooltipWidth = metrics.boundingRect(item->getLabel()).width();
      painter.setFont(font);
      int x = item->left_ + item->getWidth() / 2 - tooltipWidth / 2;
      x = std::min(x, maxWidth_ - tooltipWidth);
      x = std::max(x, 0);
      int y = isTop()
          ? isFloating() ? maxHeight_ - itemSpacing_ + floatingMargin_
                         : maxHeight_ - itemSpacing_
          : isFloating() ? itemSpacing_ + tooltipSize_ / 2 - floatingMargin_
                         : itemSpacing_ + tooltipSize_ / 2;
      drawBorderedText(x, y, item->getLabel(), /*borderWidth*/ 2, Qt::black, Qt::white, &painter);
    } else {  // Vertical
      // Do not draw tooltip for Vertical positions for now because the total
      // area of the dock would take too much desktop space.
    }
  }
}

void DockPanel::mouseMoveEvent(QMouseEvent* e) {
  if (autoHide() && QDateTime::currentMSecsSinceEpoch() - enterTime_
      < model_->autoHideActivationDelay()) {
    lastMouseX_ = e->position().x();
    lastMouseY_ = e->position().y();
    return;
  }

  const auto x = e->position().x();
  const auto y = e->position().y();
  if (isEntering_) {
    // Don't do the parabolic zooming if the mouse is outside the minimized area.
    // Also don't do the parabolic zooming if the mouse is near the border.
    // Quite often the user was just scrolling a window etc.

    if ((position_ == PanelPosition::Bottom &&
         ((!autoHide() && y < itemSpacing_ + maxHeight_ - minHeight_) ||
          (autoHide() && y < maxHeight_ - 2))) ||
        (position_ == PanelPosition::Top &&
         ((!autoHide() && y > minHeight_ - itemSpacing_) ||
          (autoHide() && y > 2))) ||
        (position_ == PanelPosition::Left &&
         ((!autoHide() && x > minWidth_ - itemSpacing_) ||
          (autoHide() && x > 2))) ||
        (position_ == PanelPosition::Right &&
         ((!autoHide() && x < itemSpacing_ + maxWidth_ - minWidth_) ||
          (autoHide() && x < maxWidth_ - 2)))) {
      return;
    }

    if (isHorizontal() &&
        (x < (maxWidth_ - minWidth_) / 2 || x > (maxWidth_ + minWidth_) / 2)) {
      return;
    }
    if (!isHorizontal() &&
        (y < (maxHeight_ - minHeight_) / 2 || y > (maxHeight_ + minHeight_) / 2)) {
      return;
    }
  }

  if (isAnimationActive_) {
    return;
  }

  updateLayout(x, y);
}

void DockPanel::mousePressEvent(QMouseEvent* e) {
  if (isAnimationActive_) {
    return;
  }

  if (activeItem_ >= 0 && activeItem_ < static_cast<int>(items_.size())) {
    items_[activeItem_]->maybeResetActiveWindow(e);
    items_[activeItem_]->mousePressEvent(e);
  }
}

void DockPanel::enterEvent (QEnterEvent* e) {
  if (autoHide()) {
    enterTime_ = QDateTime::currentMSecsSinceEpoch();
    QTimer::singleShot(model_->autoHideActivationDelay(), [this] {
      if (enterTime_ > 0) {
        isEntering_ = true;
        updateLayout(lastMouseX_, lastMouseY_);
      }
    });
    return;
  }

  isEntering_ = true;
}

void DockPanel::leaveEvent(QEvent* e) {
  if (autoHide()) {
    enterTime_ = 0;
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

  floatingStyleAction_ = menu_.addAction(
      QString("Style: Floating"), this,
      [this] {
        changeFloatingStyle();
      });
  floatingStyleAction_->setCheckable(true);
  floatingStyleAction_->setChecked(isFloating());

  style3DAction_ = menu_.addAction(
      QString("Style: 3D (bottom dock only)"), this,
      [this] {
        change3DStyle();
      });
  style3DAction_->setCheckable(true);
  style3DAction_->setChecked(is3D());

  menu_.addAction(QIcon::fromTheme("help-contents"),
                  QString("Online &Documentation"),
                  this, SLOT(showOnlineDocumentation()));
  menu_.addAction(QIcon::fromTheme("help-about"), QString("A&bout Crystal Dock"),
      this, SLOT(about()));
  menu_.addSeparator();

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

  const int numScreens = WindowSystem::screens().size();
  if (numScreens > 1) {
    QMenu* screen = menu_.addMenu(QString("Scr&een"));
    for (int i = 0; i < numScreens; ++i) {
      QAction* action = screen->addAction(
          "Screen " + QString::number(i + 1), this,
          [this, i]() {
            changeScreen(i);
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
  visibilityAlwaysOnTopAction_ = visibility->addAction(
      QString("Always On &Top"), this,
      [this]() { updateVisibility(PanelVisibility::AlwaysOnTop); });
  visibilityAlwaysOnTopAction_->setCheckable(true);

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
  visibilityAlwaysVisibleAction_->setChecked(
      visibility_ == PanelVisibility::AlwaysVisible);
  visibilityAutoHideAction_->setChecked(
      visibility_ == PanelVisibility::AutoHide);
  visibilityAlwaysOnTopAction_->setChecked(
      visibility_ == PanelVisibility::AlwaysOnTop);
}

void DockPanel::setPanelStyle(PanelStyle panelStyle) {
  panelStyle_ = panelStyle;
  floatingStyleAction_->setChecked(isFloating());
  style3DAction_->setChecked(is3D());
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
  setPanelStyle(model_->panelStyle());
}

void DockPanel::initApplicationMenu() {
  if (showApplicationMenu_) {
    items_.push_back(std::make_unique<ApplicationMenu>(
        this, model_, orientation_, minSize_, maxSize_));
  }
}

void DockPanel::initLaunchers() {
  for (const auto& launcherConfig : model_->launcherConfigs(dockId_)) {
    if (launcherConfig.appId == kSeparatorId) {
      items_.push_back(std::make_unique<Separator>(this, model_, orientation_, minSize_, maxSize_));
    } else {
      items_.push_back(std::make_unique<Program>(
          this, model_, launcherConfig.appId, launcherConfig.name, orientation_,
          launcherConfig.icon, minSize_, maxSize_, launcherConfig.command,
          model_->isAppMenuEntry(launcherConfig.appId.toStdString()), /*pinned=*/true));
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
    if (isValidTask(task)) {
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

bool DockPanel::addTask(const WindowInfo* task) {
  // Checks is the task already exists.
  if (hasTask(task->uuid)) {
    return false;
  }

  // Tries adding the task to existing programs.
  for (auto& item : items_) {
    if (item->addTask(task)) {
      return false;
    }
  }

  // Adds a new program.
  const QString appId = QString::fromStdString(task->appId);
  auto app = model_->findApplication(task->appId);
  const QString label = app ? app->name : QString::fromStdString(task->title);
  int i = 0;
  for (; i < itemCount() && items_[i]->beforeTask(label); ++i);
  if (app && QIcon::hasThemeIcon(app->icon)) {
    items_.insert(items_.begin() + i, std::make_unique<Program>(
        this, model_, appId, label, orientation_, app->icon, minSize_,
        maxSize_, app->command, /*isAppMenuEntry=*/true, /*pinned=*/false));
  } else {
    items_.insert(items_.begin() + i, std::make_unique<Program>(
        this, model_, appId, label, orientation_, QPixmap(), minSize_, maxSize_));
  }
  items_[i]->addTask(task);

  return true;
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

bool DockPanel::isValidTask(const WindowInfo* task) {
  if (task == nullptr) {
    return false;
  }

  if (task->appId == "crystal-dock") {
    return false;
  }

  if (task->skipTaskbar) {
    return false;
  }

  if (!task->onAllDesktops && model_->currentDesktopTasksOnly()
      && task->desktop != WindowSystem::currentDesktop()) {
    return false;
  }

  if (model_->currentScreenTasksOnly() &&
      !screenGeometry_.intersects(QRect(task->x, task->y, task->width, task->height))) {
    return false;
  }

  if (task->activity != WindowSystem::currentActivity()) {
    return false;
  }

  return true;
}

bool DockPanel::hasTask(std::string_view uuid) {
  for (auto& item : items_) {
    if (item->hasTask(uuid)) {
      return true;
    }
  }
  return false;
}

void DockPanel::initClock() {
  if (showClock_) {
    items_.push_back(std::make_unique<Clock>(
        this, model_, orientation_, minSize_, maxSize_));
  }
}

void DockPanel::initLayoutVars() {
  itemSpacing_ = std::round(minSize_ / 1.7 * spacingFactor_);
  margin3D_ = 2 * itemSpacing_;
  floatingMargin_ = model_->floatingMargin();
  parabolicMaxX_ = std::round(2.5 * (minSize_ + itemSpacing_));
  numAnimationSteps_ = 20;
  animationSpeed_ = 16;

  QFont font;
  font.setPointSize(model_->tooltipFontSize());
  font.setBold(true);
  QFontMetrics metrics(font);
  tooltipSize_ = metrics.boundingRect("Tooltip").height();

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
    minWidth_ = itemSpacing_;
    if (isBottom() && is3D()) { minWidth_ += 2 * margin3D_; }
    for (const auto& item : items_) {
      minWidth_ += (item->getMinWidth() + itemSpacing_);
    }
    minHeight_ = autoHide() ? kAutoHideSize : minSize_ + 2 * itemSpacing_;
    maxWidth_ = minWidth_ + delta;
    maxHeight_ = 2 * itemSpacing_ + maxSize_ + tooltipSize_;
  } else {  // Vertical
    minHeight_ = itemSpacing_;
    for (const auto& item : items_) {
      minHeight_ += (item->getMinHeight() + itemSpacing_);
    }
    minWidth_ = autoHide() ? kAutoHideSize : minSize_ + 2 * itemSpacing_;
    maxHeight_ = minHeight_ + delta;
    maxWidth_ = 2 * itemSpacing_ + maxSize_ + tooltipSize_;
  }

  resize(maxWidth_, maxHeight_);
}

void DockPanel::updateLayout() {
  if (isLeaving_) {
    for (const auto& item : items_) {
      item->setAnimationStartAsCurrent();
      if (isHorizontal()) {
        startBackgroundWidth_ = backgroundWidth_;
        startBackgroundHeight_ = minSize_ + 2 * itemSpacing_;
      } else {  // Vertical
        startBackgroundHeight_ = backgroundHeight_;
        startBackgroundWidth_ = minSize_ + 2 * itemSpacing_;
      }
    }
  }

  for (int i = 0; i < itemCount(); ++i) {
    items_[i]->size_ = minSize_;
    if (isHorizontal()) {
      items_[i]->left_ =
          (i == 0) ? isBottom() && is3D() ? itemSpacing_ + (maxWidth_ - minWidth_) / 2 + margin3D_
                                          : itemSpacing_ + (maxWidth_ - minWidth_) / 2
                   : items_[i - 1]->left_ + items_[i - 1]->getMinWidth() + itemSpacing_;
      items_[i]->top_ = isTop()
          ? isFloating() ? itemSpacing_ + floatingMargin_ : itemSpacing_
          : isFloating() ? itemSpacing_ + maxHeight_ - minHeight_ - floatingMargin_
                         : itemSpacing_ + maxHeight_ - minHeight_;
      if (is3D() && isBottom()) { items_[i]->top_ -= k3DPanelThickness; }
      items_[i]->minCenter_ = items_[i]->left_ + items_[i]->getMinWidth() / 2;
    } else {  // Vertical
      items_[i]->left_ = isLeft()
          ? isFloating() ? itemSpacing_ + floatingMargin_ : itemSpacing_
          : isFloating() ? itemSpacing_ + maxWidth_ - minWidth_ - floatingMargin_
                         : itemSpacing_ + maxWidth_ - minWidth_;
      items_[i]->top_ = (i == 0) ? itemSpacing_ + (maxHeight_ - minHeight_) / 2
          : items_[i - 1]->top_ + items_[i - 1]->getMinHeight() + itemSpacing_;
      items_[i]->minCenter_ = items_[i]->top_ + items_[i]->getMinHeight() / 2;
    }
  }
  if (isHorizontal()) {
    backgroundWidth_ = minWidth_;
    backgroundHeight_ = minSize_ + 2 * itemSpacing_;
  } else {  // Vertical
    backgroundHeight_ = minHeight_;
    backgroundWidth_ = minSize_ + 2 * itemSpacing_;
  }

  if (isLeaving_) {
    for (const auto& item : items_) {
      item->endSize_ = item->size_;
      item->endLeft_ = item->left_;
      item->endTop_ = item->top_;
      item->startAnimation(numAnimationSteps_);
    }
    if (isHorizontal()) {
      endBackgroundWidth_ = minWidth_;
      backgroundWidth_ = startBackgroundWidth_;
      endBackgroundHeight_ = autoHide() ? kAutoHideSize : minSize_ + 2 * itemSpacing_;
      backgroundHeight_ = startBackgroundHeight_;
    } else {  // Vertical
      endBackgroundHeight_ = minHeight_;
      backgroundHeight_ = startBackgroundHeight_;
      endBackgroundWidth_ = autoHide() ? kAutoHideSize : minSize_ + 2 * itemSpacing_;
      backgroundWidth_ = startBackgroundWidth_;
    }
    currentAnimationStep_ = 0;
    isAnimationActive_ = true;
    animationTimer_->start(32 - animationSpeed_);
  } else {
    WindowSystem::setLayer(this,
                           visibility_ == PanelVisibility::AlwaysOnTop
                              ? LayerShellQt::Window::LayerTop
                              : LayerShellQt::Window::LayerBottom);
    isMinimized_ = true;
    setMask();
    update();
  }
}

void DockPanel::updateLayout(int x, int y) {
  if (isEntering_) {
    for (const auto& item : items_) {
      item->startSize_ = item->size_;
      item->startLeft_ = item->left_;
      item->startTop_ = item->top_;
    }
    if (isHorizontal()) {
      startBackgroundWidth_ = minWidth_;
      startBackgroundHeight_ = autoHide() ? kAutoHideSize : minSize_ + 2 * itemSpacing_;
    } else {  // Vertical
      startBackgroundHeight_ = minHeight_;
      startBackgroundWidth_ = autoHide() ? kAutoHideSize : minSize_ + 2 * itemSpacing_;
    }
  }

  int first_update_index = -1;
  int last_update_index = 0;
  if (isHorizontal()) {
    items_[0]->left_ = isBottom() && is3D() ? itemSpacing_ + margin3D_
                                            : itemSpacing_;
  } else {  // Vertical
    items_[0]->top_ = itemSpacing_;
  }
  for (int i = 0; i < itemCount(); ++i) {
    int delta;
    if (isHorizontal()) {
      delta = std::abs(items_[i]->minCenter_ - x);
    } else {  // Vertical
      delta = std::abs(items_[i]->minCenter_ - y);
    }
    if (delta < parabolicMaxX_) {
      if (first_update_index == -1) {
        first_update_index = i;
      }
      last_update_index = i;
    }
    items_[i]->size_ = parabolic(delta);
    if (isHorizontal()) {
      items_[i]->top_ = isTop()
          ? isFloating() ? itemSpacing_ + floatingMargin_ : itemSpacing_
          : isFloating() ? itemSpacing_ + tooltipSize_ + maxSize_ - items_[i]->getHeight()
                           - floatingMargin_
                         : itemSpacing_ + tooltipSize_ + maxSize_ - items_[i]->getHeight();
      if (is3D() && isBottom()) { items_[i]->top_ -= k3DPanelThickness; }
    } else {
      items_[i]->left_ = isLeft()
          ? isFloating() ? itemSpacing_ + floatingMargin_ : itemSpacing_
          : isFloating() ? itemSpacing_ + tooltipSize_ + maxSize_ - items_[i]->getWidth()
                           - floatingMargin_
                         : itemSpacing_ + tooltipSize_ + maxSize_ - items_[i]->getWidth();
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
          ? isBottom() && is3D() ? maxWidth_ - itemSpacing_ - items_[i]->getMinWidth() - margin3D_
                                 : maxWidth_ - itemSpacing_ - items_[i]->getMinWidth()
          : items_[i + 1]->left_ - items_[i]->getMinWidth() - itemSpacing_;
    } else {  // Vertical
      items_[i]->top_ = (i == itemCount() - 1)
          ? maxHeight_ - itemSpacing_ - items_[i]->getMinHeight()
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
      endBackgroundHeight_ = minSize_ + 2 * itemSpacing_;
      backgroundHeight_ = startBackgroundHeight_;
    } else {  // Vertical
      endBackgroundHeight_ = maxHeight_;
      backgroundHeight_ = startBackgroundHeight_;
      endBackgroundWidth_ = minSize_ + 2 * itemSpacing_;
      backgroundWidth_ = startBackgroundWidth_;
    }

    currentAnimationStep_ = 0;
    isAnimationActive_ = true;
    isEntering_ = false;
    animationTimer_->start(32 - animationSpeed_);
  }

  mouseX_ = x;
  mouseY_ = y;

  //resize(maxWidth_, maxHeight_);
  WindowSystem::setLayer(this, LayerShellQt::Window::LayerTop);
  isMinimized_ = false;
  setMask();
  updateActiveItem(x, y);
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
    //QWidget::resize(maxWidth_, maxHeight_);
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
      left = (i == 0) ? isBottom() && is3D() ? itemSpacing_ + (maxWidth_ - minWidth_) / 2 + margin3D_
                                             : itemSpacing_ + (maxWidth_ - minWidth_) / 2
                      : left + items_[i - 1]->getMinWidth() + itemSpacing_;
      if (i >= itemsToKeep) {
        items_[i]->minCenter_ = left + items_[i]->getMinWidth() / 2;
      }
    } else {  // Vertical
      top = (i == 0) ? itemSpacing_ + (maxHeight_ - minHeight_) / 2
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
      delta = std::abs(items_[i]->minCenter_ - mouseX_);
    } else {  // Vertical
      delta = std::abs(items_[i]->minCenter_ - mouseY_);
    }
    if (delta < parabolicMaxX_) {
      last_update_index = i;
    }
    items_[i]->size_ = parabolic(delta);
    if (isHorizontal()) {
      items_[i]->top_ = isTop() ? itemSpacing_
                                : itemSpacing_ + tooltipSize_ + maxSize_ - items_[i]->getHeight();
    } else {
      items_[i]->left_ = isLeft() ? itemSpacing_
                                  : itemSpacing_ + tooltipSize_ + maxSize_ - items_[i]->getWidth();
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
          ? isBottom() && is3D() ? maxWidth_ - itemSpacing_ - items_[i]->getMinWidth() - margin3D_
                                 : maxWidth_ - itemSpacing_ - items_[i]->getMinWidth()
          : items_[i + 1]->left_ - items_[i]->getMinWidth() - itemSpacing_;
    } else {  // Vertical
      items_[i]->top_ = (i == itemCount() - 1)
          ? maxHeight_ - itemSpacing_ - items_[i]->getMinHeight()
          : items_[i + 1]->top_ - items_[i]->getMinHeight() - itemSpacing_;
    }
  }

  update();
}

void DockPanel::setStrut(int width) {
  LayerShellQt::Window::Anchors anchor;
  switch (position_) {
    case PanelPosition::Top:
      anchor = LayerShellQt::Window::AnchorTop;
      break;
    case PanelPosition::Bottom:
      anchor = LayerShellQt::Window::AnchorBottom;
      break;
    case PanelPosition::Left:
      anchor = LayerShellQt::Window::AnchorLeft;
      break;
    case PanelPosition::Right:
      anchor = LayerShellQt::Window::AnchorRight;
      break;
  }

  WindowSystem::setAnchorAndStrut(this, anchor, width);
}

void DockPanel::setMask() {
  if (isMinimized_) {
    constexpr int kBuffer = 20;  // to avoid some visual clipping.
    if (isHorizontal()) {
      const int x = (maxWidth_ - minWidth_) / 2 - kBuffer;
      const int y = isTop() ? 0 : maxHeight_ - minHeight_ - kBuffer;
      QWidget::setMask(
          QRegion(x, y, minWidth_ + 2 * kBuffer, minHeight_ + kBuffer));
    } else {  // Vertical.
        const int x = isLeft() ? 0 : maxWidth_ - minWidth_ - kBuffer;
        const int y = (maxHeight_ - minHeight_) / 2 - kBuffer;
        QWidget::setMask(
            QRegion(x, y, minWidth_ + kBuffer, minHeight_ + 2 * kBuffer));
    }
  } else {
    QWidget::setMask(QRegion(0, 0, width(), height()));
  }
}

void DockPanel::updateActiveItem(int x, int y) {
  int i = 0;
  while (i < itemCount() &&
      ((orientation_ == Qt::Horizontal && items_[i]->left_ < x) ||
      (orientation_ == Qt::Vertical && items_[i]->top_ < y))) {
    ++i;
  }
  activeItem_ = i - 1;
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
