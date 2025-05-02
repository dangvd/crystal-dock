/*
 * This file is part of Crystal Dock.
 * Copyright (C) 2025 Viet Dang (dangvd@gmail.com)
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

#include "kde_virtual_desktop_manager.h"

namespace crystaldock {

org_kde_plasma_virtual_desktop_management* KdeVirtualDesktopManager::virtual_desktop_management_;
std::vector<VirtualDesktopInfo> KdeVirtualDesktopManager::desktops_;
std::string KdeVirtualDesktopManager::currentDesktop_;

/* static */ KdeVirtualDesktopManager* KdeVirtualDesktopManager::self() {
  static KdeVirtualDesktopManager self;
  return &self;
}

/* static */ void KdeVirtualDesktopManager::init(
    struct org_kde_plasma_virtual_desktop_management* virtual_desktop_management) {
  virtual_desktop_management_ = virtual_desktop_management;
  org_kde_plasma_virtual_desktop_management_add_listener(
      virtual_desktop_management_, &virtual_desktop_management_listener_, NULL);
  connect(KdeVirtualDesktopManager::self(), &KdeVirtualDesktopManager::currentDesktopChanged,
      WindowSystem::self(), &WindowSystem::currentDesktopChanged);
  connect(KdeVirtualDesktopManager::self(), &KdeVirtualDesktopManager::desktopNameChanged,
      WindowSystem::self(), &WindowSystem::desktopNameChanged);
  connect(KdeVirtualDesktopManager::self(), &KdeVirtualDesktopManager::numberOfDesktopsChanged,
      WindowSystem::self(), &WindowSystem::numberOfDesktopsChanged);
}

/* static */ void KdeVirtualDesktopManager::bindVirtualDesktopManagerFunctions(
    VirtualDesktopManager* virtualDesktopManager) {
  virtualDesktopManager->currentDesktop = KdeVirtualDesktopManager::currentDesktop;
  virtualDesktopManager->desktops = KdeVirtualDesktopManager::desktops;
  virtualDesktopManager->numberOfDesktops = KdeVirtualDesktopManager::numberOfDesktops;
  virtualDesktopManager->setCurrentDesktop = KdeVirtualDesktopManager::setCurrentDesktop;
}

/* static */ int KdeVirtualDesktopManager::numberOfDesktops() {
  return desktops_.size();
}

/* static */ std::string_view KdeVirtualDesktopManager::currentDesktop() {
  return currentDesktop_;
}

/* static */ void KdeVirtualDesktopManager::setCurrentDesktop(std::string_view desktopId) {
  auto pos = std::find_if(desktops_.begin(), desktops_.end(),
               [desktopId](auto& e) { return e.id == desktopId; });
  if (pos != desktops_.end()) {
    auto desktop = static_cast<struct org_kde_plasma_virtual_desktop*>(pos->virtual_desktop);
    if (desktop != nullptr) {
      org_kde_plasma_virtual_desktop_request_activate(desktop);
    }
  }
}

// org_kde_plasma_virtual_desktop_management interface.

/* static */ void KdeVirtualDesktopManager::desktop_management_desktop_created(
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

/* static */ void KdeVirtualDesktopManager::desktop_management_desktop_removed(
    void* data, org_kde_plasma_virtual_desktop_management* virtual_desktop_management,
    const char* desktop_id) {
  desktops_.erase(std::find_if(desktops_.begin(), desktops_.end(),
                               [desktop_id](auto& e) { return e.id == desktop_id; }));
  for (unsigned int pos = 0; pos < desktops_.size(); ++pos) {
    desktops_[pos].number = pos + 1;
  }
  emit self()->numberOfDesktopsChanged(desktops_.size());
}

/* static */ void KdeVirtualDesktopManager::desktop_management_done(void *data,
          struct org_kde_plasma_virtual_desktop_management* virtual_desktop_management) {
  // Ignore.
}

/* static */ void KdeVirtualDesktopManager::desktop_management_desktop_rows(
    void* data, org_kde_plasma_virtual_desktop_management* virtual_desktop_management,
    uint32_t rows) {
  // Ignore.
}

// org_kde_plasma_virtual_desktop interface.

/* static */ void KdeVirtualDesktopManager::desktop_id(
    void *data,
    struct org_kde_plasma_virtual_desktop *virtual_desktop,
    const char *desktop_id) {
  auto pos = std::find_if(desktops_.begin(), desktops_.end(),
               [virtual_desktop](auto& e) { return e.virtual_desktop == virtual_desktop; });
  if (pos != desktops_.end()) {
    pos->id = desktop_id;
  }
}

/* static */ void KdeVirtualDesktopManager::desktop_name(
    void *data,
    struct org_kde_plasma_virtual_desktop *virtual_desktop,
    const char *name) {
  auto pos = std::find_if(desktops_.begin(), desktops_.end(),
               [virtual_desktop](auto& e) { return e.virtual_desktop == virtual_desktop; });
  if (pos != desktops_.end()) {
    pos->name = name;
    emit KdeVirtualDesktopManager::self()->desktopNameChanged(pos->id, pos->name);
  }
}

/* static */ void KdeVirtualDesktopManager::desktop_activated(
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

/* static */ void KdeVirtualDesktopManager::desktop_deactivated(
    void *data,
    struct org_kde_plasma_virtual_desktop *org_kde_plasma_virtual_desktop) {
  // Ignore.
}

/* static */ void KdeVirtualDesktopManager::desktop_done(
    void *data,
    struct org_kde_plasma_virtual_desktop *org_kde_plasma_virtual_desktop) {
  // Ignore.
}

/* static */ void KdeVirtualDesktopManager::desktop_removed(
    void *data,
    struct org_kde_plasma_virtual_desktop *virtual_desktop) {
  // Ignore.
}


}  // namespace crystaldock
