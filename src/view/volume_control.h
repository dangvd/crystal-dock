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

#ifndef CRYSTALDOCK_VOLUME_CONTROL_H_
#define CRYSTALDOCK_VOLUME_CONTROL_H_

#include "iconless_dock_item.h"

#include <QAction>
#include <QActionGroup>
#include <QMenu>
#include <QObject>
#include <QProcess>
#include <QSlider>
#include <QString>
#include <QTimer>
#include <QWheelEvent>

namespace crystaldock {

// A volume control widget that integrates with PulseAudio.
class VolumeControl : public QObject, public IconlessDockItem {
  Q_OBJECT

 public:
  VolumeControl(DockPanel* parent, MultiDockModel* model, Qt::Orientation orientation,
               int minSize, int maxSize);
  virtual ~VolumeControl();

  void draw(QPainter* painter) const override;
  void mousePressEvent(QMouseEvent* e) override;
  void wheelEvent(QWheelEvent* e);
  QString getLabel() const override;
  bool beforeTask(const QString& program) override { return false; }

 public slots:
  void refreshVolumeInfo();
  void onVolumeSliderChanged(int value);
  void toggleMute();
  void setVolumeScrollStep1();
  void setVolumeScrollStep2();
  void setVolumeScrollStep5();
  void setVolumeScrollStep10();
  void openAudioSettings();

 private:
  static constexpr float kWhRatio = 1.2;
  static constexpr int kUpdateInterval = 1000;

  // Creates the context menu.
  void createMenu();

  // PulseAudio volume control operations.
  void setVolume(int volume);

  // Current volume state.
  int currentVolume_ = 50;
  bool isMuted_ = false;

  // Update timer.
  QTimer* updateTimer_;

  // PulseAudio process for monitoring volume.
  QProcess* volumeProcess_;

  // Context menu.
  QMenu menu_;
  QSlider* volumeSlider_;
  QAction* muteAction_;
  QAction* audioSettingsAction_;

  // Scroll step submenu.
  QMenu* scrollStepMenu_;
  QAction* scrollStep1Action_;
  QAction* scrollStep2Action_;
  QAction* scrollStep5Action_;
  QAction* scrollStep10Action_;
  QActionGroup* scrollStepGroup_;
};

}  // namespace crystaldock

#endif  // CRYSTALDOCK_VOLUME_CONTROL_H_