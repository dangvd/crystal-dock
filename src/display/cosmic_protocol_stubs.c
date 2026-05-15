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

/*
 * Stub wl_interface symbols for protocols that cosmic-toplevel-info and
 * cosmic-toplevel-management reference in their XML but that we never
 * actually use at runtime. We bind v1 of zcosmic_toplevel_info_v1, which
 * means we never call get_cosmic_toplevel (so ext_foreign_toplevel_handle_v1
 * is unused) and we never handle workspace_enter/leave events meaningfully
 * (so the workspace handle types are unused). Providing empty interface
 * structs is sufficient to satisfy the linker; the runtime never dispatches
 * on these because we don't bind the relevant protocols.
 */

#include <wayland-client.h>

WL_EXPORT const struct wl_interface ext_foreign_toplevel_handle_v1_interface = {
    "ext_foreign_toplevel_handle_v1", 1, 0, NULL, 0, NULL
};

WL_EXPORT const struct wl_interface zcosmic_workspace_handle_v1_interface = {
    "zcosmic_workspace_handle_v1", 1, 0, NULL, 0, NULL
};

WL_EXPORT const struct wl_interface ext_workspace_handle_v1_interface = {
    "ext_workspace_handle_v1", 1, 0, NULL, 0, NULL
};
