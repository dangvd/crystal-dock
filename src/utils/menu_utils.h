/*
 * This file is part of Crystal Dock.
 * Copyright (C) 2024 Viet Dang (dangvd@gmail.com)
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

#ifndef CRYSTALDOCK_MENU_UTILS_H_
#define CRYSTALDOCK_MENU_UTILS_H_

#include <QColor>
#include <QIcon>
#include <QMenu>
#include <QPixmap>

namespace crystaldock {

// A work-around for sub-menu alignment issue on Wayland by adding
// empty items to the sub-menu.
inline void patchMenu(unsigned int totalNumItems, int iconSize, QMenu* menu) {
  QPixmap pix(iconSize, iconSize);
  pix.fill(QColorConstants::Transparent);
  const QIcon icon(pix);
  const auto numItemsToAdd = totalNumItems - menu->actions().size();
  for (unsigned int i = 0; i < numItemsToAdd; ++i) {
    menu->addAction(icon, "");
  }
}

}  // namespace crystaldock

#endif // CRYSTALDOCK_MENU_UTILS_H_
