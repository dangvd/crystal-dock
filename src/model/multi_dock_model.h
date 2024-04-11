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
#include <utils/command_utils.h>

namespace crystaldock {

enum class PanelPosition { Top, Bottom, Left, Right };

enum class PanelVisibility { AlwaysVisible, AutoHide, WindowsGoBelow };

constexpr int kDefaultMinSize = 48;
constexpr int kDefaultMaxSize = 128;
constexpr float kDefaultSpacingFactor = 0.5;
constexpr int kDefaultTooltipFontSize = 24;
constexpr float kDefaultBackgroundAlpha = 0.42;
constexpr char kDefaultBackgroundColor[] = "#638abd";
constexpr bool kDefaultShowBorder = true;
constexpr char kDefaultBorderColor[] = "#b1c4de";
constexpr float kLargeClockFontScaleFactor = 1.0;
constexpr float kMediumClockFontScaleFactor = 0.8;
constexpr float kSmallClockFontScaleFactor = 0.6;

constexpr PanelVisibility kDefaultVisibility = PanelVisibility::AlwaysVisible;
constexpr bool kDefaultAutoHide = false;
constexpr bool kDefaultShowApplicationMenu = true;
constexpr bool kDefaultShowPager = false;
constexpr bool kDefaultShowTaskManager = true;
constexpr bool kDefaultShowClock = false;

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

  int minIconSize() const {
    return appearanceProperty(kGeneralCategory, kMinimumIconSize,
                              kDefaultMinSize);
  }

  void setMinIconSize(int value) {
    setAppearanceProperty(kGeneralCategory, kMinimumIconSize, value);
  }

  int maxIconSize() const {
    return appearanceProperty(kGeneralCategory, kMaximumIconSize,
                              kDefaultMaxSize);
  }

  void setMaxIconSize(int value) {
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

  bool showBorder() const {
    return appearanceProperty(kGeneralCategory, kShowBorder,
                              kDefaultShowBorder);
  }

  void setShowBorder(bool value) {
    setAppearanceProperty(kGeneralCategory, kShowBorder, value);
  }

  QColor borderColor() const {
    return QColor(appearanceProperty(kGeneralCategory, kBorderColor,
                                     QString(kDefaultBorderColor)));
  }

  void setBorderColor(const QColor& value) {
    setAppearanceProperty(kGeneralCategory, kBorderColor, value.name(QColor::HexRgb));
  }

  int tooltipFontSize() const {
    return appearanceProperty(kGeneralCategory, kTooltipFontSize,
                              kDefaultTooltipFontSize);
  }

  void setTooltipFontSize(int value) {
    setAppearanceProperty(kGeneralCategory, kTooltipFontSize, value);
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
    // No need to emit signal here.
  }

  const std::vector<LauncherConfig> launcherConfigs(int dockId) const;

  void setLauncherConfigs(
      int dockId, const std::vector<LauncherConfig>& launcherConfigs);

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
  static constexpr char kBorderColor[] = "borderColor";
  static constexpr char kMaximumIconSize[] = "maximumIconSize";
  static constexpr char kMinimumIconSize[] = "minimumIconSize";
  static constexpr char kSpacingFactor[] = "spacingFactor";
  static constexpr char kShowBorder[] = "showBorder";
  static constexpr char kTooltipFontSize[] = "tooltipFontSize";

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
