#ifndef CRYSTAL_DOCK_EDIT_KEYBOARD_LAYOUTS_DIALOG_H_
#define CRYSTAL_DOCK_EDIT_KEYBOARD_LAYOUTS_DIALOG_H_

#include <map>
#include <vector>

#include <QDialog>

namespace Ui {
class EditKeyboardLayoutsDialog;
}

namespace crystaldock {

struct KeyboardLayoutInfo;

class EditKeyboardLayoutsDialog : public QDialog {
  Q_OBJECT

 public:
  explicit EditKeyboardLayoutsDialog(QWidget *parent = nullptr);
  ~EditKeyboardLayoutsDialog();

  void setData(const std::map<QString, std::vector<KeyboardLayoutInfo>>& keyboardLayouts);

 public slots:
  void onLanguageChanged(const QString&);

 private:
  Ui::EditKeyboardLayoutsDialog *ui;

  std::map<QString, std::vector<KeyboardLayoutInfo>> keyboardLayouts_;
};

}  // namespace crystaldock

#endif  // CRYSTAL_DOCK_EDIT_KEYBOARD_LAYOUTS_DIALOG_H_
