![Crystal Dock](https://github.com/dangvd/crystal-dock/raw/main/images/crystal-dock.jpg)

# Crystal Dock

[Releases](https://github.com/dangvd/crystal-dock/releases)
[Documentation](https://github.com/dangvd/crystal-dock/wiki/Documentation)
[FAQ & Troubleshooting](https://github.com/dangvd/crystal-dock/wiki/FAQ-&-Troubleshooting)
[Author & Contributors](https://github.com/dangvd/crystal-dock/wiki/Author-&-Contributors)

Crystal Dock is a cool dock (desktop panel) for Linux desktop, with the focus on attractive user interface, simplicity and cross-desktop support.

The current version (version 2) supports Hyprland, KDE Plasma 6, Labwc, LXQt, Niri and Wayfire on Wayland. Other desktop environments and compositors will be considered when they run on Wayland and provide sufficient APIs. The previous version (version 1) supports KDE Plasma 5, GNOME, LXQt, Cinnamon and MATE on X11.

## Main features

- Smooth parabolic zooming and translucent effect
- Four visual styles: Glass 3D, Glass 2D, Flat 2D and Metal 2D with various appearance settings
- Supported components: Application Menu (Application Launcher), Launcher/Task Manager, Trash, Wi-Fi Manager, Volume Control, Version Checker, Clock and (on some environments) Pager
- Multiple docks support
- Integration with various desktop environments / compositors: specific default launchers, special menu entries (e.g. Log Out)
- Separate configs for separate desktop environments / compositors

## Icon theme

Crystal Dock simply uses the system icon theme.
The one shown in the screenshots is Crystal Remix icon theme: https://github.com/dangvd/crystal-remix-icon-theme

## License

Crystal Dock is licensed under the GNU General Public License v3.0

## Dependencies

Crystal Dock is written in C++ and depends on:
- Qt6 as the GUI framework
- LayerShellQt6 for Wayland's Layer Shell integration

---

## Build from the source code

### Build dependencies

Dependencies development packages: to build from the source code, Qt6, LayerShellQt6 and Wayland development packages are required.

For example, to install them on OpenSUSE, run:

```
$ sudo zypper install qt6-base-private-devel wayland-devel layer-shell-qt6-devel
```

To install them on Fedora, run:

```
$ sudo dnf install qt6-qtbase-private-devel wayland-devel layer-shell-qt-devel
```

To install them on Ubuntu, run:

```
$ sudo apt install qt6-base-private-dev libwayland-dev liblayershellqtinterface-dev
```

### Build commands

To build and install, run:

```
$ cmake -S src -B build -DCMAKE_INSTALL_PREFIX=/usr
$ cmake --build build --parallel
$ sudo cmake --install build
```

After the installation, Crystal Dock can be launched from the Application list (Utilities category), or from the command line:
```
$ crystal-dock
```

Note that on KWin, Crystal Dock needs to be installed in order to be able to access necessary KDE Wayland protocols.

To uninstall, navigate to the cloned repository and run:

```
$ sudo cmake --build build --target uninstall
```

To execute the automated tests, run:
```
$ ctest --test-dir build
```
