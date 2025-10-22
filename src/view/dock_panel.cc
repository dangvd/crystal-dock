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
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QWheelEvent>
#include <QFont>
#include <QFontMetrics>
#include <QGuiApplication>
#include <QIcon>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QMimeData>
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
#include "trash.h"
#include "version_checker.h"
#include "volume_control.h"
#include "wifi_manager.h"
#include <display/window_system.h>
#include <utils/draw_utils.h>
#include <utils/icon_utils.h>

namespace ranges = std::ranges;

namespace crystaldock {

/*static*/ constexpr char DockPanel::kVersion[] = "2.15 RC15";

DockPanel::DockPanel(MultiDockView* parent, MultiDockModel* model, int dockId)
    : QWidget(),
      parent_(parent),
      model_(model),
      dockId_(dockId),
      visibility_(PanelVisibility::AlwaysVisible),
      showPager_(false),
      showClock_(false),
      showTrash_(false),
      aboutDialog_(QMessageBox::Information, "About Crystal Dock",
                   QString("<h3>Crystal Dock ") + kVersion + "</h3>"
                   + "<p>Copyright (C) 2025 Viet Dang (dangvd@gmail.com)"
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
      isHidden_(false),
      isEntering_(false),
      isLeaving_(false),
      isAnimationActive_(false),
      isShowingPopup_(false),
      animationTimer_(std::make_unique<QTimer>(this)) {  
  setAttribute(Qt::WA_TranslucentBackground);
  setWindowFlag(Qt::FramelessWindowHint);
  setMouseTracking(true);
  setAcceptDrops(true);

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
  connect(WindowSystem::self(), SIGNAL(windowStateChanged(const WindowInfo*)),
          this, SLOT(onWindowStateChanged(const WindowInfo*)));
  connect(WindowSystem::self(), SIGNAL(windowTitleChanged(const WindowInfo*)),
          this, SLOT(onWindowTitleChanged(const WindowInfo*)));
  connect(WindowSystem::self(), SIGNAL(activeWindowChanged(void*)),
          this, SLOT(onActiveWindowChanged()));
  connect(WindowSystem::self(), SIGNAL(windowAdded(const WindowInfo*)),
          this, SLOT(onWindowAdded(const WindowInfo*)));
  connect(WindowSystem::self(), SIGNAL(windowRemoved(void*)),
          this, SLOT(onWindowRemoved(void*)));
  connect(WindowSystem::self(), SIGNAL(windowLeftCurrentDesktop(void*)),
          this, SLOT(onWindowLeftCurrentDesktop(void*)));
  connect(WindowSystem::self(), SIGNAL(windowLeftCurrentActivity(void*)),
          this, SLOT(onWindowLeftCurrentActivity(void*)));
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
  setMask();
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
  intellihideHideUnhide();
}

void DockPanel::onCurrentActivityChanged() {
  reloadTasks();
  intellihideHideUnhide();
}

void DockPanel::setStrut() {
  switch(visibility_) {
    case PanelVisibility::AlwaysVisible:
      setStrut(isHorizontal() ? minHeight_ : minWidth_);
      break;
    case PanelVisibility::AutoHide:
    case PanelVisibility::IntelligentAutoHide:
      setStrut(WindowSystem::hasAutoHideManager() ? 0 : 1);
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
  screenOutput_ = WindowSystem::getWlOutputForScreen(screen);
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
      if (isHidden_ && !hasFocus()) { setAutoHide(); }
    }
  }
  repaint();
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
  intellihideHideUnhide();
  if (autoHide() && !isHidden_) { setAutoHide(); }

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

void DockPanel::onWindowRemoved(void* window) {
  intellihideHideUnhide(window);

  if (!showTaskManager()) {
    return;
  }

  removeTask(window);

  if (isEmpty()) {
    intellihideHideUnhide();
  }
}

void DockPanel::onWindowLeftCurrentDesktop(void* window) {
  if (showTaskManager() && model_->currentDesktopTasksOnly()) {
    removeTask(window);
  }
}

void DockPanel::onWindowLeftCurrentActivity(void* window) {
  if (showTaskManager()) {
    removeTask(window);
  }
}

void DockPanel::onWindowGeometryChanged(const WindowInfo* task) {
  intellihideHideUnhide();

  if (!showTaskManager()) {
    return;
  }

  if (!model_->currentScreenTasksOnly()) {
    return;
  }

  QRect windowGeometry(task->x, task->y, task->width, task->height);
  if (hasTask(task->window)) {
    if (!windowGeometry.intersects(screenGeometry_)) {
      removeTask(task->window);
    }
  } else {
    if (windowGeometry.intersects(screenGeometry_) && isValidTask(task)) {
      if (addTask(task)) {
        resizeTaskManager();
      }
    }
  }
}


void DockPanel::onWindowStateChanged(const WindowInfo *task) {
  intellihideHideUnhide();

  if (!showTaskManager()) {
    return;
  }

  for (auto& item : items_) {
    if (item->hasTask(task->window)) {
      item->setDemandsAttention(task->demandsAttention);
      return;
    }
  }
}

void DockPanel::onWindowTitleChanged(const WindowInfo *task) {
  if (model_->groupTasksByApplication()) {
    return;
  }

  for (auto& item : items_) {
    if (item->hasTask(task->window)) {
      item->setLabel(QString::fromStdString(task->title));
      update();
      return;
    }
  }
}

void DockPanel::onActiveWindowChanged() {
  update();
}

int DockPanel::taskIndicatorPos() {
  const auto margin = isGlass2D() || (is3D() && !isBottom())
      ? kIndicatorMarginGlass2D
      : isFlat2D()
          ? kIndicatorSizeFlat2D
          : kIndicatorSizeMetal2D / 2;
  if (isHorizontal()) {
    int y = 0;
    if (is3D() && isBottom()) {
      y = maxHeight_ - k3DPanelThickness - 2;
    } else {  // 2D
      if (isTop()) {
        y = itemSpacing_ / 3;
      } else {  // bottom
        y = maxHeight_ - itemSpacing_ / 3 - margin;
      }
    }

    if (isFloating()) {
      if (isTop()) {
        y += floatingMargin_;
      } else {
        y -= floatingMargin_;
      }
    }

    return y;
  } else {  // Vertical.
    int x = 0;
    if (isLeft()) {
      x = itemSpacing_ / 3;
    } else {  // right
      x = maxWidth_ - itemSpacing_ / 3 - margin;
    }

    if (isFloating()) {
      if (isLeft()) {
        x += floatingMargin_;
      } else {
        x -= floatingMargin_;
      }
    }

    return x;
  }
}

int DockPanel::itemCount(const QString& appId) {
  const auto first = ranges::find_if(
      items_,
      [&appId](auto& item) { return appId == item->getAppId(); });
  if (first == items_.end()) {
    return 0;
  }
  const auto last = ranges::find_if(
      first, items_.end(),
      [&appId](auto& item) { return appId != item->getAppId(); });
  return last - first;
}

void DockPanel::updatePinnedStatus(const QString& appId, bool pinned) {
  const auto first = ranges::find_if(
      items_,
      [&appId](auto& item) { return appId == item->getAppId(); });
  if (first == items_.end()) {
    return;
  }
  const auto last = ranges::find_if(
      first, items_.end(),
      [&appId](auto& item) { return appId != item->getAppId(); });
  ranges::for_each(first, last, [pinned](auto& item) { item->updatePinnedStatus(pinned); });
}

void DockPanel::setShowingPopup(bool showingPopup) {
  isShowingPopup_ = showingPopup;
  if (!isShowingPopup_) {
    // We have to do these complicated workarounds because QCursor::pos() does not
    // exactly return the current mouse position but it depends on related mouse events.
    auto mousePosition = mapFromGlobal(QCursor::pos());
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
    int x2 = 0;
    int y2 = 0;
    int w2 = 0;
    int h2 = 0;
    switch (position_) {
    case PanelPosition::Top:
      x = itemSpacing_;
      w = maxWidth_ - 2 * x;
      y = itemSpacing_ + (isFloating() ? floatingMargin_ : 0);
      h = minSize_;
      if (activeItem_ >= 0 && activeItem_ < items_.size()) {
        x2 = items_[activeItem_]->left_;
        w2 = items_[activeItem_]->getMaxWidth();
        y2 = y;
        h2 = maxSize_;
      }
      break;
    case PanelPosition::Bottom:
      x = itemSpacing_ + (is3D() && isBottom() ? margin3D_ : 0);
      w = maxWidth_ - 2 * x;
      y = maxHeight_ - itemSpacing_ - (isFloating() ? floatingMargin_ : 0)
          - (is3D() && isBottom() ? k3DPanelThickness : 0) - minSize_;
      h = minSize_;
      if (activeItem_ >= 0 && activeItem_ < items_.size()) {
        x2 = items_[activeItem_]->left_;
        w2 = items_[activeItem_]->getMaxWidth();
        y2 = y + minSize_ - maxSize_;
        h2 = maxSize_;
      }
      break;
    case PanelPosition::Left:
      y = itemSpacing_;
      h = maxHeight_ - 2 * y;
      x = itemSpacing_ + (isFloating() ? floatingMargin_ : 0);
      w = minSize_;
      if (activeItem_ >= 0 && activeItem_ < items_.size()) {
        y2 = items_[activeItem_]->top_;
        h2 = items_[activeItem_]->getMaxHeight();
        x2 = y;
        w2 = maxSize_;
      }
      break;
    case PanelPosition::Right:
      y = itemSpacing_;
      h = maxHeight_ - 2 * y;
      x = maxWidth_ - itemSpacing_ - (isFloating() ? floatingMargin_ : 0) - minSize_;
      w = minSize_;
      if (activeItem_ >= 0 && activeItem_ < items_.size()) {
        y2 = items_[activeItem_]->top_;
        h2 = items_[activeItem_]->getMaxHeight();
        x2 = x + minSize_ - maxSize_;
        w2 = maxSize_;
      }
      break;
    }

    QRect rect(x, y, w, h);
    QRect rect2(x2, y2, w2, h2);
    if (!rect.contains(mousePosition) && !rect2.contains(mousePosition)) {
      leaveEvent(nullptr);
    }
  }
}

void DockPanel::paintEvent(QPaintEvent* e) {
  if (!WindowSystem::hasAutoHideManager() && isHidden_ && (autoHide() || intellihide())) {
    return;
  }

  QPainter painter(this);

  if (is3D()) {
    drawGlass3D(painter);
  } else {
    draw2D(painter);
  }

  drawTooltip(painter);
}

void DockPanel::drawGlass3D(QPainter& painter) {
  if (isHorizontal()) {
    int y = isTop()
        ? isFloating() ? floatingMargin_ : 0
        : isFloating() ? maxHeight_ - backgroundHeight_ - floatingMargin_
                       : maxHeight_ - backgroundHeight_;
    if (isBottom()) {  // 3D styles only apply to bottom dock.
      y -= k3DPanelThickness;
      draw3dDockPanel(
          (maxWidth_ - backgroundWidth_) / 2, y, backgroundWidth_ - 1, backgroundHeight_ - 1,
           backgroundHeight_ / 16, borderColor_, backgroundColor_, &painter);
    } else {
      fillRoundedRect(
          (maxWidth_ - backgroundWidth_) / 2, y, backgroundWidth_ - 1, backgroundHeight_ - 1,
           backgroundHeight_ / 16, /*showBorder=*/true, borderColor_, backgroundColor_, &painter);
    }
  } else {  // Vertical
    const int x =  isLeft()
        ? isFloating() ? floatingMargin_ : 0
        : isFloating() ? maxWidth_ - backgroundWidth_ - floatingMargin_
                       : maxWidth_ - backgroundWidth_;
    fillRoundedRect(x, (maxHeight_ - backgroundHeight_) / 2, backgroundWidth_ - 1, backgroundHeight_ - 1,
                    backgroundWidth_ / 16, /*showBorder=*/true, borderColor_, backgroundColor_, &painter);
  }

  if (isBottom()) {
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
}

void DockPanel::draw2D(QPainter& painter) {
  const QColor bgColor = isGlass2D()
      ? model_->backgroundColor()
      : isFlat2D()
          ? model_->backgroundColor2D()
          : model_->backgroundColorMetal2D();
  const auto showBorder = isGlass2D() || isMetal2D();
  const QColor borderColor = isGlass2D() ? model_->borderColor() : model_->borderColorMetal2D();
  if (isHorizontal()) {
    const int y = isTop()
        ? isFloating() ? floatingMargin_ : 0
        : isFloating() ? maxHeight_ - backgroundHeight_ - floatingMargin_
                       : maxHeight_ - backgroundHeight_;
    const int r = isGlass2D()
        ? backgroundHeight_ / 16
        : isFlat2D()
            ? backgroundHeight_ / 4
            : 0;
    fillRoundedRect(
        (maxWidth_ - backgroundWidth_) / 2, y, backgroundWidth_ - 1, backgroundHeight_ - 1,
         r, showBorder, borderColor, bgColor, &painter);
  } else {  // Vertical
    const int x =  isLeft()
        ? isFloating() ? floatingMargin_ : 0
        : isFloating() ? maxWidth_ - backgroundWidth_ - floatingMargin_
                       : maxWidth_ - backgroundWidth_;
    const int r = isGlass2D()
        ? backgroundWidth_ / 16
        : isFlat2D()
            ? backgroundWidth_ / 4
            : 0;
    fillRoundedRect(
          x, (maxHeight_ - backgroundHeight_) / 2, backgroundWidth_ - 1, backgroundHeight_ - 1,
          r, showBorder, borderColor, bgColor, &painter);
  }

  // Draw the items from the end to avoid zoomed items getting clipped by
  // non-zoomed items.
  for (int i = itemCount() - 1; i >= 0; --i) {
    items_[i]->draw(&painter);
  }
}

void DockPanel::drawTooltip(QPainter& painter) {
  if (model_->showTooltip() && !isAnimationActive_ && activeItem_ >= 0 &&
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
      int y = isTop() ? maxHeight_ - tooltipSize_ / 2 : tooltipSize_ * 3 / 4;
      drawBorderedText(x, y, item->getLabel(), /*borderWidth*/ 2, Qt::black, Qt::white, &painter);
    } else {  // Vertical
      // Do not draw tooltip for Vertical positions for now because the total
      // area of the dock would take too much desktop space.
    }
  }
}

void DockPanel::mouseMoveEvent(QMouseEvent* e) {
  const auto x = e->position().x();
  const auto y = e->position().y();

  if (isEntering_) {
    // Don't do the parabolic zooming if the mouse is outside the minimized area.
    // Also don't do the parabolic zooming if the mouse is near the border.
    // Quite often the user was just scrolling a window etc.
    if (!checkMouseEnter(x, y)) {
      return;
    }
  }

  if (isAnimationActive_) {
    return;
  }

  updateLayout(x, y);
}

bool DockPanel::checkMouseEnter(int x, int y) {
  int x0, y0;
  if (position_ == PanelPosition::Bottom) {
    if (!WindowSystem::hasAutoHideManager() && visibility_ == PanelVisibility::AutoHide) {
      y0 = maxHeight_ - 1;
    } else {
      y0 = maxHeight_ - minHeight_;
      if (isFloating()) { y0 += floatingMargin_; }
    }
    if (y < y0) {
      return false;
    }
  } else if (position_ == PanelPosition::Top) {
    if (!WindowSystem::hasAutoHideManager() && visibility_ == PanelVisibility::AutoHide) {
      y0 = 1;
    } else {
      y0 = minHeight_;
      if (isFloating()) { y0 -= floatingMargin_; }
    }
    if (y > y0) {
      return false;
    }
  } else if (position_ == PanelPosition::Left) {
    if (!WindowSystem::hasAutoHideManager() && visibility_ == PanelVisibility::AutoHide) {
      x0 = 1;
    } else {
      x0 = minWidth_;
      if (isFloating()) { x0 -= floatingMargin_; }
    }
    if (x > x0) {
      return false;
    }
  } else {  // Right
    if (!WindowSystem::hasAutoHideManager() && visibility_ == PanelVisibility::AutoHide) {
      x0 = maxWidth_ - 1;
    } else {
      x0 = maxWidth_ - minWidth_;
      if (isFloating()) { x0 += floatingMargin_; }
    }
    if (x < x0) {
      return false;
    }
  }

  if (isHorizontal() &&
      (x < (maxWidth_ - minWidth_) / 2 || x > (maxWidth_ + minWidth_) / 2)) {
    return false;
  }
  if (!isHorizontal() &&
      (y < (maxHeight_ - minHeight_) / 2 || y > (maxHeight_ + minHeight_) / 2)) {
    return false;
  }

  return true;
}

bool DockPanel::intellihideShouldHide(void* excluding_window) {
  if (visibility_ != PanelVisibility::IntelligentAutoHide) {
    return false;
  }

  if (isEmpty()) {
    return true;
  }

  // For tiling compositors, we only show the dock if there's no window.
  if (DesktopEnv::getDesktopEnv()->isTiling()) {
    for (const auto* task : WindowSystem::windows()) {
      if (isValidTask(task) && (!excluding_window || task->window != excluding_window)) {
        return true;
      }
    }
    return false;
  }

  // For stacking compositors, we hide the dock if there's a maximized/fullscreen window.
  // If the compositor emits window geometry event, we also hide the dock if there's
  // a window that overlaps the dock.
  QRect dockGeometry = getMinimizedDockGeometry();
  for (const auto* task : WindowSystem::windows()) {
    if (isValidTask(task) && (!excluding_window || task->window != excluding_window)) {
      if ((task->maximized || task->fullscreen)
          && task->outputs.contains(screenOutput_)) {
        return true;
      }

      QRect windowGeometry(task->x, task->y, task->width, task->height);
      if (windowGeometry.isValid() && !task->minimized && windowGeometry.intersects(dockGeometry)) {
        return true;
      }
    }
  }

  return false;
}

void DockPanel::intellihideHideUnhide(void* excluding_window) {
  if (visibility_ != PanelVisibility::IntelligentAutoHide) {
    return;
  }

  if (intellihideShouldHide(excluding_window)) {
    if (!isHidden_ && isMinimized_) {
      setAutoHide();
    }
  } else {
    if (isHidden_) {
      setAutoHide(false);
    }
  }
}

bool DockPanel::isEmpty() {
  for (const auto& item : items_) {
    if (item->getAppId() != kSeparatorId && item->getAppId() != kLauncherSeparatorId) {
      return false;
    }
  }
  return true;
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

void DockPanel::wheelEvent(QWheelEvent* e) {
  if (isAnimationActive_) {
    return;
  }

  if (activeItem_ >= 0 && activeItem_ < static_cast<int>(items_.size())) {
    // Check if the active item is a VolumeControl
    VolumeControl* volumeControl = dynamic_cast<VolumeControl*>(items_[activeItem_].get());
    if (volumeControl) {
      volumeControl->wheelEvent(e);
    }
  }
}

void DockPanel::enterEvent (QEnterEvent* e) {
  if (isMinimized_) {
    isEntering_ = true;
  }
}

void DockPanel::leaveEvent(QEvent* e) {
  if (isMinimized_ || isShowingPopup_) {
    return;
  }

  isLeaving_ = true;
  updateLayout();
  activeItem_ = -1;
}

void DockPanel::dragEnterEvent(QDragEnterEvent* e) {
  if (e->mimeData()->hasUrls()) {
    e->acceptProposedAction();
    
    for (const auto& item : items_) {
      Trash* trash = dynamic_cast<Trash*>(item.get());
      if (trash) {
        trash->setAcceptDrops(true);
      }
    }
  }
}

void DockPanel::dragMoveEvent(QDragMoveEvent* e) {
  if (e->mimeData()->hasUrls()) {
    e->acceptProposedAction();
  }
}

void DockPanel::dropEvent(QDropEvent* e) {
  for (const auto& item : items_) {
    Trash* trash = dynamic_cast<Trash*>(item.get());
    if (trash) {
      trash->setAcceptDrops(false);
      trash->dropEvent(e);
      return;
    }
  }
}

void DockPanel::initUi() {
  initApplicationMenu();
  initPager();
  initLaunchers();
  initTasks();
  initTrash();
  initWifiManager();
  initVolumeControl();
  initVersionChecker();
  initClock();
  initLayoutVars();
  updateLayout();
  setStrut();
}

void DockPanel::addPanelSettings(QMenu* menu) {
  for (const auto& action : menu_.actions()) {
    menu->addAction(action);
  }
}

void DockPanel::createMenu() {
  QMenu* panelMenu = menu_.addMenu("&Panel");
  panelMenu->addAction(QIcon::fromTheme("list-add"), QString("&Add Panel"),
      this, SLOT(addDock()));
  panelMenu->addAction(QIcon::fromTheme("edit-copy"), QString("&Clone Panel"),
      this, SLOT(cloneDock()));
  panelMenu->addAction(QIcon::fromTheme("edit-delete"), QString("&Remove Panel"),
      this, SLOT(removeDock()));
  panelMenu->addSeparator();

  QMenu* extraComponents = panelMenu->addMenu(QString("&Optional Features"));
  applicationMenuAction_ = extraComponents->addAction(QString("Application Menu"), this,
      SLOT(toggleApplicationMenu()));
  applicationMenuAction_->setCheckable(true);
  pagerAction_ = extraComponents->addAction(QString("Pager"), this,
      SLOT(togglePager()));
  pagerAction_->setCheckable(true);
  taskManagerAction_ = extraComponents->addAction(QString("Task Manager"), this,
      SLOT(toggleTaskManager()));
  taskManagerAction_->setCheckable(true);
  trashAction_ = extraComponents->addAction(QString("Trash"), this,
      SLOT(toggleTrash()));
  trashAction_->setCheckable(true);
  volumeControlAction_ = extraComponents->addAction(QString("Volume Control"), this,
                                                    SLOT(toggleVolumeControl()));
  volumeControlAction_->setCheckable(true);
  wifiManagerAction_ = extraComponents->addAction(QString("Wi-Fi Manager"), this,
                                                  SLOT(toggleWifiManager()));
  wifiManagerAction_->setCheckable(true);
  versionCheckerAction_ = extraComponents->addAction(QString("Version Checker"), this,
                                                     SLOT(toggleVersionChecker()));
  versionCheckerAction_->setCheckable(true);
  clockAction_ = extraComponents->addAction(QString("Clock"), this,
                                            SLOT(toggleClock()));
  clockAction_->setCheckable(true);

  QMenu* position = panelMenu->addMenu(QString("&Position"));
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
    QMenu* screen = panelMenu->addMenu(QString("Scr&een"));
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

  QMenu* visibility = panelMenu->addMenu(QString("&Visibility"));
  visibilityAlwaysVisibleAction_ = visibility->addAction(
      QString("Always &Visible"), this,
      [this]() { updateVisibility(PanelVisibility::AlwaysVisible); });
  visibilityAlwaysVisibleAction_->setCheckable(true);
  visibilityIntelligentAutoHideAction_ = visibility->addAction(
      QString("&Intelligent Auto Hide"), this,
      [this]() { updateVisibility(PanelVisibility::IntelligentAutoHide); });
  visibilityIntelligentAutoHideAction_->setCheckable(true);
  visibilityAutoHideAction_ = visibility->addAction(
      QString("Auto &Hide"), this,
      [this]() { updateVisibility(PanelVisibility::AutoHide); });
  visibilityAutoHideAction_->setCheckable(true);
  visibilityAlwaysOnTopAction_ = visibility->addAction(
      QString("Always On &Top"), this,
      [this]() { updateVisibility(PanelVisibility::AlwaysOnTop); });
  visibilityAlwaysOnTopAction_->setCheckable(true);

  panelMenu->addSeparator();
  panelMenu->addAction(QIcon::fromTheme("application-exit"), "E&xit", parent_, SLOT(exit()));

  QMenu* appearanceMenu = menu_.addMenu("&Appearance");
  appearanceMenu->addAction(
      QIcon::fromTheme("configure"), QString("Appearance &Settings"), this,
      [this] {
        minimize();
        QTimer::singleShot(DockPanel::kExecutionDelayMs, [this]{
          showAppearanceSettingsDialog();
        });
      });
  appearanceMenu->addSeparator();

  floatingStyleAction_ = appearanceMenu->addAction(
      QString("Floating Panel"), this,
      [this] {
        changeFloatingStyle();
      });
  floatingStyleAction_->setCheckable(true);
  floatingStyleAction_->setChecked(isFloating());

  glass3DStyleAction_ = appearanceMenu->addAction(
      QString("Style: Glass 3D"), this,
      [this] {
        changePanelStyle(isFloating() ? PanelStyle::Glass3D_Floating
                                      : PanelStyle::Glass3D_NonFloating);
      });
  glass3DStyleAction_->setCheckable(true);
  glass2DStyleAction_ = appearanceMenu->addAction(
      QString("Style: Glass 2D"), this,
      [this] {
        changePanelStyle(isFloating() ? PanelStyle::Glass2D_Floating
                                      : PanelStyle::Glass2D_NonFloating);
      });
  glass2DStyleAction_->setCheckable(true);
  flat2DStyleAction_ = appearanceMenu->addAction(
      QString("Style: Flat 2D"), this,
      [this] {
      changePanelStyle(isFloating() ? PanelStyle::Flat2D_Floating
                                    : PanelStyle::Flat2D_NonFloating);
      });
  flat2DStyleAction_->setCheckable(true);
  metal2DStyleAction_ = appearanceMenu->addAction(
      QString("Style: Metal 2D"), this,
      [this] {
      changePanelStyle(isFloating() ? PanelStyle::Metal2D_Floating
                                    : PanelStyle::Metal2D_NonFloating);
      });
  metal2DStyleAction_->setCheckable(true);

  QMenu* helpMenu = menu_.addMenu(QIcon::fromTheme("help-contents"), "&Help");
  helpMenu->addAction(QIcon::fromTheme("help-contents"),
                  QString("Online &Documentation"),
                  this, SLOT(showOnlineDocumentation()));
  helpMenu->addAction(QIcon::fromTheme("help-about"), QString("A&bout Crystal Dock"), this,
      [this] {
        minimize();
        QTimer::singleShot(DockPanel::kExecutionDelayMs, [this]{
          about();
        });
      });
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
  visibilityIntelligentAutoHideAction_->setChecked(
      visibility_ == PanelVisibility::IntelligentAutoHide);
  visibilityAutoHideAction_->setChecked(
      visibility_ == PanelVisibility::AutoHide);
  visibilityAlwaysOnTopAction_->setChecked(
      visibility_ == PanelVisibility::AlwaysOnTop);
}

void DockPanel::setPanelStyle(PanelStyle panelStyle) {
  panelStyle_ = panelStyle;
  floatingStyleAction_->setChecked(isFloating());
  glass3DStyleAction_->setChecked(is3D());
  glass2DStyleAction_->setChecked(isGlass2D());
  flat2DStyleAction_->setChecked(isFlat2D());
  metal2DStyleAction_->setChecked(isMetal2D());
}

void DockPanel::loadDockConfig() {
  setPosition(model_->panelPosition(dockId_));
  setScreen(model_->screen(dockId_));
  setVisibility(model_->visibility(dockId_));

  showApplicationMenu_ = model_->showApplicationMenu(dockId_);
  applicationMenuAction_->setChecked(showApplicationMenu_);

  showPager_ = model_->showPager(dockId_) && WindowSystem::hasVirtualDesktopManager();
  pagerAction_->setVisible(WindowSystem::hasVirtualDesktopManager());
  pagerAction_->setChecked(showPager_);

  taskManagerAction_->setChecked(model_->showTaskManager(dockId_));

  showClock_ = model_->showClock(dockId_);
  clockAction_->setChecked(showClock_);

  showTrash_ = model_->showTrash(dockId_);
  trashAction_->setChecked(showTrash_);

  showVersionChecker_ = model_->showVersionChecker(dockId_);
  versionCheckerAction_->setChecked(showVersionChecker_);

  showVolumeControl_ = model_->showVolumeControl(dockId_);
  volumeControlAction_->setChecked(showVolumeControl_);

  showWifiManager_ = model_->showWifiManager(dockId_);
  wifiManagerAction_->setChecked(showWifiManager_);
}

void DockPanel::saveDockConfig() {
  model_->setPanelPosition(dockId_, position_);
  model_->setScreen(dockId_, screen_);
  model_->setVisibility(dockId_, visibility_);
  model_->setShowApplicationMenu(dockId_, showApplicationMenu_);
  model_->setShowPager(dockId_, showPager_);
  model_->setShowTaskManager(dockId_, taskManagerAction_->isChecked());
  model_->setShowClock(dockId_, showClock_);
  model_->setShowTrash(dockId_, showTrash_);
  model_->setShowVersionChecker(dockId_, showVersionChecker_);
  model_->setShowVolumeControl(dockId_, showVolumeControl_);
  model_->setShowWifiManager(dockId_, showWifiManager_);
  model_->saveDockConfig(dockId_);
}

void DockPanel::loadAppearanceConfig() {
  minSize_ = model_->minIconSize();
  maxSize_ = model_->maxIconSize();
  spacingFactor_ = model_->spacingFactor();
  backgroundColor_ = model_->backgroundColor();
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
    if (launcherConfig.appId == kSeparatorId || launcherConfig.appId == kLauncherSeparatorId) {
      items_.push_back(std::make_unique<Separator>(
          this, model_, orientation_, minSize_, maxSize_,
          launcherConfig.appId == kLauncherSeparatorId));
    } else {
      QPixmap icon = loadIcon(launcherConfig.icon, kIconLoadSize);
      items_.push_back(std::make_unique<Program>(
          this, model_, launcherConfig.appId, launcherConfig.name, orientation_,
          icon, minSize_, maxSize_, launcherConfig.command,
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
  initTrash();
  initWifiManager();
  initVolumeControl();
  initVersionChecker();
  initClock();
  resizeTaskManager();
}

bool DockPanel::addTask(const WindowInfo* task) {
  // Checks is the task already exists.
  if (hasTask(task->window)) {
    return false;
  }

  // Tries adding the task to existing programs.
  for (auto& item : items_) {
    if (item->addTask(task)) {
      return false;
    }
  }

  // Adds a new program.
  auto app = model_->findApplication(task->appId);
  if (!app && !task->appId.empty()) {
    std::cerr << "Could not find application with id: " << task->appId
              << ". The window icon will have limited functionalities." << std::endl;
  }
  const QString label = app ? app->name : QString::fromStdString(task->title);
  const QString appId = app ? app->appId : QString::fromStdString(task->appId);
  QPixmap appIcon = app ? loadIcon(app->icon, kIconLoadSize) : QPixmap();
  QString taskIconName = QString::fromStdString(task->icon);
  QPixmap taskIcon = appIcon.isNull() && !taskIconName.isEmpty()
      ? loadIcon(taskIconName, kIconLoadSize) : QPixmap();
  if (app && appIcon.isNull()) {
    std::cerr << "Could not find icon with name: " << app->icon.toStdString()
              << " in the current icon theme and its fallbacks."
              << " The window icon will have limited functionalities." << std::endl;
  }

  int i = 0;
  for (; i < itemCount() && items_[i]->beforeTask(label); ++i);
  if (!model_->groupTasksByApplication()) {
    for (; i < itemCount() && items_[i]->getAppLabel() == label; ++i);
  }
  if (!appIcon.isNull()) {
    const auto pinned = !model_->groupTasksByApplication() &&
                        model_->launchers(dockId_).contains(app->appId);
    items_.insert(items_.begin() + i, std::make_unique<Program>(
        this, model_, appId, label, orientation_, appIcon, minSize_,
        maxSize_, app->command, /*isAppMenuEntry=*/true, pinned));
  } else if (!taskIcon.isNull()) {
    items_.insert(items_.begin() + i, std::make_unique<Program>(
        this, model_, appId, label, orientation_, taskIcon, minSize_, maxSize_));
  } else {
    items_.insert(items_.begin() + i, std::make_unique<Program>(
        this, model_, appId, label, orientation_, QPixmap(), minSize_, maxSize_));
  }
  items_[i]->addTask(task);

  return true;
}

void DockPanel::removeTask(void* window) {
  for (int i = 0; i < itemCount(); ++i) {
    if (items_[i]->removeTask(window)) {
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

  if (task->skipTaskbar) {
    return false;
  }

  if (WindowSystem::hasVirtualDesktopManager() && model_->currentDesktopTasksOnly()
      && !task->onAllDesktops && task->desktop != WindowSystem::currentDesktop()) {
    return false;
  }

  QRect taskGeometry(task->x, task->y, task->width, task->height);
  if (model_->currentScreenTasksOnly() && taskGeometry.isValid()
      && !screenGeometry_.intersects(taskGeometry)) {
    return false;
  }

  if (WindowSystem::hasActivityManager() && !WindowSystem::currentActivity().empty()
      && !task->activity.empty() && task->activity != WindowSystem::currentActivity()) {
    return false;
  }

  return true;
}

bool DockPanel::hasTask(void* window) {
  for (auto& item : items_) {
    if (item->hasTask(window)) {
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

void DockPanel::initTrash() {
  if (showTrash_) {
    items_.push_back(std::make_unique<Trash>(
        this, model_, orientation_, minSize_, maxSize_));
  }
}

void DockPanel::initVersionChecker() {
  if (showVersionChecker_) {
    items_.push_back(std::make_unique<VersionChecker>(
        this, model_, orientation_, minSize_, maxSize_));
  }
}

void DockPanel::initVolumeControl() {
  if (showVolumeControl_) {
    items_.push_back(std::make_unique<VolumeControl>(
        this, model_, orientation_, minSize_, maxSize_));
  }
}

void DockPanel::initWifiManager() {
  if (showWifiManager_) {
    items_.push_back(std::make_unique<WifiManager>(
        this, model_, orientation_, minSize_, maxSize_));
  }
}

void DockPanel::initLayoutVars() {
  const auto spacingMultiplier = isMetal2D() ? kSpacingMultiplierMetal2D : kSpacingMultiplier;
  itemSpacing_ = std::round(minSize_* spacingMultiplier * spacingFactor_);
  margin3D_ = static_cast<int>(minSize_ * 0.6);
  floatingMargin_ = model_->floatingMargin();
  parabolicMaxX_ = std::round(2.5 * (minSize_ + itemSpacing_));
  numAnimationSteps_ = 14;

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
    minBackgroundWidth_ = minWidth_;
    minHeight_ = minSize_ + 2 * itemSpacing_;
    minBackgroundHeight_ = minHeight_;
    maxWidth_ = minWidth_ + delta;
    maxHeight_ = 2 * itemSpacing_ + maxSize_ + tooltipSize_;
    if (isFloating()) {
      maxHeight_ += 2 * floatingMargin_;
      minHeight_ += 2 * floatingMargin_;
    }
    if (is3D() && isBottom()) {
      maxHeight_ += k3DPanelThickness;
      minHeight_ += k3DPanelThickness;
    }
  } else {  // Vertical
    minHeight_ = itemSpacing_;
    for (const auto& item : items_) {
      minHeight_ += (item->getMinHeight() + itemSpacing_);
    }
    minBackgroundHeight_ = minHeight_;
    minWidth_ = minSize_ + 2 * itemSpacing_;
    minBackgroundWidth_ = minWidth_;
    maxHeight_ = minHeight_ + delta;
    maxWidth_ = 2 * itemSpacing_ + maxSize_ + tooltipSize_;
    if (isFloating()) {
      maxWidth_ += 2 * floatingMargin_;
      minWidth_ += 2 * floatingMargin_;
    }
  }

  resize(maxWidth_, maxHeight_);
}

QRect DockPanel::getMinimizedDockGeometry() {
  QRect dockGeometry;
  dockGeometry.setX(
      isHorizontal()
      ? screenGeometry_.x() + (screenGeometry_.width() - minWidth_) / 2
      : isLeft()
          ? screenGeometry_.x()
          : screenGeometry_.x() + screenGeometry_.width() - minWidth_);
  dockGeometry.setY(
      isHorizontal()
      ? isTop()
        ? screenGeometry_.y()
        : screenGeometry_.y() + screenGeometry_.height() - minHeight_
      : screenGeometry_.y() + (screenGeometry_.height() - minHeight_) / 2);
  dockGeometry.setWidth(minWidth_);
  dockGeometry.setHeight(minHeight_);
  return dockGeometry;
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
      items_[i]->top_ = isTop() ? itemSpacing_
                                : itemSpacing_ + maxHeight_ - minHeight_;
      if (isFloating()) { items_[i]->top_ += floatingMargin_; }
      items_[i]->minCenter_ = items_[i]->left_ + items_[i]->getMinWidth() / 2;
    } else {  // Vertical
      items_[i]->left_ = isLeft() ? itemSpacing_
                                  : itemSpacing_ + maxWidth_ - minWidth_;
      if (isFloating()) { items_[i]->left_ += floatingMargin_; }
      items_[i]->top_ = (i == 0) ? itemSpacing_ + (maxHeight_ - minHeight_) / 2
          : items_[i - 1]->top_ + items_[i - 1]->getMinHeight() + itemSpacing_;
      items_[i]->minCenter_ = items_[i]->top_ + items_[i]->getMinHeight() / 2;
    }
  }

  backgroundWidth_ = minBackgroundWidth_;
  backgroundHeight_ = minBackgroundHeight_;

  if (isLeaving_) {
    for (const auto& item : items_) {
      item->endSize_ = item->size_;
      item->endLeft_ = item->left_;
      item->endTop_ = item->top_;
      item->startAnimation(numAnimationSteps_);
    }

    endBackgroundWidth_ = minBackgroundWidth_;
    backgroundWidth_ = startBackgroundWidth_;
    endBackgroundHeight_ = minBackgroundHeight_;
    backgroundHeight_ = startBackgroundHeight_;

    currentAnimationStep_ = 0;
    isAnimationActive_ = true;
    animationTimer_->start(32 - model_->zoomingAnimationSpeed());
  } else {
    WindowSystem::setLayer(this,
                           visibility_ == PanelVisibility::AlwaysVisible
                               ? LayerShellQt::Window::LayerBottom
                               : LayerShellQt::Window::LayerTop);
    isMinimized_ = true;
    if (autoHide()) { isHidden_ = true; }
    if (intellihide()) { isHidden_ = intellihideShouldHide(); }
    update();
    // Here we have to wait a bit before setMask() to avoid visual artifacts.
    QTimer::singleShot(500, [this]{ setMask(); });
  }
}

void DockPanel::updateLayout(int x, int y) {
  if (isEntering_) {
    for (const auto& item : items_) {
      item->startSize_ = item->size_;
      item->startLeft_ = item->left_;
      item->startTop_ = item->top_;
    }
    startBackgroundWidth_ = minBackgroundWidth_;
    startBackgroundHeight_ = minBackgroundHeight_;
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
      items_[i]->top_ = isTop() ? itemSpacing_
                                : itemSpacing_ + tooltipSize_ + maxSize_ - items_[i]->size_;
      if (isFloating()) { items_[i]->top_ += floatingMargin_; }
    } else {  // Vertical
      items_[i]->left_ = isLeft() ? itemSpacing_
                                  : itemSpacing_ + tooltipSize_ + maxSize_ - items_[i]->size_;
      if (isFloating()) { items_[i]->left_ += floatingMargin_; }
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

  if (first_update_index == -1) {
    if ((isHorizontal() && x < maxWidth_ / 2) || (!isHorizontal() && y < maxHeight_ / 2)) {
      first_update_index = last_update_index = 0;
    } else {
      first_update_index = last_update_index = itemCount() - 1;
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
    animationTimer_->start(32 - model_->zoomingAnimationSpeed());
  }

  mouseX_ = x;
  mouseY_ = y;

  //resize(maxWidth_, maxHeight_);
  WindowSystem::setLayer(this, LayerShellQt::Window::LayerTop);
  isMinimized_ = false;
  if (autoHide() || intellihide()) { isHidden_ = false; }
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
      if (isFloating()) { items_[i]->top_ += floatingMargin_; }
    } else {  // Vertical
      items_[i]->left_ = isLeft() ? itemSpacing_
                                  : itemSpacing_ + tooltipSize_ + maxSize_ - items_[i]->getWidth();
      if (isFloating()) { items_[i]->left_ += floatingMargin_; }
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

  setMask();
  update();
}

void DockPanel::setStrut(int width) {
  LayerShellQt::Window::Anchors anchor = LayerShellQt::Window::AnchorBottom;
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
    if (isHorizontal()) {
      const int x = (maxWidth_ - minWidth_) / 2;
      const int h = !WindowSystem::hasAutoHideManager() && isHidden_ ? 1 : minHeight_;
      const int y = isTop() ? 0 : maxHeight_ - h;
      QWidget::setMask(QRegion(x, y, minWidth_, h));
    } else {  // Vertical.
      const int y = (maxHeight_ - minHeight_) / 2;
      const int w = !WindowSystem::hasAutoHideManager() && isHidden_ ? 1 : minWidth_;
      const int x = isLeft() ? 0 : maxWidth_ - w;
      QWidget::setMask(QRegion(x, y, w, minHeight_));
    }
  } else {
    QWidget::setMask(QRegion(0, 0, maxWidth_, maxHeight_));
  }
}

void DockPanel::updatePosition(PanelPosition position) {
  setPosition(position);
  reload();
  if (isHidden_) {
    // we have to deactivate, wait then re-activate Auto Hide
    // otherwise the Auto Hide screen edge's border length would not be updated
    // correctly.
    setAutoHide(false);
    update();
    QTimer::singleShot(1000, [this]{ setAutoHide(); });
  }
  saveDockConfig();
}

void DockPanel::updateVisibility(PanelVisibility visibility) {
  setVisibility(visibility);
  setStrut();
  setAutoHide(autoHide() || intellihideShouldHide());
  saveDockConfig();
}

void DockPanel::setAutoHide(bool on) {
  if (isHidden_ != on) {
    isHidden_ = on;
  }

  if (!WindowSystem::hasAutoHideManager()) {
    repaint();
    setMask();
    return;
  }

  Qt::Edge edge = Qt::BottomEdge;
  switch (position_) {
    case PanelPosition::Top:
      edge = Qt::TopEdge;
      break;
    case PanelPosition::Bottom:
      edge = Qt::BottomEdge;
      break;
    case PanelPosition::Left:
      edge = Qt::LeftEdge;
      break;
    case PanelPosition::Right:
      edge = Qt::RightEdge;
      break;
  }
  WindowSystem::setAutoHide(this, edge, on);
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
