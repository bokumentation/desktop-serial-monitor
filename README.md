# Crossplatform Desktop Serial Monitor App for my project

Use QT framework. Tested on Debian 13.

A cross-platform Qt6 serial monitor for ESP32-C3 with colored log output and real-time plotting.

[![Ubuntu](https://github.com/USER/REPO/actions/workflows/ci.yml/badge.svg?job=ubuntu-build)](https://github.com/USER/REPO/actions/workflows/ci.yml)
[![macOS](https://github.com/USER/REPO/actions/workflows/ci.yml/badge.svg?job=macos-build)](https://github.com/USER/REPO/actions/workflows/ci.yml)
[![Windows](https://github.com/USER/REPO/actions/workflows/ci.yml/badge.svg?job=windows-build)](https://github.com/USER/REPO/actions/workflows/ci.yml)

## Features

- Serial monitor with ANSI color parsing for ESP-IDF log output
- Real-time serial plotter with auto-scaling (supports JSON key-value data)
- Industrial-grade error handling with status bar
- Cross-platform: GNOME, KDE Plasma, and Windows
- Dark-themed monospace terminal output

## Quick Start

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/appQtTest
```

Or use VSCode: **Ctrl+Shift+B** to build, **F5** to debug.

## Build for Windows

```bash
./build-windows.sh
```

Requires Docker. Outputs `appQtTest-windows/` folder with bundled DLLs.

## Dependencies (Debian 13)

```bash
sudo apt install qt6-base-dev qt6-declarative-dev qt6-serialport-dev cmake g++
```

Optional: `qgnomeplatform-qt6` for GNOME native look.

## Serial Plotter

Send JSON from ESP32 to plot named values:

```cpp
printf("{\"temp\":%.2f,\"humidity\":%.2f}\n", t, h);
```

Each key becomes a separate colored line on the plotter. Supports up to 6 channels.

## License

MIT
