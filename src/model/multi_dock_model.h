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

#ifndef CRYSTALDOCK_MULTI_DOCK_MODEL_H_
#define CRYSTALDOCK_MULTI_DOCK_MODEL_H_

#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include <QColor>
#include <QDir>
#include <QObject>
#include <QSettings>
#include <QString>

#include "application_menu_config.h"
#include "config_helper.h"
#include "launcher_config.h"
#include <desktop/desktop_env.h>

namespace crystaldock {

enum class PanelPosition { Top, Bottom, Left, Right };

enum class PanelVisibility { AlwaysVisible, AutoHide, AlwaysOnTop, IntelligentAutoHide };

// Glass 3D style only makes the bottom dock really 3D. For left/right/top docks, they will look
// more like "Glass 2D" for aesthetic reasons.
enum class PanelStyle { Glass3D_Floating, Glass3D_NonFloating, Flat2D_Floating, Flat2D_NonFloating,
                        Metal2D_Floating, Metal2D_NonFloating };

constexpr int kDefaultMinSize = 48;
constexpr int kDefaultMaxSize = 128;
constexpr float kDefaultSpacingFactor = 0.5;
constexpr int kDefaultTooltipFontSize = 24;
constexpr float kDefaultBackgroundAlpha = 0.42;
constexpr float kDefaultBackgroundAlphaMetal2D = 0.68;
constexpr char kDefaultBackgroundColor[] = "#638abd";
constexpr char kDefaultBackgroundColor2D[] = "#86baff";
constexpr char kDefaultBackgroundColorMetal2D[] = "#7381a6";
constexpr char kDefaultBorderColor[] = "#b1c4de";
constexpr char kDefaultBorderColorMetal2D[] = "#99addd";
constexpr char kDefaultActiveIndicatorColor[] = "darkorange";
constexpr char kDefaultActiveIndicatorColor2D[] = "#ffbf00";
constexpr char kDefaultActiveIndicatorColorMetal2D[] = "#ffbf00";
constexpr char kDefaultInactiveIndicatorColor[] = "darkcyan";
constexpr char kDefaultInactiveIndicatorColor2D[] = "cyan";
constexpr char kDefaultInactiveIndicatorColorMetal2D[] = "cyan";
constexpr int kDefaultFloatingMargin = 6;

constexpr float kLargeClockFontScaleFactor = 1.0;
constexpr float kMediumClockFontScaleFactor = 0.8;
constexpr float kSmallClockFontScaleFactor = 0.6;

constexpr PanelVisibility kDefaultVisibility = PanelVisibility::AlwaysVisible;
constexpr bool kDefaultAutoHide = false;
constexpr bool kDefaultShowApplicationMenu = true;
constexpr bool kDefaultShowPager = false;
constexpr bool kDefaultShowTaskManager = true;
constexpr bool kDefaultShowClock = false;
constexpr PanelStyle kDefaultPanelStyle = PanelStyle::Glass3D_Floating;

constexpr char kDefaultApplicationMenuName[] = "Applications";
constexpr int kDefaultApplicationMenuIconSize = 40;
constexpr int kDefaultApplicationMenuFontSize = 14;
constexpr float kDefaultApplicationMenuBackgroundAlpha = 0.8;
constexpr bool kDefaultShowDesktopNumber = true;
constexpr bool kDefaultCurrentDesktopTasksOnly = true;
constexpr bool kDefaultCurrentScreenTasksOnly = false;
constexpr bool kDefaultUse24HourClock = true;
constexpr float kDefaultClockFontScaleFactor = kLargeClockFontScaleFactor;

constexpr char kSeparatorId[] = "separator";
constexpr char kLockScreenId[] = "lock-screen";

// The model.
class MultiDockModel : public QObject {
  Q_OBJECT

 public:
  MultiDockModel(const QString& configDir);
  ~MultiDockModel() = default;

  MultiDockModel(const MultiDockModel&) = delete;
  MultiDockModel& operator=(const MultiDockModel&) = delete;

