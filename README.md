# fcitx5-lotus-osk

Standalone on-screen keyboard extracted from `fcitx5-lotus`.

This repository keeps the Qt/LayerShell UI app separate while remaining
compatible with the OSK support code kept inside `fcitx5-lotus`.

## Compatibility contract

- Executable name: `fcitx5-lotus-osk`
- D-Bus service: `app.lotus.Osk`
- D-Bus object path: `/app/lotus/Osk/Controller`
- D-Bus interface: `app.lotus.Osk.Controller1`
- Unix socket protocol: `LotusKeyCommand` from [`src/lotus-key-command.h`](src/lotus-key-command.h)

`fcitx5-lotus` currently expects this executable name when authorizing OSK
connections on the privileged server side.

## Build

```bash
cmake -S . -B build
cmake --build build -j
```

## Dependencies

- Qt6: `Core`, `Gui`, `Widgets`, `DBus`
- `LayerShellQt` (optional, recommended on KDE Wayland)

## Install

```bash
cmake -S . -B build
cmake --build build -j
sudo cmake --install build
```

Installing also places the `app.lotus.Osk` D-Bus service file under the normal
`dbus-1/services` location.
