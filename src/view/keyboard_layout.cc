/*
 * This file is part of Crystal Dock.
 * Copyright (C) 2025 Viet Dang (dangvd@gmail.com)
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

#include "keyboard_layout.h"

#include <iostream>

#include <QMessageBox>

#include "dock_panel.h"

#include <utils/command_utils.h>
#include <utils/draw_utils.h>

namespace crystaldock {

KeyboardLayout::KeyboardLayout(DockPanel* parent, MultiDockModel* model,
                               Qt::Orientation orientation, int minSize, int maxSize)
    : IconBasedDockItem(parent, model, kLabel, orientation, kIcon,
                        minSize, maxSize),
    process_(nullptr),
    keyboardLayouts_{
      {"English", "GB", "xkb:gb::eng", "English (UK)"},
      {"Vietnamese", "VI", "m17n:vi:telex", "vi-telex (m17n)"}},
    activeKeyboardLayout_({"English", "GB", "xkb:gb::eng", "English (UK)"}) {
  createMenu();

  connect(&menu_, &QMenu::triggered, this, &KeyboardLayout::onKeyboardLayoutSelected);

  connect(&menu_, &QMenu::aboutToHide, this,
          [this]() {
            parent_->setShowingPopup(false);
          });
  connect(&contextMenu_, &QMenu::aboutToHide, this,
          [this]() {
            parent_->setShowingPopup(false);
          });
}

KeyboardLayout::~KeyboardLayout() {
  if (process_ && process_->state() != QProcess::NotRunning) {
    process_->kill();
    process_->waitForFinished(1000);
  }
}

void KeyboardLayout::draw(QPainter* painter) const {
  IconBasedDockItem::draw(painter);

  QFont font;
  font.setPixelSize(getHeight() / 2);
  painter->setFont(font);
  drawBorderedText(left_ + getWidth() / 4, top_ + getHeight()  * 3 / 8,
                   getWidth() * 3 / 4, getHeight() * 5 / 8,
                   0, activeKeyboardLayout_.countryCode, 2, Qt::black, Qt::white, painter);
}

void KeyboardLayout::mousePressEvent(QMouseEvent* e) {
  if (e->button() == Qt::LeftButton) {
    if (commandExists({kCommand}).isEmpty()) {
      QMessageBox::warning(parent_, "Command not found",
                           QString("Command '") + kCommand + "' not found. This is required by the "
                               + kLabel + " component.");
      return;
    }
    showPopupMenu(&menu_);
  } else if (e->button() == Qt::RightButton) {
    showPopupMenu(&contextMenu_);
  }
}

QString KeyboardLayout::getLabel() const {
  return QString(kLabel) + ": " + activeKeyboardLayout_.toString();
}

void KeyboardLayout::onKeyboardLayoutSelected(QAction* action) {
  KeyboardLayoutInfo layout = action->data().value<KeyboardLayoutInfo>();
  setKeyboardLayout(layout);
}

void KeyboardLayout::setKeyboardLayout(const KeyboardLayoutInfo& layout) {
  // Prevent concurrent processes
  if (process_ && process_->state() != QProcess::NotRunning) {
    return;
  }

  process_ = new QProcess(parent_);
  connect(process_, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
          [this, layout](int exitCode, QProcess::ExitStatus exitStatus) {
            // Somehow ibus returns 1 even when it succeeded.
            activeKeyboardLayout_ = layout;
            updateUi();
            process_->deleteLater();
            process_ = nullptr;
          });

  process_->start(kCommand, QStringList() << "engine" << layout.engine);
}

void KeyboardLayout::createMenu() {
  // Left-click menu.
  for (const auto& layout : keyboardLayouts_) {
    QAction* action = new QAction(layout.toString(), &menu_);
    action->setData(QVariant::fromValue(layout));
    menu_.addAction(action);
  }

  // Right-click context menu.
  contextMenu_.addSection(kLabel);
  contextMenu_.addSeparator();
  parent_->addPanelSettings(&contextMenu_);
}

void KeyboardLayout::updateUi() {
  parent_->update();
}

}  // namespace crystaldock
