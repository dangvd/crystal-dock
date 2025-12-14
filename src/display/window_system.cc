#include "window_system.h"

#include <algorithm>
#include <iostream>
#include <string>

#include <QDBusReply>
#include <QGuiApplication>

#include <LayerShellQt/Shell>
#include <LayerShellQt/Window>

#include "kde_auto_hide_manager.h"
#include "kde_virtual_desktop_manager.h"
#include "kde_window_manager.h"
#include "wlr_window_manager.h"

namespace crystaldock {

namespace {

LayerShellQt::Window* getLayerShellWin(QWidget* widget) {
  QWindow* win = WindowSystem::getWindow(widget);
  if (win == nullptr) {
    std::cerr << "Null QWindow" << std::endl;
    return nullptr;
  }

  return LayerShellQt::Window::get(win);
}

}  // namespace

org_kde_plasma_virtual_desktop_management* WindowSystem::kde_virtual_desktop_management_;
org_kde_plasma_window_management* WindowSystem::kde_window_management_;
kde_screen_edge_manager_v1* WindowSystem::kde_screen_edge_manager_;

zwlr_foreign_toplevel_manager_v1* WindowSystem::wlr_window_manager_;

VirtualDesktopManager WindowSystem::virtualDesktopManager_;
WindowManager WindowSystem::windowManager_;
AutoHideManager WindowSystem::autoHideManager_;

std::vector<QScreen*> WindowSystem::screens_;
std::unique_ptr<QDBusInterface> WindowSystem::activityManager_;

/* static */ WindowSystem* WindowSystem::self() {
  static WindowSystem self;
  return &self;
}

/* static */ bool WindowSystem::init(struct wl_display* display) {
  struct wl_registry *registry = wl_display_get_registry(display);
  wl_registry_add_listener(registry, &registry_listener_, NULL);

  // wait for the "initial" set of globals to appear
  wl_display_roundtrip(display);

  if (!kde_window_management_ && !wlr_window_manager_) {
    std::cerr << "Failed to bind required Wayland interfaces" << std::endl;
    return false;
  }

  if (kde_virtual_desktop_management_) {
    KdeVirtualDesktopManager::init(kde_virtual_desktop_management_);
    KdeVirtualDesktopManager::bindVirtualDesktopManagerFunctions(&virtualDesktopManager_);
  }

  if (kde_window_management_) {
    KdeWindowManager::init(kde_window_management_);
    KdeWindowManager::bindWindowManagerFunctions(&windowManager_);
  } else if (wlr_window_manager_) {
    WlrWindowManager::init(wlr_window_manager_);
    WlrWindowManager::bindWindowManagerFunctions(&windowManager_);
  }

  if (kde_screen_edge_manager_) {
    KdeAutoHideManager::init(kde_screen_edge_manager_);
    KdeAutoHideManager::bindAutoHideManagerFunctions(&autoHideManager_);
  }

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

  initScreens();

  return true;
}

/* static */ bool WindowSystem::hasVirtualDesktopManager() {
  return kde_virtual_desktop_management_ != nullptr;
}

/* static */ bool WindowSystem::hasAutoHideManager() {
  return kde_screen_edge_manager_ != nullptr;
}

/* static */ bool WindowSystem::hasActivityManager() {
  return activityManager_->isValid();
}

/* static */ void WindowSystem::setAnchorAndStrut(
    QWidget* widget, LayerShellQt::Window::Anchors anchors, uint32_t strutSize) {
  auto* layerShellWin = getLayerShellWin(widget);
  if (layerShellWin) {
    layerShellWin->setAnchors(anchors);
    layerShellWin->setExclusiveZone(strutSize);
    layerShellWin->setScreenConfiguration(LayerShellQt::Window::ScreenFromQWindow);
  }
}

/*static*/ void WindowSystem::setLayer(QWidget* widget, LayerShellQt::Window::Layer layer) {
  auto* layerShellWin = getLayerShellWin(widget);
  if (layerShellWin) {
    layerShellWin->setLayer(layer);
  }
}

void WindowSystem::initScreens() {
  screens_.clear();
  for (auto* screen : QGuiApplication::screens()) {
    screens_.push_back(screen);
  }
  std::sort(screens_.begin(), screens_.end(), [](QScreen* s1, QScreen* s2) {
    return s1->geometry().center().manhattanLength() < s2->geometry().center().manhattanLength();
  });
}

/*static*/ std::vector<QScreen*> WindowSystem::screens() {
  return screens_;
}

/*static*/ void WindowSystem::setScreen(QWidget* widget, int screen) {
  if (screen < 0 || screen >= static_cast<int>(screens_.size())) {
    return;
  }

  QWindow* win = getWindow(widget);
  if (win) {
    win->setScreen(screens_[screen]);
  }
}

/*static*/ wl_output* WindowSystem::getWlOutputForScreen(int screen) {
  if (screen < 0 || screen >= static_cast<int>(screens_.size())) {
    return nullptr;
  }

  auto* waylandScreen = screens_[screen]->nativeInterface<QNativeInterface::QWaylandScreen>();
  if (waylandScreen) {
    return waylandScreen->output();
  }

  return nullptr;
}

// wl_registry interface.

/* static */ void WindowSystem::registry_global(
    void* data,
    struct wl_registry* registry,
    uint32_t name,
    const char* interface,
    uint32_t version) {
  if (std::string(interface) == "org_kde_plasma_virtual_desktop_management") {
    kde_virtual_desktop_management_ =
        reinterpret_cast<org_kde_plasma_virtual_desktop_management*>(wl_registry_bind(
            registry, name, &org_kde_plasma_virtual_desktop_management_interface, 2));
    if (!kde_virtual_desktop_management_) {
      std::cerr << "Failed to bind org_kde_plasma_virtual_desktop_management Wayland interface"
                << std::endl;
    }
  } else if (std::string(interface) == "org_kde_plasma_window_management") {
    kde_window_management_ =
        reinterpret_cast<org_kde_plasma_window_management*>(wl_registry_bind(
            registry, name, &org_kde_plasma_window_management_interface, 16));
    if (!kde_window_management_) {
      std::cerr << "Failed to bind org_kde_plasma_window_management Wayland interface. "
                << "Maybe another client has already bound it?" << std::endl;
    }
  } else if (std::string(interface) == "kde_screen_edge_manager_v1") {
    kde_screen_edge_manager_ =
        reinterpret_cast<kde_screen_edge_manager_v1*>(wl_registry_bind(
            registry, name, &kde_screen_edge_manager_v1_interface, 1));
    if (!kde_screen_edge_manager_) {
      std::cerr << "Failed to bind kde_screen_edge_manager_v1 Wayland interface"
                << std::endl;
    }
  } else if (std::string(interface) == "zwlr_foreign_toplevel_manager_v1") {
    wlr_window_manager_ =
        reinterpret_cast<zwlr_foreign_toplevel_manager_v1*>(wl_registry_bind(
            registry, name, &zwlr_foreign_toplevel_manager_v1_interface, 3));
    if (!wlr_window_manager_) {
      std::cerr << "Failed to bind zwlr_foreign_toplevel_manager_v1 Wayland interface"
                << std::endl;
    }
  }
}

/* static */ void WindowSystem::registry_global_remove(
    void *data, struct wl_registry *wl_registry, uint32_t name) {
  // Ignore.
}

}  // namespace crystaldock
