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
    process_(nullptr) {
  initKeyboardLayouts();

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
                   0, activeKeyboardLayout_.languageCode, 2, Qt::black, Qt::white, painter);
}

void KeyboardLayout::mousePressEvent(QMouseEvent* e) {
  if (e->button() == Qt::LeftButton) {
    if (commandExists({kCommand}).isEmpty()) {
      QMessageBox::warning(parent_, "Command not found",
                           QString("Command '") + kCommand + "' not found. This is required by the "
                               + kLabel + " component.");
      return;
    } else if (!ibusReady_) {
      QMessageBox::warning(parent_, "IBus is not running",
                           "Please make sure the IBus daemon is running.");
      return;
    }
    showPopupMenu(&menu_);
  } else if (e->button() == Qt::RightButton) {
    showPopupMenu(&contextMenu_);
  }
}

QString KeyboardLayout::getLabel() const {
  return activeKeyboardLayout_.isEmpty()
      ? kLabel
      : QString(kLabel) + ": " + activeKeyboardLayout_.toString();
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

void KeyboardLayout::initKeyboardLayouts() {
  if (process_ && process_->state() != QProcess::NotRunning) {
    return;
  }

  process_ = new QProcess(parent_);
  connect(process_, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
          [this](int exitCode, QProcess::ExitStatus exitStatus) {
            if (exitCode == 0) {
              ibusReady_ = true;
              QRegularExpression languageRe(R"(language:\s+(.+))");
              QRegularExpression keyboardRe(R"(\s*(.+)\s+-\s+(.+))");
              QString language;
              for (const QString& line : process_->readAllStandardOutput().split('\n')) {
                QRegularExpressionMatch match = languageRe.match(line);
                if (match.hasMatch()) {
                  language = match.captured(1).trimmed();
                } else {
                  match = keyboardRe.match(line);
                  if (match.hasMatch()) {
                    QString engine = match.captured(1).trimmed();
                    QString description = match.captured(2).trimmed();
                    if (!language.isEmpty()) {
                      if (keyboardLayouts_.count(language) == 0) {
                        keyboardLayouts_[language] = std::vector<KeyboardLayoutInfo>();
                      }
                      keyboardLayouts_[language].push_back(
                          KeyboardLayoutInfo(language, engine, description));
                      keyboardEngines_[engine] = keyboardLayouts_[language].back();
                    }
                  }
                }
              }

              parent_->editKeyboardLayoutsDialog_.setData(keyboardLayouts_);
              QString activeLayout = model_->activeKeyboardLayout();
              if (!activeLayout.isEmpty() && keyboardEngines_.count(activeLayout) > 0) {
                initUserKeyboardLayouts(activeLayout);
              } else {
                // Gets the currently active keyboard layout.
                QProcess* process = new QProcess(parent_);
                connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                    [this, process](int exitCode, QProcess::ExitStatus exitStatus) {
                      if (exitCode == 0) {
                        const QString ibusActiveLayout = process->readAllStandardOutput().trimmed();
                        if (!ibusActiveLayout.isEmpty() && keyboardEngines_.count(ibusActiveLayout) > 0) {
                          initUserKeyboardLayouts(ibusActiveLayout);
                          model_->setActiveKeyboardLayout(ibusActiveLayout);
                        }
                      }
                      process->deleteLater();
                    });
                process->start(kCommand, QStringList() << "engine");
              }
            }
            process_->deleteLater();
            process_ = nullptr;
          });

  process_->start(kCommand, QStringList() << "list-engine");
}

void KeyboardLayout::initUserKeyboardLayouts(const QString& activeLayout) {
  activeKeyboardLayout_ = keyboardEngines_[activeLayout];
  QStringList userLayouts = model_->userKeyboardLayouts();
  if (userLayouts.isEmpty()) {
    model_->setUserKeyboardLayouts(QStringList() << activeLayout);
  }
  if (!userLayouts.contains(activeLayout)) {
    userLayouts << activeLayout;
  }
  for (const auto& layout : userLayouts) {
    if (keyboardEngines_.count(layout) > 0) {
      userKeyboardLayouts_.push_back(keyboardEngines_[layout]);
    }
  }
  createMenu();
}

void KeyboardLayout::createMenu() {
  // Left-click menu.
  for (const auto& layout : userKeyboardLayouts_) {
    QAction* action = new QAction(layout.toString(), &menu_);
    action->setData(QVariant::fromValue(layout));
    menu_.addAction(action);
  }

  // Right-click context menu.
  contextMenu_.addSection(kLabel);
  contextMenu_.addAction(QIcon::fromTheme("configure"), QString("&Edit Keyboard Layouts"), parent_,
                  [this] {
                    parent_->minimize();
                    QTimer::singleShot(DockPanel::kExecutionDelayMs, [this]{
                      parent_->showEditKeyboardLayoutsDialog();
                    });
                  });

  contextMenu_.addSeparator();
  parent_->addPanelSettings(&contextMenu_);
}

void KeyboardLayout::updateUi() {
  parent_->update();
}

}  // namespace crystaldock
