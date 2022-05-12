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

#ifndef CRYSTALDOCK_FONT_UTILS_H_
#define CRYSTALDOCK_FONT_UTILS_H_

#include <algorithm>
#include <vector>

#include <QFont>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QRect>
#include <QString>

namespace crystaldock {

// Returns a QFont with font size adjusted automatically according to the given
// width, height, reference string and scale factor.
inline QFont adjustFontSize(int w, int h, const QString& referenceString,
                            float scaleFactor, const QString& fontFamily = "") {
  QFont font;
  QFontMetrics metrics(font);
  const QRect& rect = metrics.tightBoundingRect(referenceString);
  // Scale the font size according to the size of the dock.
  font.setPointSize(std::min(font.pointSize() * w / rect.width(),
                             font.pointSize() * h / rect.height()));
  font.setPointSize(static_cast<int>(font.pointSize() * scaleFactor));
  if (!fontFamily.isEmpty()) {
    font.setFamily(fontFamily);
  }

  return font;
}

// Gets the list of base font families, i.e. just 'Noto Sans'
// instead of 'Noto Sans Bold', 'Noto Sans CJK' etc.
inline std::vector<QString> getBaseFontFamilies() {
  std::vector<QString> baseFamilies;
  QFontDatabase database;
  const auto families = database.families(QFontDatabase::Latin);
  for (int i = 0; i < families.size(); ++i) {
    bool isBaseFont = true;
    const auto family = families.at(i);
    if (database.isSmoothlyScalable(family)) {
      for (int j = 0; j < families.size(); ++j) {
        if (family.startsWith(families[j] + ' ')) {
          isBaseFont = false;
          break;
        }
      }
      if (isBaseFont) {
        baseFamilies.push_back(family);
      }
    }
  }
  return baseFamilies;
}

}  // namespace crystaldock

#endif  // CRYSTALDOCK_FONT_UTILS_H_