  // Returns the number of docks.
  int dockCount() const { return dockConfigs_.size(); }

  // Adds a new dock in the specified position and screen.
  void addDock(PanelPosition position, int screen, bool showApplicationMenu,
               bool showPager, bool showTaskManager, bool showClock);

  void addDock() {
    addDock(PanelPosition::Bottom, 0, true, true, true, true);
  }

  // Clones an existing dock in the specified position and screen.
  void cloneDock(int srcDockId, PanelPosition position, int screen);

  // Removes a dock.
  void removeDock(int dockId);

  void maybeAddDockForMultiScreen();

  int minIconSize() const {
    return appearanceProperty(kGeneralCategory, kMinimumIconSize,
                              kDefaultMinSize);
  }

  void setMinIconSize(int value) {
    if (value > maxIconSize()) {
      setMaxIconSize(value);
    }
    setAppearanceProperty(kGeneralCategory, kMinimumIconSize, value);
  }

  int maxIconSize() const {
    return appearanceProperty(kGeneralCategory, kMaximumIconSize,
                              kDefaultMaxSize);
  }

  void setMaxIconSize(int value) {
    if (value < minIconSize()) {
      setMinIconSize(value);
    }
    setAppearanceProperty(kGeneralCategory, kMaximumIconSize, value);
  }

  float spacingFactor() const {
    return appearanceProperty(kGeneralCategory, kSpacingFactor,
                              QString::number(kDefaultSpacingFactor)).toFloat();
  }

  // Converts float to string to make the entry in the config file human-readable.
  void setSpacingFactor(float value) {
    setAppearanceProperty(kGeneralCategory, kSpacingFactor, QString::number(value));
  }

  QColor backgroundColor() const {
    QColor defaultBackgroundColor(kDefaultBackgroundColor);
    defaultBackgroundColor.setAlphaF(kDefaultBackgroundAlpha);
    return QColor(appearanceProperty(kGeneralCategory, kBackgroundColor,
                                     defaultBackgroundColor.name(QColor::HexArgb)));
  }

  void setBackgroundColor(const QColor& value) {
    setAppearanceProperty(kGeneralCategory, kBackgroundColor, value.name(QColor::HexArgb));
  }

  QColor backgroundColor2D() const {
    QColor defaultBackgroundColor2D(kDefaultBackgroundColor2D);
    defaultBackgroundColor2D.setAlphaF(kDefaultBackgroundAlpha);
    return QColor(appearanceProperty(kGeneralCategory, kBackgroundColor2D,
                                     defaultBackgroundColor2D.name(QColor::HexArgb)));
  }

  void setBackgroundColor2D(const QColor& value) {
    setAppearanceProperty(kGeneralCategory, kBackgroundColor2D, value.name(QColor::HexArgb));
  }

  QColor backgroundColorMetal2D() const {
    QColor defaultBackgroundColorMetal2D(kDefaultBackgroundColorMetal2D);
    defaultBackgroundColorMetal2D.setAlphaF(kDefaultBackgroundAlphaMetal2D);
    return QColor(appearanceProperty(kGeneralCategory, kBackgroundColorMetal2D,
                                     defaultBackgroundColorMetal2D.name(QColor::HexArgb)));
  }

  void setBackgroundColorMetal2D(const QColor& value) {
    setAppearanceProperty(kGeneralCategory, kBackgroundColorMetal2D, value.name(QColor::HexArgb));
  }

  QColor borderColor() const {
    return QColor(appearanceProperty(kGeneralCategory, kBorderColor,
                                     QString(kDefaultBorderColor)));
  }

  void setBorderColor(const QColor& value) {
    setAppearanceProperty(kGeneralCategory, kBorderColor, value.name(QColor::HexRgb));
  }

  QColor borderColorMetal2D() const {
    return QColor(appearanceProperty(kGeneralCategory, kBorderColorMetal2D,
                                     QString(kDefaultBorderColorMetal2D)));
  }

