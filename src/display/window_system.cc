#include "window_system.h"

#include <iostream>
#include <string>

#include <QDBusReply>

#include <LayerShellQt/Shell>
#include <LayerShellQt/Window>

namespace crystaldock {

namespace {

LayerShellQt::Window* getLayerShellWin(QWidget* widget) {
  widget->winId();  // we need this for widget->windowHandle() to not return nullptr.
  QWindow* win = widget->windowHandle();
  if (win == nullptr) {
    std::cerr << "Null QWindow" << std::endl;
    return nullptr;
  }

  return LayerShellQt::Window::get(win);
}

}  // namespace

org_kde_plasma_virtual_desktop_management* WindowSystem::virtual_desktop_management_;
org_kde_plasma_window_management* WindowSystem::window_management_;

std::vector<VirtualDesktopInfo> WindowSystem::desktops_;
std::string WindowSystem::currentDesktop_;
bool WindowSystem::showingDesktop_;
std::unique_ptr<QDBusInterface> WindowSystem::activityManager_;

std::unordered_map<struct org_kde_plasma_window*, WindowInfo*> WindowSystem::windows_;
std::unordered_map<std::string, struct org_kde_plasma_window*> WindowSystem::uuids_;
std::vector<std::string> WindowSystem::stackingOrder_;
std::string WindowSystem::activeUuid_;

ApplicationMenuConfig WindowSystem::applicationMenuConfig_;

/* static */ WindowSystem* WindowSystem::self() {
  static WindowSystem* self = nullptr;
  if (!self) {
    self = new WindowSystem();
  }
  return self;
}

/* static */ bool WindowSystem::init(struct wl_display* display) {
  struct wl_registry *registry = wl_display_get_registry(display);
  wl_registry_add_listener(registry, &registry_listener_, NULL);

  // wait for the "initial" set of globals to appear
  wl_display_roundtrip(display);

  if (!virtual_desktop_management_ || !window_management_) {
    std::cerr << "Failed to bind required Wayland interfaces" << std::endl;
    return false;
  }

  org_kde_plasma_virtual_desktop_management_add_listener(
      virtual_desktop_management_, &virtual_desktop_management_listener_, NULL);
  org_kde_plasma_window_management_add_listener(
      window_management_, &window_management_listener_, NULL);

  LayerShellQt::Shell::useLayerShell();

  activityManager_ = std::make_unique<QDBusInterface>(
      "org.kde.ActivityManager", "/ActivityManager/Activities",
      "org.kde.ActivityManager.Activities");
  if (activityManager_->isValid()) {
    const QDBusReply<QString> reply = activityManager_->call("CurrentActivity");
    if (reply.isValid()) {
      WindowSystem::self()->setCurrentActivity(reply.value().toStdString());
    }
    connect(activityManager_.get(), SIGNAL(CurrentActivityChanged(QString)),
            WindowSystem::self(), SLOT(onCurrentActivityChanged(QString)));
  }

  return true;
}

/* static */ int WindowSystem::numberOfDesktops() {
  return desktops_.size();
}

/* static */ std::string_view WindowSystem::currentDesktop() {
  return currentDesktop_;
}

/* static */ void WindowSystem::setCurrentDesktop(std::string_view desktopId) {
  auto pos = std::find_if(desktops_.begin(), desktops_.end(),
               [desktopId](auto& e) { return e.id == desktopId; });
  if (pos != desktops_.end()) {
    org_kde_plasma_virtual_desktop_request_activate(pos->virtual_desktop);
  }
}

/* static */ bool WindowSystem::showingDesktop() {
  return showingDesktop_;
}

/* static */ void WindowSystem::setShowingDesktop(bool show) {
  // This does not work because it would hide Crystal Dock.
  /*
  org_kde_plasma_window_management_show_desktop(
      window_management_,
      show ? ORG_KDE_PLASMA_WINDOW_MANAGEMENT_SHOW_DESKTOP_ENABLED
           : ORG_KDE_PLASMA_WINDOW_MANAGEMENT_SHOW_DESKTOP_DISABLED);
  */

  // So we make our own implementation.
  for (const auto& uuid : stackingOrder_) {
    auto* window = uuids_[uuid];
    auto* windowInfo = windows_[window];
    if (windowInfo->desktop != currentDesktop_) {
      continue;
    }

    if (show) {
      windowInfo->restoreAfterShowDesktop = !windowInfo->minimized;
      if (!windowInfo->minimized) {
        org_kde_plasma_window_set_state(
            window,
            ORG_KDE_PLASMA_WINDOW_MANAGEMENT_STATE_MINIMIZED,
            ORG_KDE_PLASMA_WINDOW_MANAGEMENT_STATE_MINIMIZED);
      }
      showingDesktop_ = true;
    } else {
      if (windowInfo->restoreAfterShowDesktop) {
        org_kde_plasma_window_set_state(
            window,
            ORG_KDE_PLASMA_WINDOW_MANAGEMENT_STATE_ACTIVE,
            ORG_KDE_PLASMA_WINDOW_MANAGEMENT_STATE_ACTIVE);
      }
      showingDesktop_ = false;
    }
  }
}

/* static */ std::vector<const WindowInfo*> WindowSystem::windows() {
  std::vector<const WindowInfo*> windows;
  for (const auto& element : windows_) {
    if (element.second != nullptr) {
      windows.push_back(element.second);
    }
  }
  return windows;
}

/* static */ std::string_view WindowSystem::activeWindow() {
  return activeUuid_;
}

/* static */ void WindowSystem::activateWindow(const std::string& uuid) {
  if (uuids_.count(uuid) == 0) {
    return;
  }

  auto* window = uuids_[uuid];
  if (window) {
    org_kde_plasma_window_set_state(
        window,
        ORG_KDE_PLASMA_WINDOW_MANAGEMENT_STATE_ACTIVE,
        ORG_KDE_PLASMA_WINDOW_MANAGEMENT_STATE_ACTIVE);
  }
}

/* static */ void WindowSystem::activateOrMinimizeWindow(const std::string& uuid) {
  if (uuids_.count(uuid) == 0) {
    return;
  }

  auto* window = uuids_[uuid];
  if (window) {
    if (windows_.count(window) == 0) {
      return;
    }

    auto* info = windows_[window];
    if (info) {
      if (info->minimized || uuid != activeUuid_) {
          org_kde_plasma_window_set_state(
              window,
              ORG_KDE_PLASMA_WINDOW_MANAGEMENT_STATE_ACTIVE,
              ORG_KDE_PLASMA_WINDOW_MANAGEMENT_STATE_ACTIVE);
      } else {
        org_kde_plasma_window_set_state(
            window,
            ORG_KDE_PLASMA_WINDOW_MANAGEMENT_STATE_MINIMIZED,
            ORG_KDE_PLASMA_WINDOW_MANAGEMENT_STATE_MINIMIZED);
      }
    }
  }
}

/*static*/ void WindowSystem::closeWindow(const std::string& uuid) {
  if (uuids_.count(uuid) == 0) {
    return;
  }

  auto* window = uuids_[uuid];
  if (window) {
    org_kde_plasma_window_close(window);
  }
}

/* static */ void WindowSystem::setAnchorAndStrut(
    QWidget* widget, LayerShellQt::Window::Anchors anchors, uint32_t strutSize) {
  auto* layerShellWin = getLayerShellWin(widget);
  if (layerShellWin) {
    layerShellWin->setLayer(LayerShellQt::Window::LayerBottom);
    layerShellWin->setAnchors(anchors);
    layerShellWin->setExclusiveZone(strutSize);
  }
}

/*static*/ void WindowSystem::setLayer(QWidget* widget, LayerShellQt::Window::Layer layer) {
  auto* layerShellWin = getLayerShellWin(widget);
  if (layerShellWin) {
    layerShellWin->setLayer(layer);
  }
}

// wl_registry interface.

/* static */ void WindowSystem::registry_global(
    void* data,
    struct wl_registry* registry,
    uint32_t name,
    const char* interface,
    uint32_t version) {
  if (std::string(interface) == "org_kde_plasma_virtual_desktop_management") {
    virtual_desktop_management_ =
        reinterpret_cast<org_kde_plasma_virtual_desktop_management*>(wl_registry_bind(
            registry, name, &org_kde_plasma_virtual_desktop_management_interface, 2));
    if (!virtual_desktop_management_) {
      std::cerr << "Failed to bind org_kde_plasma_virtual_desktop_management Wayland interface"
                << std::endl;
    }
  } else if (std::string(interface) == "org_kde_plasma_window_management") {
    window_management_ =
        reinterpret_cast<org_kde_plasma_window_management*>(wl_registry_bind(
            registry, name, &org_kde_plasma_window_management_interface, 16));
    if (!window_management_) {
      std::cerr << "Failed to bind org_kde_plasma_window_management Wayland interface. "
                << "Maybe another client has already bound it?" << std::endl;
    }
  }
}

/* static */ void WindowSystem::registry_global_remove(
    void *data, struct wl_registry *wl_registry, uint32_t name) {
  // Ignore.
}

// org_kde_plasma_virtual_desktop_management interface.

/* static */ void WindowSystem::desktop_management_desktop_created(
    void* data, org_kde_plasma_virtual_desktop_management* virtual_desktop_management,
    const char* desktop_id, uint32_t position) {
  VirtualDesktopInfo info;
  info.id = desktop_id;
  info.number = position + 1;  
  auto* virtual_desktop = org_kde_plasma_virtual_desktop_management_get_virtual_desktop(
      virtual_desktop_management, desktop_id);
  info.virtual_desktop = virtual_desktop;
  desktops_.insert(desktops_.begin() + position, info);
  org_kde_plasma_virtual_desktop_add_listener(
      virtual_desktop, &virtual_desktop_listener_, NULL);
  emit self()->numberOfDesktopsChanged(desktops_.size());
}

/* static */ void WindowSystem::desktop_management_desktop_removed(
    void* data, org_kde_plasma_virtual_desktop_management* virtual_desktop_management,
    const char* desktop_id) {
  desktops_.erase(std::find_if(desktops_.begin(), desktops_.end(),
                               [desktop_id](auto& e) { return e.id == desktop_id; }));
  for (unsigned int pos = 0; pos < desktops_.size(); ++pos) {
    desktops_[pos].number = pos + 1;
  }
  emit self()->numberOfDesktopsChanged(desktops_.size());
}

/* static */ void WindowSystem::desktop_management_done(void *data,
          struct org_kde_plasma_virtual_desktop_management* virtual_desktop_management) {
  // Ignore.
}

/* static */ void WindowSystem::desktop_management_desktop_rows(
    void* data, org_kde_plasma_virtual_desktop_management* virtual_desktop_management,
    uint32_t rows) {
  // Ignore.
}

// org_kde_plasma_virtual_desktop interface.

/* static */ void WindowSystem::desktop_id(
    void *data,
    struct org_kde_plasma_virtual_desktop *virtual_desktop,
    const char *desktop_id) {
  auto pos = std::find_if(desktops_.begin(), desktops_.end(),
               [virtual_desktop](auto& e) { return e.virtual_desktop == virtual_desktop; });
  if (pos != desktops_.end()) {
    pos->id = desktop_id;
  }
}

/* static */ void WindowSystem::desktop_name(
    void *data,
    struct org_kde_plasma_virtual_desktop *virtual_desktop,
    const char *name) {
  auto pos = std::find_if(desktops_.begin(), desktops_.end(),
               [virtual_desktop](auto& e) { return e.virtual_desktop == virtual_desktop; });
  if (pos != desktops_.end()) {
    pos->name = name;
    emit WindowSystem::self()->desktopNameChanged(pos->id, pos->name);
  }
}

/* static */ void WindowSystem::desktop_activated(
    void *data,
    struct org_kde_plasma_virtual_desktop *virtual_desktop) {
  auto pos = std::find_if(desktops_.begin(), desktops_.end(),                          
               [virtual_desktop](auto& e) { return e.virtual_desktop == virtual_desktop; });
  if (pos != desktops_.end()) {
    if (currentDesktop_ != pos->id) {
      currentDesktop_ = pos->id;
      emit self()->currentDesktopChanged(currentDesktop_);
    }
  }
}

/* static */ void WindowSystem::desktop_deactivated(
    void *data,
    struct org_kde_plasma_virtual_desktop *org_kde_plasma_virtual_desktop) {
  // Ignore.
}

/* static */ void WindowSystem::desktop_done(
    void *data,
    struct org_kde_plasma_virtual_desktop *org_kde_plasma_virtual_desktop) {
  // Ignore.
}

/* static */ void WindowSystem::desktop_removed(
    void *data,
    struct org_kde_plasma_virtual_desktop *virtual_desktop) {
  // Ignore.
}

// org_kde_plasma_window_management interface.

/* static */ void WindowSystem::show_desktop_changed(
    void *data,
    struct org_kde_plasma_window_management *org_kde_plasma_window_management,
    uint32_t state) {
  // Ignore and use our own state.
  //showingDesktop_ = state & ORG_KDE_PLASMA_WINDOW_MANAGEMENT_SHOW_DESKTOP_ENABLED;
}

/* static */ void WindowSystem::window(
    void *data,
    struct org_kde_plasma_window_management *org_kde_plasma_window_management,
    uint32_t id) {
  // Ignore.
}

/* static */ void WindowSystem::stacking_order_changed(
    void *data,
    struct org_kde_plasma_window_management *org_kde_plasma_window_management,
    struct wl_array *ids) {
  // Ignore.
}

/* static */ void WindowSystem::stacking_order_uuid_changed(
    void *data,
    struct org_kde_plasma_window_management *org_kde_plasma_window_management,
    const char *uuids) {
  QStringList ids = QString(uuids).split(";", Qt::SkipEmptyParts);
  stackingOrder_.clear();
  for (const auto& id : ids) {
    stackingOrder_.push_back(id.toStdString());
  }
}

/* static */ void WindowSystem::window_with_uuid(
    void *data,
    struct org_kde_plasma_window_management *org_kde_plasma_window_management,
    uint32_t id,
    const char *uuid) {
  struct org_kde_plasma_window* window =
      org_kde_plasma_window_management_get_window_by_uuid(window_management_, uuid);  
  WindowInfo* info = new WindowInfo();
  info->uuid = std::string(uuid);
  windows_[window] = info;
  uuids_[info->uuid] = window;

  org_kde_plasma_window_add_listener(window, &window_listener_, NULL);
}

// org_kde_plasma_window interface.

/* static */ void WindowSystem::title_changed(
    void *data,
    struct org_kde_plasma_window* window,
    const char *title) {
  if (windows_.count(window) == 0) {
    return;
  }
  WindowInfo* info = windows_[window];
  if (info) {
    info->title = title;
  }
}

/* static */ void WindowSystem::app_id_changed(
    void *data,
    struct org_kde_plasma_window* window,
    const char *app_id) {
  if (windows_.count(window) == 0) {
    return;
  }
  WindowInfo* info = windows_[window];
  if (info) {
    info->appId = app_id;
  }

  if (std::string(app_id) == "crystal-dock") {
    org_kde_plasma_window_set_state(
        window,
        ORG_KDE_PLASMA_WINDOW_MANAGEMENT_STATE_ON_ALL_DESKTOPS |
            ORG_KDE_PLASMA_WINDOW_MANAGEMENT_STATE_SKIPTASKBAR,
        ORG_KDE_PLASMA_WINDOW_MANAGEMENT_STATE_ON_ALL_DESKTOPS |
            ORG_KDE_PLASMA_WINDOW_MANAGEMENT_STATE_SKIPTASKBAR);
  }

  if (applicationMenuConfig_.findApplication(app_id) == nullptr) {
    // Try to fix the app ID.

    // E.g. Google Chrome
    QString id = app_id;
    std::string fixedAppId = id.toLower().toStdString();
    if (applicationMenuConfig_.findApplication(fixedAppId) != nullptr) {
      info->appId = fixedAppId;
      return;
    }

    // E.g. Krita
    fixedAppId = std::string{"org.kde."} + app_id;
    if (applicationMenuConfig_.findApplication(fixedAppId) != nullptr) {
      info->appId = fixedAppId;
      return;
    }

    // E.g. GIMP
    fixedAppId = id.mid(0, id.indexOf("-")).toLower().toStdString();
    if (applicationMenuConfig_.findApplication(fixedAppId) != nullptr) {
      info->appId = fixedAppId;
      return;
    }
  }
}

/* static */ void WindowSystem::state_changed(
    void *data,
    struct org_kde_plasma_window* window,
    uint32_t flags) {
  if (windows_.count(window) == 0) {
    return;
  }
  WindowInfo* info = windows_[window];
  if (info) {
    info->skipTaskbar = flags & ORG_KDE_PLASMA_WINDOW_MANAGEMENT_STATE_SKIPTASKBAR;
    info->onAllDesktops = flags & ORG_KDE_PLASMA_WINDOW_MANAGEMENT_STATE_ON_ALL_DESKTOPS;
    info->demandsAttention = flags & ORG_KDE_PLASMA_WINDOW_MANAGEMENT_STATE_DEMANDS_ATTENTION;
    info->minimized = flags & ORG_KDE_PLASMA_WINDOW_MANAGEMENT_STATE_MINIMIZED;

    if (info->minimized && activeUuid_ == info->uuid) {
      activeUuid_ = "";
      emit self()->activeWindowChanged(activeUuid_);
    } else if (flags & ORG_KDE_PLASMA_WINDOW_MANAGEMENT_STATE_ACTIVE && activeUuid_ != info->uuid) {
      activeUuid_ = info->uuid;
      emit self()->activeWindowChanged(activeUuid_);
    }
  }
}

/* static */ void WindowSystem::virtual_desktop_changed(
    void *data,
    struct org_kde_plasma_window* window,
    int32_t number) {
  // Ignore.
}

/* static */ void WindowSystem::themed_icon_name_changed(
    void *data,
    struct org_kde_plasma_window* window,
    const char *name) {

}

/* static */ void WindowSystem::unmapped(
    void *data,
    struct org_kde_plasma_window* window) {
  if (windows_.count(window) == 0) {
    return;
  }
  WindowInfo* info = windows_[window];
  if (info) {
    emit self()->windowRemoved(info->uuid);
    delete info;
    info = nullptr;
  }
}

/* static */ void WindowSystem::initial_state(
    void *data, struct org_kde_plasma_window* window) {
  if (windows_.count(window) == 0) {
    return;
  }
  WindowInfo* info = windows_[window];
  if (info) {
    emit self()->windowAdded(info);
  }
}

/* static */ void WindowSystem::parent_window(
    void *data,
    struct org_kde_plasma_window* window,
    struct org_kde_plasma_window *parent) {

}

/* static */ void WindowSystem::geometry(
    void *data,
    struct org_kde_plasma_window* window,
    int32_t x,
    int32_t y,
    uint32_t width,
    uint32_t height) {
  if (windows_.count(window) == 0) {
    return;
  }
  WindowInfo* info = windows_[window];
  if (info) {
    info->x = x;
    info->y = y;
    info->width = width;
    info->height = height;
  }
}

/* static */ void WindowSystem::icon_changed(
    void *data,
    struct org_kde_plasma_window* window) {

}

/* static */ void WindowSystem::pid_changed(
    void *data,
    struct org_kde_plasma_window* window,
    uint32_t pid) {

}

/* static */ void WindowSystem::virtual_desktop_entered(
    void *data,
    struct org_kde_plasma_window* window,
    const char *id) {
  if (windows_.count(window) == 0) {
    return;
  }
  WindowInfo* info = windows_[window];
  if (info) {
    info->desktop = id;
  }
}

/* static */ void WindowSystem::virtual_desktop_left(
    void *data,
    struct org_kde_plasma_window* window,
    const char *id) {
  if (id != currentDesktop_) {
    return;
  }
  if (windows_.count(window) == 0) {
    return;
  }
  WindowInfo* info = windows_[window];
  if (info && !info->onAllDesktops) {
    emit self()->windowLeftCurrentDesktop(info->uuid);
  }
}

/* static */ void WindowSystem::application_menu(
    void *data,
    struct org_kde_plasma_window* window,
    const char *service_name,
    const char *object_path) {

}

/* static */ void WindowSystem::activity_entered(
    void *data,
    struct org_kde_plasma_window* window,
    const char *id) {
  if (windows_.count(window) == 0) {
    return;
  }
  WindowInfo* info = windows_[window];
  if (info) {
    info->activity = id;
  }
}

/* static */ void WindowSystem::activity_left(
    void *data,
    struct org_kde_plasma_window* window,
    const char *id) {
  if (id != WindowSystem::currentActivity()) {
    return;
  }
  if (windows_.count(window) == 0) {
    return;
  }
  WindowInfo* info = windows_[window];
  if (info) {
    emit self()->windowLeftCurrentActivity(info->uuid);
  }
}

/* static */ void WindowSystem::resource_name_changed(
    void *data,
    struct org_kde_plasma_window* window,
    const char *resource_name) {

}

}  // namespace crystaldock
