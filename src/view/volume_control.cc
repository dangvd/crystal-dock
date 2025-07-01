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

#include "volume_control.h"

#include <QApplication>
#include <QColor>
#include <QIcon>
#include <QPainter>
#include <QPainterPath>
#include <QRegularExpression>
#include <QWidgetAction>
#include <QtMath>

#include "dock_panel.h"
#include <utils/draw_utils.h>

namespace crystaldock {

constexpr float VolumeControl::kWhRatio;
constexpr int VolumeControl::kUpdateInterval;

VolumeControl::VolumeControl(DockPanel* parent, MultiDockModel* model,
                            Qt::Orientation orientation, int minSize, int maxSize)
    : IconlessDockItem(parent, model, "Volume Control", orientation, minSize, maxSize,
                       kWhRatio),
      updateTimer_(new QTimer(this)),
      volumeProcess_(nullptr) {
  createMenu();

  // Set up update timer
  connect(updateTimer_, &QTimer::timeout, this, &VolumeControl::refreshVolumeInfo);
  updateTimer_->start(kUpdateInterval);

  // Initial volume info refresh
  QTimer::singleShot(1000, this, &VolumeControl::refreshVolumeInfo);

  connect(&menu_, &QMenu::aboutToHide, this,
          [this]() {
            parent_->setShowingPopup(false);
          });
  connect(&contextMenu_, &QMenu::aboutToHide, this,
          [this]() {
            parent_->setShowingPopup(false);
          });
}

VolumeControl::~VolumeControl() {
  if (volumeProcess_ && volumeProcess_->state() != QProcess::NotRunning) {
    volumeProcess_->kill();
    volumeProcess_->waitForFinished(1000);
  }
}

void VolumeControl::draw(QPainter* painter) const {
  const auto x = left_;
  const auto y = top_;
  const auto w = getWidth();
  const auto h = getHeight();

  // Try to use system icons first
  QString iconName;
  if (isMuted_ || currentVolume_ == 0) {
    iconName = "audio-volume-muted";
  } else if (currentVolume_ < 30) {
    iconName = "audio-volume-low";
  } else if (currentVolume_ <= 70) {
    iconName = "audio-volume-medium";
  } else {
    iconName = "audio-volume-high";
  }

  QIcon icon = QIcon::fromTheme(iconName);
  if (!icon.isNull()) {
    const int iconSize = qMin(w, h) * 0.8;
    const int iconX = x + (w - iconSize) / 2;
    const int iconY = y + (h - iconSize) / 2;
    icon.paint(painter, iconX, iconY, iconSize, iconSize);
  } else {
    // Fallback: draw custom speaker icon
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(QPen(Qt::white, 2));
    painter->setBrush(Qt::white);

    const int centerX = x + w / 2;
    const int centerY = y + h / 2;
    const int speakerSize = qMin(w, h) * 0.4;

    // Draw speaker cone
    QRect speakerRect(centerX - speakerSize / 2, centerY - speakerSize / 3,
                      speakerSize / 2, speakerSize * 2 / 3);
    painter->fillRect(speakerRect, Qt::white);

    // Draw speaker driver
    QPolygon driver;
    driver << QPoint(centerX, centerY - speakerSize / 3)
           << QPoint(centerX + speakerSize / 2, centerY - speakerSize / 6)
           << QPoint(centerX + speakerSize / 2, centerY + speakerSize / 6)
           << QPoint(centerX, centerY + speakerSize / 3);
    painter->drawPolygon(driver);

    // Draw volume level arcs if not muted
    if (!isMuted_ && currentVolume_ > 0) {
      painter->setBrush(Qt::NoBrush);
      const int arcStartX = centerX + speakerSize / 2 + 4;
      const int numArcs = currentVolume_ > 70 ? 3 : currentVolume_ > 30 ? 2 : 1;

      for (int i = 0; i < numArcs; ++i) {
        const int arcRadius = speakerSize / 4 + i * speakerSize / 8;
        const QRect arcRect(arcStartX - arcRadius, centerY - arcRadius,
                           arcRadius * 2, arcRadius * 2);
        painter->drawArc(arcRect, -30 * 16, 60 * 16);
      }
    }

    // Draw mute X if muted
    if (isMuted_) {
      painter->setPen(QPen(Qt::red, 3));
      const int crossSize = speakerSize / 2;
      painter->drawLine(centerX - crossSize / 2, centerY - crossSize / 2,
                       centerX + crossSize / 2, centerY + crossSize / 2);
      painter->drawLine(centerX + crossSize / 2, centerY - crossSize / 2,
                       centerX - crossSize / 2, centerY + crossSize / 2);
    }
  }
}

void VolumeControl::mousePressEvent(QMouseEvent* e) {
  if (e->button() == Qt::LeftButton) {
    showPopupMenu(&menu_);
  } else if (e->button() == Qt::MiddleButton) {
    toggleMute();
  } else if (e->button() == Qt::RightButton) {
    showPopupMenu(&contextMenu_);
  }
}

void VolumeControl::wheelEvent(QWheelEvent* e) {
  if (e->angleDelta().y() == 0) {
    return;
  }

  const int delta = e->angleDelta().y();
  const int scrollStep = model_->volumeScrollStep();
  const int volumeChange = (delta > 0) ? scrollStep : -scrollStep;
  const int newVolume = qBound(0, currentVolume_ + volumeChange, 100);

  // Update slider position without triggering signals
  volumeSlider_->blockSignals(true);
  volumeSlider_->setValue(newVolume);
  volumeSlider_->blockSignals(false);

  // Update internal state immediately for responsive UI
  currentVolume_ = newVolume;
  setVolume(newVolume);

  // Trigger repaint
  parent_->update();
}

QString VolumeControl::getLabel() const {
  return QString("Volume: %1%").arg(currentVolume_);
}

void VolumeControl::refreshVolumeInfo() {
  // Prevent concurrent processes
  if (volumeProcess_ && volumeProcess_->state() != QProcess::NotRunning) {
    return;
  }

  // Get volume level
  volumeProcess_ = new QProcess(parent_);
  connect(volumeProcess_, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
          [this](int exitCode, QProcess::ExitStatus exitStatus) {
            if (exitCode == 0) {
              const QString output = volumeProcess_->readAllStandardOutput();
              QRegularExpression volumeRegex(R"((\d+)%)");
              QRegularExpressionMatch match = volumeRegex.match(output);
              if (match.hasMatch()) {
                const int newVolume = match.captured(1).toInt();
                if (newVolume != currentVolume_) {
                  currentVolume_ = newVolume;
                  volumeSlider_->blockSignals(true);
                  volumeSlider_->setValue(currentVolume_);
                  volumeSlider_->blockSignals(false);
                  parent_->update();
                }
              }
            }
            volumeProcess_->deleteLater();
            volumeProcess_ = nullptr;

            // Check mute status
            QProcess* muteProcess = new QProcess(parent_);
            connect(muteProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                    [this, muteProcess](int exitCode, QProcess::ExitStatus exitStatus) {
                      if (exitCode == 0) {
                        const QString output = muteProcess->readAllStandardOutput().trimmed();
                        const bool newMuted = (output.toLower().contains("yes"));
                        if (newMuted != isMuted_) {
                          isMuted_ = newMuted;
                          muteAction_->setChecked(isMuted_);
                          parent_->update();
                        }
                      }
                      muteProcess->deleteLater();
                    });
            muteProcess->start("pactl", QStringList() << "get-sink-mute" << "@DEFAULT_SINK@");
          });

  volumeProcess_->start("pactl", QStringList() << "get-sink-volume" << "@DEFAULT_SINK@");
}

void VolumeControl::setVolume(int volume) {
  QProcess* process = new QProcess(parent_);
  connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
          [process](int exitCode, QProcess::ExitStatus exitStatus) {
            process->deleteLater();
          });
  process->start("pactl", QStringList() << "set-sink-volume" << "@DEFAULT_SINK@" << QString("%1%").arg(volume));
}

