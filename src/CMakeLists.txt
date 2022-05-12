cmake_minimum_required(VERSION 3.16)

project(crystal-dock)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Werror)
add_definitions(-DQT_DEPRECATED_WARNINGS)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

find_package(ECM REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

find_package(Qt5 5.11 REQUIRED COMPONENTS DBus Gui Test Widgets)
find_package(KF5 5.7 REQUIRED COMPONENTS WindowSystem)

set(SRCS
    desktop/cinnamon_desktop_env.cc
    desktop/desktop_env.cc
    desktop/gnome_desktop_env.cc
    desktop/kde_desktop_env.cc
    model/application_menu_config.cc
    model/config_helper.cc
    model/launcher_config.cc
    model/multi_dock_model.cc
    view/add_panel_dialog.cc
    view/appearance_settings_dialog.cc
    view/application_menu_settings_dialog.cc
    view/application_menu.cc
    view/calendar.cc
    view/clock.cc
    view/color_button.cc
    view/desktop_selector.cc
    view/dock_panel.cc
    view/edit_launchers_dialog.cc
    view/icon_based_dock_item.cc
    view/icon_button.cc
    view/iconless_dock_item.cc
    view/multi_dock_view.cc
    view/program.cc
    view/separator.cc
    view/task_manager_settings_dialog.cc
    view/tooltip.cc
    view/wallpaper_settings_dialog.cc
    utils/desktop_file.cc
    utils/task_helper.cc
    desktop/cinnamon_desktop_env.h
    desktop/desktop_env.h
    desktop/gnome_desktop_env.h
    desktop/kde_desktop_env.h
    model/application_menu_config.h
    model/application_menu_entry.h
    model/config_helper.h
    model/launcher_config.h
    model/multi_dock_model.h
    view/add_panel_dialog.h
    view/appearance_settings_dialog.h
    view/application_menu_settings_dialog.h
    view/application_menu.h
    view/calendar.h
    view/clock.h
    view/color_button.h
    view/desktop_selector.h
    view/dock_item.h
    view/dock_panel.h
    view/edit_launchers_dialog.h
    view/icon_based_dock_item.h
    view/icon_button.h
    view/iconless_dock_item.h
    view/multi_dock_view.h
    view/program.h
    view/separator.h
    view/task_manager_settings_dialog.h
    view/tooltip.h
    view/wallpaper_settings_dialog.h
    utils/command_utils.h
    utils/desktop_file.h
    utils/draw_utils.h
    utils/font_utils.h
    utils/task_helper.h)
add_library(crystal-dock_lib ${SRCS})

set(LIBS Qt5::DBus Qt5::Gui Qt5::Widgets KF5::WindowSystem stdc++fs)
target_link_libraries(crystal-dock_lib ${LIBS})

add_executable(crystal-dock main.cc)
target_link_libraries(crystal-dock crystal-dock_lib ${LIBS})

# Install

install(TARGETS crystal-dock RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES crystal-dock.desktop DESTINATION ${CMAKE_INSTALL_PREFIX}/share/applications)

# Uninstall

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall COMMAND ${CMAKE_COMMAND}
    -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)