  void setBorderColorMetal2D(const QColor& value) {
    setAppearanceProperty(kGeneralCategory, kBorderColorMetal2D, value.name(QColor::HexRgb));
  }

  QColor activeIndicatorColor() const {
    return QColor(appearanceProperty(kGeneralCategory, kActiveIndicatorColor,
                                     QString(kDefaultActiveIndicatorColor)));
  }

  void setActiveIndicatorColor(const QColor& value) {
    setAppearanceProperty(kGeneralCategory, kActiveIndicatorColor, value.name(QColor::HexRgb));
  }

  QColor activeIndicatorColor2D() const {
    return QColor(appearanceProperty(kGeneralCategory, kActiveIndicatorColor2D,
                                     QString(kDefaultActiveIndicatorColor2D)));
  }

  void setActiveIndicatorColor2D(const QColor& value) {
    setAppearanceProperty(kGeneralCategory, kActiveIndicatorColor2D, value.name(QColor::HexRgb));
  }

  QColor activeIndicatorColorMetal2D() const {
    return QColor(appearanceProperty(kGeneralCategory, kActiveIndicatorColorMetal2D,
                                     QString(kDefaultActiveIndicatorColorMetal2D)));
  }

  void setActiveIndicatorColorMetal2D(const QColor& value) {
    setAppearanceProperty(kGeneralCategory, kActiveIndicatorColorMetal2D, value.name(QColor::HexRgb));
  }

  QColor inactiveIndicatorColor() const {
    return QColor(appearanceProperty(kGeneralCategory, kInactiveIndicatorColor,
                                     QString(kDefaultInactiveIndicatorColor)));
  }

  void setInactiveIndicatorColor(const QColor& value) {
    setAppearanceProperty(kGeneralCategory, kInactiveIndicatorColor, value.name(QColor::HexRgb));
  }

  QColor inactiveIndicatorColor2D() const {
    return QColor(appearanceProperty(kGeneralCategory, kInactiveIndicatorColor2D,
                                     QString(kDefaultInactiveIndicatorColor2D)));
  }

  void setInactiveIndicatorColor2D(const QColor& value) {
    setAppearanceProperty(kGeneralCategory, kInactiveIndicatorColor2D, value.name(QColor::HexRgb));
  }

  QColor inactiveIndicatorColorMetal2D() const {
    return QColor(appearanceProperty(kGeneralCategory, kInactiveIndicatorColorMetal2D,
                                     QString(kDefaultInactiveIndicatorColorMetal2D)));
  }

  void setInactiveIndicatorColorMetal2D(const QColor& value) {
    setAppearanceProperty(kGeneralCategory, kInactiveIndicatorColorMetal2D,
                          value.name(QColor::HexRgb));
  }

  int tooltipFontSize() const {
    return appearanceProperty(kGeneralCategory, kTooltipFontSize,
                              kDefaultTooltipFontSize);
  }

  void setTooltipFontSize(int value) {
    setAppearanceProperty(kGeneralCategory, kTooltipFontSize, value);
  }

  PanelStyle panelStyle() {
    return static_cast<PanelStyle>(
        appearanceProperty(kGeneralCategory, kPanelStyle,
                           static_cast<int>(kDefaultPanelStyle)));
  }

  void setPanelStyle(PanelStyle value) {
    setAppearanceProperty(kGeneralCategory, kPanelStyle, static_cast<int>(value));
  }

  bool is3D() {
    return panelStyle() == PanelStyle::Glass3D_Floating ||
        panelStyle() == PanelStyle::Glass3D_NonFloating;
  }

  bool isFlat2D() {
    return panelStyle() == PanelStyle::Flat2D_Floating ||
        panelStyle() == PanelStyle::Flat2D_NonFloating;
  }

  bool isMetal2D() {
    return panelStyle() == PanelStyle::Metal2D_Floating ||
        panelStyle() == PanelStyle::Metal2D_NonFloating;
  }

  int floatingMargin() const {
    return appearanceProperty(kGeneralCategory, kFloatingMargin,
                              kDefaultFloatingMargin);
  }

