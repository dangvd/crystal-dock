#ifndef CRYSTAL_DOCK_EDIT_KEYBOARD_LAYOUTS_DIALOG_H_
#define CRYSTAL_DOCK_EDIT_KEYBOARD_LAYOUTS_DIALOG_H_

#include <map>
#include <vector>

#include <QDialog>

namespace Ui {
class EditKeyboardLayoutsDialog;
}

namespace crystaldock {

class MultiDockModel;

struct KeyboardLayoutInfo;

class EditKeyboardLayoutsDialog : public QDialog {
  Q_OBJECT

 public:
  explicit EditKeyboardLayoutsDialog(QWidget* parent, MultiDockModel* model);
  ~EditKeyboardLayoutsDialog();

  void setKeyboardLayouts(
      const std::map<QString, std::vector<KeyboardLayoutInfo>>& keyboardLayouts,
      const std::map<QString, KeyboardLayoutInfo>& keyboardEngines);
  void refreshData();

 public slots:
  void onLanguageChanged(const QString&);
  void onAddButtonClicked();
  void onRemoveButtonClicked();
  void saveData();

 private:
  Ui::EditKeyboardLayoutsDialog *ui;
  MultiDockModel* model_;

  // TODO: It's not great to duplicate this here (they are also in KeyboardLayout).
  // Consider moving them to the model.
  // All the available keyboard layouts, as map from languages to list of structs.
  std::map<QString, std::vector<KeyboardLayoutInfo>> keyboardLayouts_;
  // All the available keyboard layouts, as map from engines to structs.
  std::map<QString, KeyboardLayoutInfo> keyboardEngines_;
};

}  // namespace crystaldock

#endif  // CRYSTAL_DOCK_EDIT_KEYBOARD_LAYOUTS_DIALOG_H_
