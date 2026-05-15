/*
 * This file is part of Crystal Dock.
 * Copyright (C) 2026 Crystal Dock contributors
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

#ifndef COSMIC_WINDOW_MANAGER_H_
#define COSMIC_WINDOW_MANAGER_H_

#include <memory>
#include <string>

#include "window_system.h"
#include "cosmic_toplevel_info.h"
#include "cosmic_toplevel_management.h"

namespace crystaldock {

// Window manager implementation for the COSMIC desktop using the
// zcosmic_toplevel_info_v1 (v1 of the protocol, which still emits
// `toplevel` events directly) and zcosmic_toplevel_manager_v1
// protocols. We deliberately bind v1 of toplevel_info so we receive
// toplevel events natively instead of having to go through
// ext_foreign_toplevel_list_v1.
class CosmicWindowManager : public QObject {
  Q_OBJECT

 private:
  CosmicWindowManager() = default;

 signals:
  void windowAdded(const WindowInfo*);
  void windowRemoved(void*);
  void windowLeftCurrentDesktop(void*);
  void windowGeometryChanged(const WindowInfo*);
  void windowStateChanged(const WindowInfo*);
  void windowTitleChanged(const WindowInfo*);
  void activeWindowChanged(void*);

  void windowEnteredOutput(const WindowInfo*, const wl_output*);
  void windowLeftOutput(const WindowInfo*, const wl_output*);

 public:
  static CosmicWindowManager* self();
  static void init(struct zcosmic_toplevel_info_v1* info,
                   struct zcosmic_toplevel_manager_v1* manager);

  static void bindWindowManagerFunctions(WindowManager* windowManager);

  static std::vector<const WindowInfo*> windows();
  static void* activeWindow();
  static void resetActiveWindow();
  static void activateWindow(void* window);
  static void activateOrMinimizeWindow(void* window);
  static void minimizeWindow(void* window);
  static void closeWindow(void* window);
  static bool showingDesktop();
  static void setShowingDesktop(bool show);

 private:

  // zcosmic_toplevel_info_v1 interface.

  static void info_toplevel(
      void *data,
      struct zcosmic_toplevel_info_v1 *info,
      struct zcosmic_toplevel_handle_v1 *toplevel);

  static void info_finished(
      void *data,
      struct zcosmic_toplevel_info_v1 *info);

  static void info_done(
      void *data,
      struct zcosmic_toplevel_info_v1 *info);

  // zcosmic_toplevel_handle_v1 interface.

  static void handle_closed(
      void *data,
      struct zcosmic_toplevel_handle_v1 *handle);

  static void handle_done(
      void *data,
      struct zcosmic_toplevel_handle_v1 *handle);

  static void handle_title(
      void *data,
      struct zcosmic_toplevel_handle_v1 *handle,
      const char *title);

  static void handle_app_id(
      void *data,
      struct zcosmic_toplevel_handle_v1 *handle,
      const char *app_id);

  static void handle_output_enter(
      void *data,
      struct zcosmic_toplevel_handle_v1 *handle,
      struct wl_output *output);

  static void handle_output_leave(
      void *data,
      struct zcosmic_toplevel_handle_v1 *handle,
      struct wl_output *output);

  static void handle_workspace_enter(
      void *data,
      struct zcosmic_toplevel_handle_v1 *handle,
      struct zcosmic_workspace_handle_v1 *workspace);

  static void handle_workspace_leave(
      void *data,
      struct zcosmic_toplevel_handle_v1 *handle,
      struct zcosmic_workspace_handle_v1 *workspace);

  static void handle_state(
      void *data,
      struct zcosmic_toplevel_handle_v1 *handle,
      struct wl_array *state);

  static void handle_geometry(
      void *data,
      struct zcosmic_toplevel_handle_v1 *handle,
      struct wl_output *output,
      int32_t x,
      int32_t y,
      int32_t width,
      int32_t height);

  static void handle_ext_workspace_enter(
      void *data,
      struct zcosmic_toplevel_handle_v1 *handle,
      struct ext_workspace_handle_v1 *workspace);

  static void handle_ext_workspace_leave(
      void *data,
      struct zcosmic_toplevel_handle_v1 *handle,
      struct ext_workspace_handle_v1 *workspace);

  // Listener tables. The toplevel handle listener has every callback the
  // protocol declares (the compositor will not call a function it doesn't
  // need, but the table size must match what the generated code expects).
  static constexpr struct zcosmic_toplevel_info_v1_listener info_listener_ = {
      info_toplevel,
      info_finished,
      info_done,
  };
  static constexpr struct zcosmic_toplevel_handle_v1_listener handle_listener_ = {
      handle_closed,
      handle_done,
      handle_title,
      handle_app_id,
      handle_output_enter,
      handle_output_leave,
      handle_workspace_enter,
      handle_workspace_leave,
      handle_state,
      handle_geometry,
      handle_ext_workspace_enter,
      handle_ext_workspace_leave,
  };

  static struct zcosmic_toplevel_info_v1* info_;
  static struct zcosmic_toplevel_manager_v1* manager_;

  static std::unordered_map<struct zcosmic_toplevel_handle_v1*,
                            std::unique_ptr<WindowInfo>> windows_;
  static struct zcosmic_toplevel_handle_v1* activeWindow_;
  static struct zcosmic_toplevel_handle_v1* activeWindowBeforeShowDesktop_;
  static bool showingDesktop_;
};

}  // namespace crystaldock

#endif  // COSMIC_WINDOW_MANAGER_H_