  void setFloatingMargin(int value) {
    setAppearanceProperty(kGeneralCategory, kFloatingMargin, value);
  }

  bool firstRunMultiScreen() {
    const auto value = appearanceProperty(kGeneralCategory, kFirstRunMultiScreen, true);
    setAppearanceProperty(kGeneralCategory, kFirstRunMultiScreen, false);
    return value;
  }

  bool firstRunWindowCountIndicator() {
    const auto value = appearanceProperty(kGeneralCategory, kFirstRunWindowCountIndicator, true);
    setAppearanceProperty(kGeneralCategory, kFirstRunWindowCountIndicator, false);
    return value;
  }

  QString applicationMenuName() const {
    return appearanceProperty(kApplicationMenuCategory, kLabel,
                              QString(kDefaultApplicationMenuName));
  }

  void setApplicationMenuName(const QString& value) {
    setAppearanceProperty(kApplicationMenuCategory, kLabel, value);
  }

  QString applicationMenuIcon() const {
    return desktopEnv_->getApplicationMenuIcon();
  }

  int applicationMenuIconSize() const {
    return appearanceProperty(kApplicationMenuCategory, kIconSize,
                              kDefaultApplicationMenuIconSize);
  }

  void setApplicationMenuIconSize(int value) {
    setAppearanceProperty(kApplicationMenuCategory, kIconSize, value);
  }

  int applicationMenuFontSize() const {
    return appearanceProperty(kApplicationMenuCategory, kFontSize,
                              kDefaultApplicationMenuFontSize);
  }

  void setApplicationMenuFontSize(int value) {
    setAppearanceProperty(kApplicationMenuCategory, kFontSize, value);
  }

  float applicationMenuBackgroundAlpha() const {
    return appearanceProperty(kApplicationMenuCategory, kBackgroundAlpha,
                              QString::number(kDefaultApplicationMenuBackgroundAlpha)).toFloat();
  }

  // Converts float to string to make the entry in the config file human-readable.
  void setApplicationMenuBackgroundAlpha(float value) {
    setAppearanceProperty(kApplicationMenuCategory, kBackgroundAlpha, QString::number(value));
  }

  QString wallpaper(std::string_view desktopId, int screen) const {
    return appearanceProperty(kPagerCategory,
                              ConfigHelper::wallpaperConfigKey(desktopId, screen),
                              QString());
  }

  void setWallpaper(std::string_view desktopId, int screen, const QString& value) {
    setAppearanceProperty(kPagerCategory,
                          ConfigHelper::wallpaperConfigKey(desktopId, screen),
                          value);
  }

  // Notifies that the wallpaper for the current desktop for the specified
  // screen has been changed.
  void notifyWallpaperChanged(int screen) {
    emit wallpaperChanged(screen);
  }

  bool showDesktopNumber() const {
    return appearanceProperty(kPagerCategory, kShowDesktopNumber,
                              kDefaultShowDesktopNumber);
  }

  void setShowDesktopNumber(bool value) {
    setAppearanceProperty(kPagerCategory, kShowDesktopNumber, value);
  }

  bool currentDesktopTasksOnly() const {
    return appearanceProperty(kTaskManagerCategory, kCurrentDesktopTasksOnly,
                              kDefaultCurrentDesktopTasksOnly);
  }

  void setCurrentDesktopTasksOnly(bool value) {
    setAppearanceProperty(kTaskManagerCategory, kCurrentDesktopTasksOnly, value);
  }

  bool currentScreenTasksOnly() const {
    return appearanceProperty(kTaskManagerCategory, kCurrentScreenTasksOnly,
                              kDefaultCurrentScreenTasksOnly);
  }

  void setCurrentScreenTasksOnly(bool value) {
    setAppearanceProperty(kTaskManagerCategory, kCurrentScreenTasksOnly, value);
  }

  bool use24HourClock() const {
    return appearanceProperty(kClockCategory, kUse24HourClock,
                              kDefaultUse24HourClock);
  }

