#include "edit_keyboard_layouts_dialog.h"
#include "ui_edit_keyboard_layouts_dialog.h"

#include "keyboard_layout.h"
#include <model/multi_dock_model.h>

namespace crystaldock {

EditKeyboardLayoutsDialog::EditKeyboardLayoutsDialog(QWidget *parent, MultiDockModel* model)
    : QDialog(parent),
      ui(new Ui::EditKeyboardLayoutsDialog),
      model_(model) {
  ui->setupUi(this);

  connect(ui->languages, &QListWidget::currentTextChanged,
      this, &EditKeyboardLayoutsDialog::onLanguageChanged);
  connect(ui->addButton, &QPushButton::pressed,
      this, &EditKeyboardLayoutsDialog::onAddButtonClicked);
  connect(ui->removeButton, &QPushButton::pressed,
      this, &EditKeyboardLayoutsDialog::onRemoveButtonClicked);
  connect(ui->buttonBox, &QDialogButtonBox::accepted,
      this, &EditKeyboardLayoutsDialog::saveData);
}

EditKeyboardLayoutsDialog::~EditKeyboardLayoutsDialog() {
  delete ui;
}

void EditKeyboardLayoutsDialog::setKeyboardLayouts(
    const std::map<QString, std::vector<KeyboardLayoutInfo>>& keyboardLayouts,
    const std::map<QString, KeyboardLayoutInfo>& keyboardEngines) {
  keyboardLayouts_ = keyboardLayouts;
  keyboardEngines_ = keyboardEngines;
  ui->languages->clear();
  for (const auto& pair : keyboardLayouts_) {
    ui->languages->addItem(pair.first);
  }
}

void EditKeyboardLayoutsDialog::refreshData() {
  ui->userKeyboardLayouts->clear();
  for (const auto& layout : model_->userKeyboardLayouts()) {
    if (keyboardEngines_.count(layout) > 0) {
      auto* item = new QListWidgetItem;
      item->setText(keyboardEngines_[layout].toString());
      item->setData(Qt::UserRole, QVariant::fromValue(keyboardEngines_[layout]));
      ui->userKeyboardLayouts->addItem(item);
    }
  }
}

void EditKeyboardLayoutsDialog::onLanguageChanged(const QString& language) {
  if (keyboardLayouts_.count(language) == 0) {
    return;
  }

  ui->languageKeyboardLayouts->clear();
  for (const auto& layout : keyboardLayouts_[language]) {
    auto* item = new  QListWidgetItem;
    item->setText(layout.toString());
    item->setData(Qt::UserRole, QVariant::fromValue(layout));
    ui->languageKeyboardLayouts->addItem(item);
  }
}

void EditKeyboardLayoutsDialog::onAddButtonClicked() {
  auto* item = ui->languageKeyboardLayouts->currentItem();
  if (item == nullptr) {
    return;
  }
  auto info = item->data(Qt::UserRole).value<KeyboardLayoutInfo>();
  if (ui->userKeyboardLayouts->findItems(info.toString(), Qt::MatchExactly).isEmpty()) {
    auto* newItem = new QListWidgetItem;
    newItem->setText(info.toString());
    newItem->setData(Qt::UserRole, QVariant::fromValue(info));
    ui->userKeyboardLayouts->addItem(newItem);
  }
}

void EditKeyboardLayoutsDialog::onRemoveButtonClicked() {
  auto* item = ui->userKeyboardLayouts->currentItem();
  if (item == nullptr) {
    return;
  }
  delete item;
}

void EditKeyboardLayoutsDialog::saveData() {
  QStringList userLayouts;
  for (int i = 0; i < ui->userKeyboardLayouts->count(); ++i) {
    auto* item = ui->userKeyboardLayouts->item(i);
    auto info = item->data(Qt::UserRole).value<KeyboardLayoutInfo>();
    userLayouts << info.engine;
  }
  model_->setUserKeyboardLayouts(userLayouts);
  model_->saveAppearanceConfig();
}

}  // namespace crystaldock
