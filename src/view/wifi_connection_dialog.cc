#include "wifi_connection_dialog.h"
#include "ui_wifi_connection_dialog.h"

#include <QDialog>
#include <QIcon>
#include <QPushButton>
#include <QWidget>

#include "wifi_manager.h"

namespace crystaldock {

WifiConnectionDialog::WifiConnectionDialog(QWidget* parent, WifiManager* manager)
    : QDialog(parent),
      ui(new Ui::WifiConnectionDialog),
      manager_(manager) {
  ui->setupUi(this);

  ui->connectButton->setIcon(QIcon::fromTheme("network-wireless"));
  ui->disconnectButton->setIcon(QIcon::fromTheme("network-wireless"));
  ui->closeButton->setIcon(QIcon::fromTheme("dialog-close"));

  connect(ui->connectButton, &QPushButton::clicked,
      this, &WifiConnectionDialog::connectWifi);
  connect(ui->disconnectButton, &QPushButton::clicked,
      this, &WifiConnectionDialog::disconnectWifi);
}

WifiConnectionDialog::~WifiConnectionDialog() {
  delete ui;
}

void WifiConnectionDialog::setData(const WifiNetwork& network) {
  ui->network->setText(network.name);
  ui->signal->setText(QString::number(network.signal) + "%");
  setInUse(network.inUse);
}

void WifiConnectionDialog::setInUse(bool inUse) {
  ui->status->setText(inUse ? "Connected" : "Not connected");
  ui->passwordLabel->setVisible(!inUse);
  ui->password->setVisible(!inUse);
  ui->password->setText("");
  ui->connectButton->setEnabled(true);
  ui->disconnectButton->setEnabled(true);
  ui->connectButton->setVisible(!inUse);
  ui->disconnectButton->setVisible(inUse);
}

void WifiConnectionDialog::setStatus(const QString &status, bool enableButtons) {
  ui->status->setText(status);
  if (enableButtons) {
    ui->connectButton->setEnabled(true);
    ui->disconnectButton->setEnabled(true);
  }
}

void WifiConnectionDialog::connectWifi() {
  ui->connectButton->setEnabled(false);
  setStatus("Connecting...", /*enableButtons=*/false);
  manager_->connectWifi(ui->network->text(), ui->password->text());
}

void WifiConnectionDialog::disconnectWifi() {
  ui->disconnectButton->setEnabled(false);
  setStatus("Disconnecting...", /*enableButtons=*/false);
  manager_->disconnectWifi(ui->network->text());
}

}  // namespace crystaldock
