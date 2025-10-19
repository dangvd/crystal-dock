#ifndef CRYSTAL_DOCK_WIFI_CONNECTION_DIALOG_H_
#define CRYSTAL_DOCK_WIFI_CONNECTION_DIALOG_H_

#include <QDialog>

namespace Ui {
class WifiConnectionDialog;
}

namespace crystaldock {

struct WifiNetwork;
class WifiManager;

class WifiConnectionDialog : public QDialog {
    Q_OBJECT

 public:
  explicit WifiConnectionDialog(QWidget* parent, WifiManager* manager);
  ~WifiConnectionDialog();

  void setData(const WifiNetwork& network);
  void setInUse(bool inUse);
  void setStatus(const QString& status, bool enableButtons = true);

 public slots:
  void connectWifi();
  void disconnectWifi();

 private:
  Ui::WifiConnectionDialog *ui;

  WifiManager* manager_;
};

}  // namespace crystaldock

#endif // CRYSTAL_DOCK_WIFI_CONNECTION_DIALOG_H_
