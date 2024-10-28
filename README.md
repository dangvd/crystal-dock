![Crystal Dock](https://github.com/dangvd/crystal-dock/raw/main/images/crystal-dock.jpg)

# Crystal Dock

[Releases](https://github.com/dangvd/crystal-dock/releases)
[Screenshots](https://github.com/dangvd/crystal-dock/wiki/Screenshots)
[Documentation](https://github.com/dangvd/crystal-dock/wiki/Documentation)
[Author & Contributors](https://github.com/dangvd/crystal-dock/wiki/Author-&-Contributors)

Crystal Dock is a cool dock (desktop panel) for Linux desktop, with the focus on attractive user interface, being simple and easy to customize, and cross-desktop support.

The current version (version 2) supports KDE Plasma 6 on Wayland. Other desktop environments will be considered when they run on Wayland and provide sufficient APIs. The previous version (version 1) supports KDE Plasma 5, GNOME, LXQt, Cinnamon and MATE on X11.

## Main features

- Smooth parabolic zooming and translucent effect
- Three visual styles: Glass 3D, Flat 2D and Metal 2D
- Supported components: Application Menu, Pager, Launcher/Task Manager, Clock
- Multiple docks support
- Integration with various desktop environments: special menu entries (e.g. Log Out), specific default launchers, setting wallpapers
- Support for setting different wallpapers for different virtual desktops
- Separate configs for separate desktop environments

## Recommended icon themes

For Glass 3D / Metal 2D style, a good icon theme to use with Crystal Dock is the Crystal Remix icon theme: https://github.com/dangvd/crystal-remix-icon-theme

For Flat 2D style, use something like Mkos-Big-Sur.

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

Note that on Wayland, Crystal Dock needs to be installed in order to be able to access necessary Wayland protocols on KDE Plasma 6.

To uninstall, navigate to the cloned repository and run:

```
$ sudo cmake --build build --target uninstall
```

To execute the automated tests, run:
```
$ ctest --test-dir build
```
