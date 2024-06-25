![Crystal Dock](https://github.com/dangvd/crystal-dock/raw/main/images/crystal-dock.jpg)

# Crystal Dock

[Releases](https://github.com/dangvd/crystal-dock/releases)
[Documentation](https://github.com/dangvd/crystal-dock/wiki/Documentation)
[Screenshots](https://github.com/dangvd/crystal-dock/wiki/Screenshots)

Crystal Dock is a cool dock (desktop panel) for Linux desktop, with the focus on attractive user interface, being simple and easy to use and customize, and cross-desktop support.

The current version (version 2) supports KDE Plasma 6 on Wayland. Other desktop environments will be considered when they run on Wayland and provide sufficient APIs. The previous version (version 1) supports KDE Plasma 5, GNOME, LXQt, Cinnamon and MATE on X11.

## Main features

- Smooth parabolic zooming and translucent effect
- Supported components: Application Menu, Pager, Programs/Task Manager, Clock
- Multiple docks support
- Integration with various desktop environments: special menu entries (e.g. Log Out), specific default launchers, setting wallpapers
- Support for setting different wallpapers for different virtual desktops
- Separate configs for separate desktop environments

## Recommended icon theme

A good icon theme to use with Crystal Dock is the Crystal Remix icon theme: https://github.com/dangvd/crystal-remix-icon-theme

## License

Crystal Dock is licensed under the GNU General Public License v3.0

## Dependencies

Crystal Dock is written in C++ and depends on:
- Qt6 as the GUI framework
- LayerShellQt6 for Wayland's Layer Shell integration

---

## Build from the source code

Dependencies development packages: to build from the source code, Qt6, LayerShellQt6 and Wayland development packages are required.
For example, to install them on OpenSUSE, run:

```
$ sudo zypper install qt6-base-devel wayland-devel layer-shell-qt6-devel
```

To build and install, run:
```
$ mkdir -p build && cd build
$ cmake ../src && make && sudo make install
```

After the installation, Crystal Dock can be launched from the Application list (Utilities category), or from the command line:
```
$ crystal-dock
```

Note that on Wayland, Crystal Dock needs to be installed in order to be able to access necessary Wayland protocols on KDE Plasma 6.

To uninstall, run:
```
$ sudo make uninstall
```

To execute the automated tests, run:
```
$ make test
```
