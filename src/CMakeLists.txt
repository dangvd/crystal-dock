cmake_minimum_required(VERSION 3.20)

project(crystal-dock)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# When doing code clean-up, uncomment this.
# add_compile_options(-Wall -Werror)
# When doing code clean-up, comment this.
add_definitions(-DQT_NO_DEPRECATED_WARNINGS)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 6.6 REQUIRED COMPONENTS DBus Gui Test Widgets)
if (Qt6_VERSION VERSION_GREATER_EQUAL 6.9.0)
  set(QT_NO_PRIVATE_MODULE_WARNING ON)
  find_package(Qt6 REQUIRED COMPONENTS GuiPrivate)
endif()
find_package(Wayland 1.22 REQUIRED COMPONENTS Client)
find_package(LayerShellQt 6.0 REQUIRED)

set(SRCS
    desktop/desktop_env.cc
    desktop/budgie_desktop_env.cc
    desktop/hyprland_desktop_env.cc
    desktop/kde_desktop_env.cc
    desktop/labwc_desktop_env.cc
    desktop/lxqt_desktop_env.cc
    desktop/niri_desktop_env.cc
    desktop/sway_desktop_env.cc
    desktop/wayfire_desktop_env.cc
    display/window_system.cc
    display/kde_auto_hide_manager.cc
    display/kde_virtual_desktop_manager.cc
    display/kde_window_manager.cc
    display/kde_screen_edge.c
    display/plasma_virtual_desktop.c
    display/plasma_window_management.c
    display/wlr_foreign_toplevel_management.c
    display/wlr_window_manager.cc
    model/application_menu_config.cc
    model/config_helper.cc
    model/launcher_config.cc
    model/multi_dock_model.cc
    view/add_panel_dialog.cc
    view/appearance_settings_dialog.cc
    view/application_menu_settings_dialog.cc
    view/application_menu.cc
    view/battery_indicator.cc
    view/calendar.cc
    view/clock.cc
    view/color_button.cc
    view/desktop_selector.cc
    view/dock_item.cc
    view/dock_panel.cc
    view/edit_keyboard_layouts_dialog.cc
    view/edit_launchers_dialog.cc
    view/icon_based_dock_item.cc
    view/icon_button.cc
    view/iconless_dock_item.cc
    view/keyboard_layout.cc
    view/multi_dock_view.cc
    view/program.cc
    view/separator.cc
    view/trash.cc
    view/version_checker.cc
    view/volume_control.cc
    view/task_manager_settings_dialog.cc
    view/wallpaper_settings_dialog.cc
    view/wifi_connection_dialog.cc
    view/wifi_manager.cc
    utils/desktop_file.cc
    desktop/desktop_env.h
    desktop/budgie_desktop_env.h
    desktop/hyprland_desktop_env.h
    desktop/kde_desktop_env.h
    desktop/labwc_desktop_env.h
    desktop/lxqt_desktop_env.h
    desktop/niri_desktop_env.h
    desktop/sway_desktop_env.h
    desktop/wayfire_desktop_env.h
    display/window_system.h
    display/kde_auto_hide_manager.h
    display/kde_virtual_desktop_manager.h
    display/kde_window_manager.h
    display/kde_screen_edge.h
    display/plasma_virtual_desktop.h
    display/plasma_window_management.h
    display/wlr_foreign_toplevel_management.h
    display/wlr_window_manager.h
    model/application_menu_config.h
    model/application_menu_entry.h
    model/config_helper.h
    model/launcher_config.h
    model/multi_dock_model.h
    view/add_panel_dialog.h
    view/appearance_settings_dialog.h
    view/application_menu_settings_dialog.h
    view/application_menu.h
    view/battery_indicator.h
    view/calendar.h
    view/clock.h
    view/color_button.h
    view/desktop_selector.h
    view/dock_item.h
    view/dock_panel.h
    view/edit_keyboard_layouts_dialog.h
    view/edit_launchers_dialog.h
    view/icon_based_dock_item.h
    view/icon_button.h
    view/iconless_dock_item.h
    view/keyboard_layout.h
    view/multi_dock_view.h
    view/program.h
    view/separator.h
    view/trash.h
    view/version_checker.h
    view/volume_control.h
    view/task_manager_settings_dialog.h
    view/wallpaper_settings_dialog.h
    view/wifi_connection_dialog.h
    view/wifi_manager.h
    utils/command_utils.h
    utils/desktop_file.h
    utils/draw_utils.h
    utils/font_utils.h
    utils/icon_utils.h
    utils/math_utils.h
    utils/menu_utils.h
    view/add_panel_dialog.ui
    view/appearance_settings_dialog.ui
    view/application_menu_settings_dialog.ui
    view/edit_keyboard_layouts_dialog.ui
    view/edit_launchers_dialog.ui
    view/task_manager_settings_dialog.ui
    view/wallpaper_settings_dialog.ui
    view/wifi_connection_dialog.ui)
add_library(crystal-dock_lib STATIC ${SRCS})

set(LIBS Qt6::DBus Qt6::GuiPrivate Qt6::Widgets Wayland::Client LayerShellQt::Interface)
target_link_libraries(crystal-dock_lib ${LIBS})

add_executable(crystal-dock main.cc)
target_link_libraries(crystal-dock crystal-dock_lib ${LIBS})

configure_file(crystal-dock.desktop.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/crystal-dock.desktop @ONLY)

# Install

install(TARGETS crystal-dock RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/crystal-dock.desktop
    DESTINATION ${CMAKE_INSTALL_PREFIX}/share/applications)

# Uninstall

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    @ONLY)

add_custom_target(uninstall COMMAND ${CMAKE_COMMAND}
    -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)

# Test

enable_testing()

add_executable(application_menu_config_test model/application_menu_config_test.cc)
target_link_libraries(application_menu_config_test Qt6::Test crystal-dock_lib ${LIBS})
add_test(application_menu_config_test application_menu_config_test)
