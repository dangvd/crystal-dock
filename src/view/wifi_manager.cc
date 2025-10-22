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

#include "wifi_manager.h"

#include <algorithm>

#include <QProcess>

#include "dock_panel.h"

namespace crystaldock {

WifiManager::WifiManager(DockPanel* parent, MultiDockModel* model,
                         Qt::Orientation orientation, int minSize, int maxSize)
    : IconBasedDockItem(parent, model, kLabel, orientation, kIcon,
                        minSize, maxSize),
      connectionDialog_(parent_, this) {
  createMenu();

  connect(&menu_, &QMenu::triggered, this, &WifiManager::onNetworkSelected);

  connect(&menu_, &QMenu::aboutToHide, this,
          [this]() {
            parent_->setShowingPopup(false);
          });
  connect(&contextMenu_, &QMenu::aboutToHide, this,
          [this]() {
            parent_->setShowingPopup(false);
          });

  scanWifiNetworks();
}

WifiManager::~WifiManager() {
  if (process_ && process_->state() != QProcess::NotRunning) {
    process_->kill();
    process_->waitForFinished(1000);
  }
}

void WifiManager::mousePressEvent(QMouseEvent* e) {
  if (e->button() == Qt::LeftButton) {
    if (commandExists({kCommand}).isEmpty()) {
      QMessageBox::warning(parent_, "Command not found",
          QString("Command '") + kCommand + "' not found. This is required by the "
          + kLabel + " component.");
      return;
    }
    showWifiNetworks();
  } else if (e->button() == Qt::RightButton) {
    showPopupMenu(&contextMenu_);
  }
}

void WifiManager::onNetworkSelected(QAction* action) {
  WifiNetwork network = action->data().value<WifiNetwork>();
  connectionDialog_.setData(network);
  connectionDialog_.show();
}

void WifiManager::rescan() {
  info_.setText("Rescanning Wi-Fi networks...");
  parent_->minimize();
  QTimer::singleShot(DockPanel::kExecutionDelayMs, [this]{
    info_.show();
  });
  scanWifiNetworks([this]() {
    info_.setText("Rescanning completed");
  });
}

void WifiManager::connectWifi(const QString &network, const QString &password) {
  if (process_ && process_->state() != QProcess::NotRunning) {
    return;
  }

  process_ = new QProcess(parent_);
  connect(process_, &QProcess::started,
          [this, network, password]() {
            process_->write((password + "\n").toStdString().c_str());
            connect(process_, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                    [this, network](int exitCode, QProcess::ExitStatus exitStatus) {
                      if (exitCode == 0) {
                        connectionDialog_.setInUse(true);
                        setLabel("Wi-Fi: Connected to " + network);
                        if (auto result = std::ranges::find(networks_, network, &WifiNetwork::name);
                            result != networks_.end()) {
                          result->inUse = true;
                          updateWifiList();
                        }
                      } else {
                        connectionDialog_.setStatus("Failed to connect");
                      }
                      process_->deleteLater();
                      process_ = nullptr;
                    });
          });

  process_->start(kCommand, {"dev", "wifi", "connect", network, "--ask"});
}

void WifiManager::disconnectWifi(const QString &network) {
  if (process_ && process_->state() != QProcess::NotRunning) {
    return;
  }

  process_ = new QProcess(parent_);
  connect(process_, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
          [this, network](int exitCode, QProcess::ExitStatus exitStatus) {
            if (exitCode == 0) {
              connectionDialog_.setInUse(false);
              setLabel("Wi-Fi: Not connected");
              if (auto result = std::ranges::find(networks_, network, &WifiNetwork::name);
                  result != networks_.end()) {
                result->inUse = false;
                updateWifiList();
              }
            }
            process_->deleteLater();
            process_ = nullptr;
          });

  process_->start(kCommand, {"connection", "delete", network});
}

void WifiManager::scanWifiNetworks(std::function<void()> onSuccess) {
  if (process_ && process_->state() != QProcess::NotRunning) {
    return;
  }

  process_ = new QProcess(parent_);
  connect(process_, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
          [this, onSuccess](int exitCode, QProcess::ExitStatus exitStatus) {
            if (exitCode == 0) {
              networks_.clear();
              bool connected = false;
              for (const QString& network : process_->readAllStandardOutput().split('\n')) {
                QStringList fields = network.split(':');
                if (fields.size() < 3) continue;
                const QString& name = fields[0];
                unsigned int signal = fields[1].toInt();
                bool inUse = !fields[2].trimmed().isEmpty();
                if (!name.isEmpty() && signal != 0) {
                  WifiNetwork network = {
                    .name = name,
                    .signal = signal,
                    .inUse = inUse
                  };
                  networks_.push_back(network);
                  if (network.inUse) {
                    setLabel("Wi-Fi: Connected to " + network.name);
                    connected = true;
                  }
                }
              }
              if (!connected) {
                setLabel("Wi-Fi: Not connected");
              }
              updateWifiList();
              if (onSuccess) {
                onSuccess();
              }
            }
            process_->deleteLater();
            process_ = nullptr;
          });

  process_->start(kCommand, {"--terse", "--fields", "SSID,SIGNAL,IN-USE", "dev", "wifi", "list"});
}

void WifiManager::showWifiNetworks() {
  showPopupMenu(&menu_);
}

void WifiManager::updateWifiList() {
  menu_.clear();
  for (const auto& network : networks_) {
    QString label = network.name + (network.inUse ? " (Connected)" : "");
    QAction* action = new QAction(label, &menu_);
    action->setData(QVariant::fromValue(network));
    menu_.addAction(action);
  }
}

void WifiManager::createMenu() {
  contextMenu_.addSection(kLabel);
  rescanAction_ = contextMenu_.addAction(
      QIcon::fromTheme("network-wireless"), "Rescan Wi-Fi networks");
  connect(rescanAction_, &QAction::triggered, this, &WifiManager::rescan);

  contextMenu_.addSeparator();
  parent_->addPanelSettings(&contextMenu_);
}

}  // namespace crystaldock
