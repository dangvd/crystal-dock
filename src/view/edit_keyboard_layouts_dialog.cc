#include "edit_keyboard_layouts_dialog.h"
#include "ui_edit_keyboard_layouts_dialog.h"

#include "keyboard_layout.h"

namespace crystaldock {

EditKeyboardLayoutsDialog::EditKeyboardLayoutsDialog(QWidget *parent)
    : QDialog(parent),
      ui(new Ui::EditKeyboardLayoutsDialog) {
  ui->setupUi(this);
}

EditKeyboardLayoutsDialog::~EditKeyboardLayoutsDialog() {
  delete ui;
}

void EditKeyboardLayoutsDialog::setData(
    const std::map<QString, std::vector<KeyboardLayoutInfo>>& keyboardLayouts) {
  ui->languages->clear();
  ui->languageKeyboardLayouts->clear();
  for (const auto& pair : keyboardLayouts) {
    ui->languages->addItem(pair.first);
    for (const auto& layout : pair.second) {
      ui->languageKeyboardLayouts->addItem(layout.toString());
    }
  }
}

}  // namespace crystaldock
