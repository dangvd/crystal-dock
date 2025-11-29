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

#ifndef CRYSTAL_DOCK_KEYBOARD_LAYOUT_H_
#define CRYSTAL_DOCK_KEYBOARD_LAYOUT_H_

#include "icon_based_dock_item.h"

#include <map>
#include <vector>

#include <QAction>
#include <QMenu>
#include <QMouseEvent>
#include <QProcess>
#include <QString>

namespace crystaldock {

struct KeyboardLayoutInfo {
  QString language;
  QString languageCode;
  QString engine;
  QString description;

  KeyboardLayoutInfo() = default;

  KeyboardLayoutInfo(const QString& language2, const QString& engine2, const QString& description2)
      : language(language2), engine(engine2), description(description2) {
    if (language.size() >= 2) {
      languageCode = language.first(2).toUpper();
    }
  }

  bool isEmpty() const { return engine.isEmpty(); }

  QString toString() const { return language + " - " + description; }
};

// A keyboard layout manager that integrates with IBus.
class KeyboardLayout : public QObject, public IconBasedDockItem {
  Q_OBJECT

 public:
  KeyboardLayout(DockPanel* parent, MultiDockModel* model, Qt::Orientation orientation,
                 int minSize, int maxSize);
  virtual ~KeyboardLayout();

  void draw(QPainter* painter) const override;
  void mousePressEvent(QMouseEvent* e) override;
  QString getLabel() const override;
  bool beforeTask(const QString& program) override { return false; }

 public slots:
  void onKeyboardLayoutSelected(QAction* action);
  void setKeyboardLayout(const KeyboardLayoutInfo& layout);

 private:
  static constexpr char kCommand[] = "ibus";
  static constexpr char kLabel[] = "Keyboard Layout";
  static constexpr char kIcon[] = "input-keyboard";
  static constexpr int kUpdateInterval = 1000;  // 1 second.

  void initKeyboardLayouts();
  void initUserKeyboardLayouts(const QString& activeLayout);

  // Creates the context menu.
  void createMenu();

  // All the available keyboard layouts, as map from languages to list of structs.
  std::map<QString, std::vector<KeyboardLayoutInfo>> keyboardLayouts_;
  // All the available keyboard layouts, as map from engines to structs.
  std::map<QString, KeyboardLayoutInfo> keyboardEngines_;
  // The user-selected keyboard layouts for quick switching.
  std::vector<KeyboardLayoutInfo> userKeyboardLayouts_;
  // The active keyboard layout.
  KeyboardLayoutInfo activeKeyboardLayout_;

  bool ibusReady_ = false;

  // ibus process.
  QProcess* process_ = nullptr;

  // Left-click volume menu.
  QMenu menu_;
  // Right-click context menu.
  QMenu contextMenu_;
};

}  // namespace crystaldock

Q_DECLARE_METATYPE(crystaldock::KeyboardLayoutInfo);

#endif  // CRYSTAL_DOCK_KEYBOARD_LAYOUT_H_
