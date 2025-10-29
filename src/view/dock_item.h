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

#ifndef CRYSTALDOCK_DOCK_ITEM_H_
#define CRYSTALDOCK_DOCK_ITEM_H_

#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QString>
#include <Qt>

#include <display/window_system.h>
#include <model/multi_dock_model.h>

namespace crystaldock {

class DockPanel;

// Base class for all dock items, e.g. launchers and pager icons.
//
// It's a design decision that DockItem is not a sub-class of QWidget, to make
// the dock's parabolic zooming effect smoother.
class DockItem {
 public:
  DockItem(DockPanel* parent, MultiDockModel* model, const QString& label,
      Qt::Orientation orientation, int minSize, int maxSize)
      : parent_(parent), model_(model), label_(label), orientation_(orientation),
        minSize_(minSize), maxSize_(maxSize), size_(minSize) {}
  virtual ~DockItem() {}

  // Gets the width of the item given a size.
  virtual int getWidthForSize(int size) const = 0;

  // Gets the height of the item given a size.
  virtual int getHeightForSize(int size) const = 0;

  // Draws itself on the parent's canvas.
  virtual void draw(QPainter* painter) const = 0;

  // Mouse press event handler.
  virtual void mousePressEvent(QMouseEvent* e) = 0;

  // We manually reset active window on the dock's mouse event.
  // We don't want to always do this (e.g. handle this in window_system::state_change() handler)
  // because otherwise we wouldn't be able to click on an active window's icon to minimize it
  // (the click action would change the active window to be the dock).
  virtual void maybeResetActiveWindow(QMouseEvent* e) {
    WindowSystem::resetActiveWindow();
  }

  // Some dock items (e.g. Application Menu or Clock) have their own global
  // (i.e. not dock-specific) config that they need to reload when the config
  // has been changed by another dock (not their parent dock).
  virtual void loadConfig() {}

  // Handles adding the task, e.g. for a Program dock item.
  virtual bool addTask(const WindowInfo* task) { return false; }

  // Handles updating the task, e.g. for a Program dock item.
  virtual bool updateTask(const WindowInfo* task) { return false; }

  // Handles removing the task, e.g. for a Program dock item.
  virtual bool removeTask(void* window) { return false; }

  // Does this (Program) dock item already have this task?
  virtual bool hasTask(void* window) { return false; }

  // Will this item be ordered before the Program item for this task?
  virtual bool beforeTask(const QString& program) { return true; }

  // Should be removed for example if a Program item has no task and is not pinned.
  virtual bool shouldBeRemoved() { return false; }

  // This is virtual so dynamic label can be implemented in its subclasses.
  virtual QString getLabel() const { return label_; }
  void setLabel(const QString& label) { label_ = label; }

  // For a Program dock item.
  virtual QString getAppId() const { return ""; }

  // For a Program dock item.
  virtual QString getAppLabel() const { return ""; }

  // For a Program dock item.
  virtual void updatePinnedStatus(bool pinned) {}

  // For a Program dock item.
  virtual void setDemandsAttention(bool demandsAttention) {}

  // Visual feedback functionality
  virtual void setHovered(bool hovered) { isHovered_ = hovered; }
  virtual bool isHovered() const { return isHovered_; }
  virtual void setPressed(bool pressed) { isPressed_ = pressed; }
  virtual bool isPressed() const { return isPressed_; }

  bool isHorizontal() const { return orientation_ == Qt::Horizontal; }

  void setAnimationStartAsCurrent() {
    startLeft_ = left_;
    startTop_ = top_;
    startSize_ = size_;
  }

  void setAnimationEndAsCurrent() {
    endLeft_ = left_;
    endTop_ = top_;
    endSize_ = size_;
  }

  void startAnimation(int numSteps) {
    left_ = startLeft_;
    top_ = startTop_;
    size_ = startSize_;
    currentStep_ = 0;
    numSteps_ = numSteps;
  }

  void nextAnimationStep() {
    ++currentStep_;
    if (currentStep_ <= numSteps_) {
      left_ = startLeft_ + (endLeft_ - startLeft_) * currentStep_ / numSteps_;
      top_ = startTop_ + (endTop_ - startTop_) * currentStep_ / numSteps_;
      size_ = startSize_ + (endSize_ - startSize_) * currentStep_ / numSteps_;
    }
  }

  // Gets max width, i.e. the width when the item is max zoomed.
  int getMaxWidth() const {
    return getWidthForSize(maxSize_);
  }

  // Gets max height, i.e. the width when the item is max zoomed.
  int getMaxHeight() const {
    return getHeightForSize(maxSize_);
  }

  // Gets min width, i.e. when the item is not zoomed in.
  int getMinWidth() const {
    return getWidthForSize(minSize_);
  }

  // Gets min height, i.e. when the item is not zoomed in.
  int getMinHeight() const {
    return getHeightForSize(minSize_);
  }

  int getWidth() const {
    return getWidthForSize(size_);
  }
  
  int getHeight() const {
    return getHeightForSize(size_);
  }

 protected:
  void showPopupMenu(QMenu* menu);

  DockPanel* parent_;
  MultiDockModel* model_;
  QString label_; // Label of the dock item.
  Qt::Orientation orientation_; // Orientation (horizontal/vertical).
  int minSize_;
  int maxSize_;

  int size_;
  int left_;
  int top_;
  // Center when minimized, as x or y depends on whether the orientation is
  // horizontal or vertical. This is used when calculating the size of the item
  // when the dock is in parabolic zoom.
  int minCenter_; 

  // For animation.
  int startLeft_;
  int startTop_;
  int startSize_;
  int endLeft_;
  int endTop_;
  int endSize_;
  int currentStep_;
  int numSteps_;

  // Visual feedback states
  bool isHovered_ = false;
  bool isPressed_ = false;

 private:
  friend class DockPanel;
};

}  // namespace crystaldock

#endif  // CRYSTALDOCK_DOCK_ITEM_H_
