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

#ifndef KDE_VIRTUAL_DESKTOP_MANAGER_H_
#define KDE_VIRTUAL_DESKTOP_MANAGER_H_

#include <string>

#include "plasma_virtual_desktop.h"
#include "window_system.h"

namespace crystaldock {

class KdeVirtualDesktopManager : public QObject {
  Q_OBJECT

 private:
  KdeVirtualDesktopManager() = default;

 signals:
  void currentDesktopChanged(std::string_view);
  void numberOfDesktopsChanged(int);
  void desktopNameChanged(std::string_view desktopId, std::string_view desktopName);

 public:
  static KdeVirtualDesktopManager* self();
  static void init(struct org_kde_plasma_virtual_desktop_management* virtual_desktop_management);

  static void bindVirtualDesktopManagerFunctions(VirtualDesktopManager* virtualDesktopManager);

  static int numberOfDesktops();
  static std::vector<VirtualDesktopInfo> desktops() { return desktops_; }
  static std::string_view currentDesktop();
  static void setCurrentDesktop(std::string_view);

 private:

  // org_kde_plasma_virtual_desktop_management interface.

  static void desktop_management_desktop_created(
      void* data, org_kde_plasma_virtual_desktop_management* virtual_desktop_management,
      const char* desktop_id, uint32_t position);
  static void desktop_management_desktop_removed(
      void* data, org_kde_plasma_virtual_desktop_management* virtual_desktop_management,
      const char* desktop_id);
  static void desktop_management_done(void *data,
                                   struct org_kde_plasma_virtual_desktop_management* virtual_desktop_management);
  static void desktop_management_desktop_rows(
      void* data, org_kde_plasma_virtual_desktop_management* virtual_desktop_management,
      uint32_t rows);

  // org_kde_plasma_virtual_desktop interface.

  static void desktop_id(
      void *data,
      struct org_kde_plasma_virtual_desktop *org_kde_plasma_virtual_desktop,
      const char *desktop_id);

  static void desktop_name(
      void *data,
      struct org_kde_plasma_virtual_desktop *org_kde_plasma_virtual_desktop,
      const char *name);

  static void desktop_activated(
      void *data,
      struct org_kde_plasma_virtual_desktop *org_kde_plasma_virtual_desktop);

  static void desktop_deactivated(
      void *data,
      struct org_kde_plasma_virtual_desktop *org_kde_plasma_virtual_desktop);

  static void desktop_done(
      void *data,
      struct org_kde_plasma_virtual_desktop *org_kde_plasma_virtual_desktop);

  static void desktop_removed(
      void *data,
      struct org_kde_plasma_virtual_desktop *org_kde_plasma_virtual_desktop);

  static constexpr struct org_kde_plasma_virtual_desktop_management_listener
      virtual_desktop_management_listener_ = {
          desktop_management_desktop_created,
          desktop_management_desktop_removed,
          desktop_management_done,
          desktop_management_desktop_rows
      };
  static constexpr struct org_kde_plasma_virtual_desktop_listener virtual_desktop_listener_ = {
      desktop_id,
      desktop_name,
      desktop_activated,
      desktop_deactivated,
      desktop_done,
      desktop_removed
  };

  static org_kde_plasma_virtual_desktop_management* virtual_desktop_management_;

  static std::vector<VirtualDesktopInfo> desktops_;
  // Current desktop ID.
  static std::string currentDesktop_;
};

}

#endif  // KDE_VIRTUAL_DESKTOP_MANAGER_H_
