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

#ifndef CRYSTALDOCK_ADD_PANEL_DIALOG_H_
#define CRYSTALDOCK_ADD_PANEL_DIALOG_H_

#include <QDialog>

#include <model/multi_dock_model.h>

namespace Ui {
  class AddPanelDialog;
}

namespace crystaldock {

class AddPanelDialog : public QDialog {
  Q_OBJECT

 public:
  enum class Mode { Add, Clone, Welcome };

  // Parameter dockId is only needed in Clone mode.
  AddPanelDialog(QWidget* parent, MultiDockModel* model, int dockId);
  ~AddPanelDialog();

  void setMode(Mode mode);

 public slots:
  void accept() override;

 private:
  Ui::AddPanelDialog *ui;

  Mode mode_;
  MultiDockModel* model_;
  int dockId_;

  bool isSingleScreen_;

  friend class AddPanelDialogTest;
};

}  // namespace crystaldock

#endif  // CRYSTALDOCK_ADD_PANEL_DIALOG_H_