  void setUse24HourClock(bool value) {
    setAppearanceProperty(kClockCategory, kUse24HourClock, value);
  }

  float clockFontScaleFactor() const {
    return appearanceProperty(kClockCategory, kFontScaleFactor,
                              QString::number(kDefaultClockFontScaleFactor)).toFloat();
  }

  // Converts float to string to make the entry in the config file human-readable.
  void setClockFontScaleFactor(float value) {
    setAppearanceProperty(kClockCategory, kFontScaleFactor, QString::number(value));
  }

  QString clockFontFamily() const {
    return appearanceProperty(kClockCategory, kClockFontFamily, QString());
  }

  void setClockFontFamily(const QString& value) {
    setAppearanceProperty(kClockCategory, kClockFontFamily, value);
  }

  void saveAppearanceConfig(bool repaintOnly = false) {
    syncAppearanceConfig();
    if (repaintOnly) {
      emit appearanceOutdated();
    } else {
      emit appearanceChanged();
    }
  }

  PanelPosition panelPosition(int dockId) const {
    return static_cast<PanelPosition>(dockProperty(
        dockId, kGeneralCategory, kPosition,
        static_cast<int>(PanelPosition::Bottom)));
  }

  void setPanelPosition(int dockId, PanelPosition value) {
    setDockProperty(dockId, kGeneralCategory, kPosition,
                    static_cast<int>(value));
  }

  int screen(int dockId) const {
    return dockProperty(dockId, kGeneralCategory, kScreen, 0);
  }

  void setScreen(int dockId, int value) {
    setDockProperty(dockId, kGeneralCategory, kScreen, value);
  }

  PanelVisibility visibility(int dockId) const {
    if (autoHide(dockId)) {  // for backward compatibility.
      return PanelVisibility::AutoHide;
    }
    return static_cast<PanelVisibility>(dockProperty(
        dockId, kGeneralCategory, kVisibility,
        static_cast<int>(kDefaultVisibility)));
  }

  void setVisibility(int dockId, PanelVisibility value) {
    setDockProperty(dockId, kGeneralCategory, kVisibility,
                    static_cast<int>(value));
    // For backward compatibility.
    setAutoHide(dockId, value == PanelVisibility::AutoHide);
  }

  bool autoHide(int dockId) const {
    return dockProperty(dockId, kGeneralCategory, kAutoHide, kDefaultAutoHide);
  }

  void setAutoHide(int dockId, bool value) {
    setDockProperty(dockId, kGeneralCategory, kAutoHide, value);
  }

  bool showApplicationMenu(int dockId) const {
    return dockProperty(dockId, kGeneralCategory, kShowApplicationMenu,
                        kDefaultShowApplicationMenu);
  }

  void setShowApplicationMenu(int dockId, bool value) {
    setDockProperty(dockId, kGeneralCategory, kShowApplicationMenu, value);
  }

  bool showPager(int dockId) const {
    return dockProperty(dockId, kGeneralCategory, kShowPager, kDefaultShowPager);
  }

  void setShowPager(int dockId, bool value) {
    setDockProperty(dockId, kGeneralCategory, kShowPager, value);
  }

  bool showTaskManager(int dockId) const {
    return dockProperty(dockId, kGeneralCategory, kShowTaskManager,
                        kDefaultShowTaskManager);
  }

  void setShowTaskManager(int dockId, bool value) {
    setDockProperty(dockId, kGeneralCategory, kShowTaskManager, value);
  }

  bool showClock(int dockId) const {
    return dockProperty(dockId, kGeneralCategory, kShowClock,
                        kDefaultShowClock);
  }

  void setShowClock(int dockId, bool value) {
    setDockProperty(dockId, kGeneralCategory, kShowClock, value);
  }

  QStringList launchers(int dockId) const {
    return dockProperty(dockId, kGeneralCategory, kLaunchers, QString())
        .split(";", Qt::SkipEmptyParts);
  }

  void setLaunchers(int dockId, QStringList value) {
    setDockProperty(dockId, kGeneralCategory, kLaunchers, value.join(";"));
  }