void VolumeControl::onVolumeSliderChanged(int value) {
  currentVolume_ = value;
  setVolume(value);
  parent_->update();
}

void VolumeControl::toggleMute() {
  QProcess* process = new QProcess(parent_);
  connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
          [process](int exitCode, QProcess::ExitStatus exitStatus) {
            process->deleteLater();
          });
  process->start("pactl", QStringList() << "set-sink-mute" << "@DEFAULT_SINK@" << "toggle");
}

void VolumeControl::createMenu() {
  // Volume menu

  // Volume slider
  volumeSlider_ = new QSlider(Qt::Horizontal);
  volumeSlider_->setRange(0, 100);
  volumeSlider_->setValue(currentVolume_);
  volumeSlider_->setMinimumWidth(getMaxWidth());
  connect(volumeSlider_, &QSlider::valueChanged, this, &VolumeControl::onVolumeSliderChanged);

  QWidgetAction* sliderAction = new QWidgetAction(&menu_);
  sliderAction->setDefaultWidget(volumeSlider_);
  menu_.addAction(sliderAction);

  // Mute toggle
  muteAction_ = menu_.addAction("Mute", this, &VolumeControl::toggleMute);
  muteAction_->setCheckable(true);

  // Context menu

  // Scroll step submenu
  scrollStepMenu_ = contextMenu_.addMenu("Scroll Step");
  scrollStepGroup_ = new QActionGroup(this);

  scrollStep1Action_ = scrollStepMenu_->addAction("1% (Fine)", this, &VolumeControl::setVolumeScrollStep1);
  scrollStep1Action_->setCheckable(true);
  scrollStep1Action_->setActionGroup(scrollStepGroup_);

  scrollStep2Action_ = scrollStepMenu_->addAction("2% (Default)", this, &VolumeControl::setVolumeScrollStep2);
  scrollStep2Action_->setCheckable(true);
  scrollStep2Action_->setActionGroup(scrollStepGroup_);
  scrollStep2Action_->setChecked(true);

  scrollStep5Action_ = scrollStepMenu_->addAction("5% (Coarse)", this, &VolumeControl::setVolumeScrollStep5);
  scrollStep5Action_->setCheckable(true);
  scrollStep5Action_->setActionGroup(scrollStepGroup_);

  scrollStep10Action_ = scrollStepMenu_->addAction("10% (Very Coarse)", this, &VolumeControl::setVolumeScrollStep10);
  scrollStep10Action_->setCheckable(true);
  scrollStep10Action_->setActionGroup(scrollStepGroup_);

  // Audio settings
  audioSettingsAction_ = contextMenu_.addAction(QIcon::fromTheme("configure"), "Audio Settings",
                                                this, &VolumeControl::openAudioSettings);

  contextMenu_.addSeparator();
  parent_->addPanelSettings(&contextMenu_);
}

void VolumeControl::setVolumeScrollStep1() {
  model_->setVolumeScrollStep(1);
  model_->saveAppearanceConfig(true);
}

void VolumeControl::setVolumeScrollStep2() {
  model_->setVolumeScrollStep(2);
  model_->saveAppearanceConfig(true);
}

void VolumeControl::setVolumeScrollStep5() {
  model_->setVolumeScrollStep(5);
  model_->saveAppearanceConfig(true);
}

void VolumeControl::setVolumeScrollStep10() {
  model_->setVolumeScrollStep(10);
  model_->saveAppearanceConfig(true);
}

void VolumeControl::openAudioSettings() {
  QProcess::startDetached("pavucontrol");
}

}  // namespace crystaldock
