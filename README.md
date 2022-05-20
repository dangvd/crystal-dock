![Crystal Dock](https://github.com/dangvd/crystal-dock/raw/main/crystal-dock.jpg)

# Crystal Dock

[Releases](https://github.com/dangvd/crystal-dock/releases)
[Documentation](https://github.com/dangvd/crystal-dock/wiki/Documentation)
[Screenshots](https://github.com/dangvd/crystal-dock/wiki/Screenshots)

Crystal Dock is a cool dock (desktop panel) for Linux desktop, with the focus on attractive user interface, being simple and easy to use, and cross-desktop support.

## Main features

- Smooth parabolic zooming effect
- Supported components: Application Menu, Pager, combined Launchers/Tasks, Clock
- Multiple docks support
- Integration with various desktop environments (KDE Plasma, GNOME, XFCE, MATE, Cinnamon, LXQT): special menu entries (e.g. Log Out), specific default launchers, setting wallpapers
- Support for separate wallpapers for separate virtual desktops
- Separate configs for separate desktop environments

## Recommended icon theme

A good icon theme to use with Crystal Dock is the Crystal Remix icon theme: https://github.com/dangvd/crystal-remix-icon-theme

## License

Crystal Dock is licensed under the GNU General Public License v3.0

## Dependencies

Crystal Dock is written in C++ and depends on:
- Qt5 as the GUI framework
- KWindowSystem as a high-level API to access the window system

---

## Build from the source code

Dependencies development packages: to build from the source code, Qt5 and KWindowSystem development packages are required.
- On Debian-based distributions, they can be installed by running:
```
$ sudo apt install extra-cmake-modules qtbase5-dev libkf5windowsystem-dev
```
- For Fedora, install the following packages: extra-cmake-modules qt5-devel kf5-kwindowsystem-devel

To build, run:
```
$ mkdir -p build && cd build && cmake ../src
$ make
```

To launch the program:
```
$ ./crystal-dock
```

To install, run:
```
$ sudo make install
```

After the installation, Crystal Dock can be launched from the Application Menu (Utilities category), or from the command line:
```
$ crystal-dock
```

To uninstall, run:
```
$ sudo make uninstall
```

To execute the automated tests, run:
```
$ make test
```