  void saveDockConfig(int dockId) {
    syncDockConfig(dockId);
    emit dockLaunchersChanged(dockId);
  }

  const std::vector<LauncherConfig> launcherConfigs(int dockId) const;

  void addLauncher(int dockId, const LauncherConfig& launcher) {
    auto entries = launchers(dockId);
    unsigned int i = 0;
    for (; i < entries.size() && entries[i] != kSeparatorId; ++i) {}
    entries.insert(i, launcher.appId);
    setLaunchers(dockId, entries);
    syncDockConfig(dockId);
  }

  void removeLauncher(int dockId, const QString& appId) {
    auto entries = launchers(dockId);
    for (unsigned i = 0; i < entries.size(); ++i) {
      if (entries[i] == appId) {
        entries.remove(i);
        setLaunchers(dockId, entries);
        syncDockConfig(dockId);
        return;
      }
    }
  }

  // Whether any dock has a pager.
  bool hasPager() const;

  const std::vector<Category>& applicationMenuCategories() const {
    return applicationMenuConfig_.categories();
  }

  const std::vector<Category>& applicationMenuSystemCategories() const {
    return applicationMenuConfig_.systemCategories();
  }

  const ApplicationEntry* findApplication(const std::string& appId) const {
    return applicationMenuConfig_.findApplication(appId);
  }

  bool isAppMenuEntry(const std::string& appId) const {
    return applicationMenuConfig_.isAppMenuEntry(appId);
  }

  const std::vector<ApplicationEntry> searchApplications(const QString& text) const {
    return applicationMenuConfig_.searchApplications(text);
  }

 signals:
  // Minor appearance changes that require view update (repaint).
  void appearanceOutdated();
  // Major appearance changes that require view reload.
  void appearanceChanged();
  void dockAdded(int dockId);
  void dockLaunchersChanged(int dockId);
  // Wallpaper for the current desktop for screen <screen> has been changed.
  // Will require calling Plasma D-Bus to update the wallpaper.
  void wallpaperChanged(int screen);
  void applicationMenuConfigChanged();

 private:
  // Dock config's categories/properties.
  static constexpr char kGeneralCategory[] = "";
  static constexpr char kAutoHide[] = "autoHide";
  static constexpr char kVisibility[] = "visibility";
  static constexpr char kPosition[] = "position";
  static constexpr char kScreen[] = "screen";
  static constexpr char kShowApplicationMenu[] = "showApplicationMenu";
  static constexpr char kShowClock[] = "showClock";
  static constexpr char kShowPager[] = "showPager";
  static constexpr char kShowTaskManager[] = "showTaskManager";
  static constexpr char kLaunchers[] = "launchers";

  // Global appearance config's categories/properties.

  // General category.
  static constexpr char kBackgroundColor[] = "backgroundColor";
  static constexpr char kBackgroundColor2D[] = "backgroundColor2D";
  static constexpr char kBackgroundColorMetal2D[] = "backgroundColorMetal2D";
  static constexpr char kBorderColor[] = "borderColor";
  static constexpr char kBorderColorMetal2D[] = "borderColorMetal2D";
  static constexpr char kActiveIndicatorColor[] = "activeIndicatorColor";
  static constexpr char kActiveIndicatorColor2D[] = "activeIndicatorColor2D";
  static constexpr char kActiveIndicatorColorMetal2D[] = "activeIndicatorColorMetal2D";
  static constexpr char kInactiveIndicatorColor[] = "inactiveIndicatorColor";
  static constexpr char kInactiveIndicatorColor2D[] = "inactiveIndicatorColor2D";
  static constexpr char kInactiveIndicatorColorMetal2D[] = "inactiveIndicatorColorMetal2D";
  static constexpr char kMaximumIconSize[] = "maximumIconSize";
  static constexpr char kMinimumIconSize[] = "minimumIconSize";
  static constexpr char kSpacingFactor[] = "spacingFactor";
  static constexpr char kTooltipFontSize[] = "tooltipFontSize";
  static constexpr char kPanelStyle[] = "panelStyle";
  static constexpr char kFloatingMargin[] = "floatingMargin";

