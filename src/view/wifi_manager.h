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

#ifndef CRYSTAL_DOCK_WIFI_MANAGER_H_
#define CRYSTAL_DOCK_WIFI_MANAGER_H_

#include "icon_based_dock_item.h"

#include <QAction>
#include <QMenu>
#include <QMouseEvent>
#include <QProcess>
#include <QObject>
#include <QString>

#include "view/wifi_connection_dialog.h"

namespace crystaldock {

struct WifiNetwork {
  QString name;
  unsigned int signal;
  bool inUse;
};

// A Wifi manager that integrates with nmcli.
class WifiManager : public QObject, public IconBasedDockItem {
  Q_OBJECT

 public:
  WifiManager(DockPanel* parent, MultiDockModel* model, Qt::Orientation orientation,
              int minSize, int maxSize);
  virtual ~WifiManager();

  void mousePressEvent(QMouseEvent* e) override;
  bool beforeTask(const QString& program) override { return false; }

  void connectWifi(const QString& network, const QString& password);
  void disconnectWifi(const QString& network);

 public slots:
  void onNetworkSelected(QAction* action);

 private:
  static constexpr char kCommand[] = "nmcli";
  static constexpr char kLabel[] = "Wi-Fi Manager";
  static constexpr char kIcon[] = "network-wireless";

  void initWifiNetworks();
  void showWifiNetworks();

  void updateWifiList();

  // Creates the context menu.
  void createMenu();

  std::vector<WifiNetwork> networks_;
  // nmcli process.
  QProcess* process_ = nullptr;

  // Left-click volume menu.
  QMenu menu_;
  // Right-click context menu.
  QMenu contextMenu_;

  WifiConnectionDialog connectionDialog_;
};

}  // namespace crystaldock

Q_DECLARE_METATYPE(crystaldock::WifiNetwork);

#endif  // CRYSTAL_DOCK_WIFI_MANAGER_H_