  static constexpr char kFirstRunMultiScreen[] = "firstRunMultiScreen";
  static constexpr char kFirstRunWindowCountIndicator[] = "firstRunWindowCountIndicator";

  static constexpr char kApplicationMenuCategory[] = "Application Menu";
  static constexpr char kLabel[] = "label";
  static constexpr char kIconSize[] = "iconSize";
  static constexpr char kFontSize[] = "fontSize";
  static constexpr char kBackgroundAlpha[] = "backgroundAlpha";

  static constexpr char kPagerCategory[] = "Pager";
  static constexpr char kWallpaper[] = "wallpaper";
  static constexpr char kShowDesktopNumber[] = "showDesktopNumber";  

  static constexpr char kTaskManagerCategory[] = "TaskManager";
  static constexpr char kCurrentDesktopTasksOnly[] = "currentDesktopTasksOnly";
  static constexpr char kCurrentScreenTasksOnly[] = "currentScreenTasksOnly";

  static constexpr char kClockCategory[] = "Clock";
  static constexpr char kUse24HourClock[] = "use24HourClock";
  static constexpr char kFontScaleFactor[] = "fontScaleFactor";
  static constexpr char kClockFontFamily[] = "clockFontFamily";

  template <typename T>
  T appearanceProperty(QString category, QString name, T defaultValue) const {
    return category.isEmpty()
        ? appearanceConfig_.value(name, defaultValue).template value<T>()
        : appearanceConfig_.value(category + '/' + name, defaultValue).template value<T>();
  }
  template <typename T>
  void setAppearanceProperty(QString category, QString name, T value) {
    if (category.isEmpty()) {
      appearanceConfig_.setValue(name, value);
      return;
    }

    appearanceConfig_.beginGroup(category);
    appearanceConfig_.setValue(name, value);
    appearanceConfig_.endGroup();
  }

  template <typename T>
  T dockProperty(int dockId, QString category, QString name, T defaultValue)
      const {
    return category.isEmpty()
        ? dockConfig(dockId)->value(name, defaultValue).template value<T>()
        : dockConfig(dockId)->value(category + '/' + name, defaultValue).template value<T>();
  }

  template <typename T>
  void setDockProperty(int dockId, QString category, QString name, T value) {
    auto* config = dockConfig(dockId);
    if (category.isEmpty()) {
      config->setValue(name, value);
      return;
    }

    config->beginGroup(category);
    config->setValue(name, value);
    config->endGroup();
  }

  QString dockConfigPath(int dockId) const {
    return std::get<0>(dockConfigs_.at(dockId));
  }

  const QSettings* dockConfig(int dockId) const {
    return std::get<1>(dockConfigs_.at(dockId)).get();
  }

  QSettings* dockConfig(int dockId) {
    return std::get<1>(dockConfigs_[dockId]).get();
  }

  QStringList defaultLaunchers();
  const ApplicationEntry* defaultBrowser();

  void loadDocks();

  int addDock(const QString& configPath, PanelPosition position, int screen);

  void syncAppearanceConfig() {
    appearanceConfig_.sync();
  }

  void syncDockConfig(int dockId) {
    dockConfig(dockId)->sync();
  }

  // Helper(s).
  ConfigHelper configHelper_;

  // Model data.

  // Appearance config.
  QSettings appearanceConfig_;

  // Dock configs, as map from dockIds to tuples of:
  // (dock config file path,
  //  dock config)
  std::unordered_map<int,
                     std::tuple<QString,
                                std::unique_ptr<QSettings>>> dockConfigs_;

  // ID for the next dock.
  int nextDockId_;

  ApplicationMenuConfig applicationMenuConfig_;
  DesktopEnv* desktopEnv_;
};

}  // namespace crystaldock

#endif  // CRYSTALDOCK_MULTI_DOCK_MODEL_H_
