Use `gh-axi` for GitHub and `chrome-devtools-axi` for browser automation.

## Project Overview

This is a Qt6-based serial monitor application for ESP32-C3, built with QML + C++.
The app provides a terminal-like interface to communicate with ESP32-C3 over USB-CDC serial.

## System Architecture

```
main.cpp                     Entry point, sets up qgnomeplatform theme, creates SerialPortHandler,
                             exposes it to QML via setContextProperty("serialHandler", ...)

serialporthandler.h/.cpp     C++ backend: QSerialPort wrapper exposed to QML via QML_ELEMENT.
                             Handles port enumeration, connect/disconnect, read/write, error
                             recovery, and ANSI-to-HTML conversion for colored output.

Main.qml                     QML frontend: port/baud/connect header, TabBar (Monitor | Plotter),
                             SwipeView with Monitor tab (RichText output, input, send) and
                             Plotter tab (Canvas-based real-time line chart), status bar.

qtquickcontrols2.conf        Forces Qt Quick Controls Fusion style (rounded buttons).
.clangd                       Points clangd to build/compile_commands.json for intellisense.
.vscode/                     settings.json (QML file associations), tasks.json (build/run tasks),
                             launch.json (F5 debug with gdb), extensions.json (recommendations).
```

## Data Flow

1. User selects port + baud, clicks Connect → `SerialPortHandler::connectToPort()`
2. `QSerialPort::readyRead` → `onReadyRead()` → appends UTF-8 bytes to `m_receivedData`
3. `updateDisplayText()` converts raw text via `ansiToHtml()` → `m_displayData`
4. QML `TextArea` bound to `displayData` renders HTML with colored ESP-IDF log output
5. User types in TextField → Enter/Send → `sendData()` writes bytes to serial port
6. Incoming lines are also parsed for numeric values → `parsePlotValues()` → `m_plotData`
7. QML Canvas repaints on `plotDataChanged`, auto-scaling Y-axis, rolling X time window
8. Errors (unplug, permission, etc.) → `onErrorOccurred()` → auto-close, status bar turns red

## SerialPortHandler Properties (exposed to QML)

| Property | Type | Description |
|----------|------|-------------|
| availablePorts | QStringList | List of detected serial ports (e.g. ttyACM0) |
| currentPort | QString | Currently selected port name |
| baudRate | int | Baud rate (default 115200) |
| connected | bool | Whether serial port is open |
| receivedData | QString | Raw accumulated received text |
| displayData | QString | ANSI-parsed HTML for RichText display |
| statusMessage | QString | Human-readable status (e.g. "Connected @ 115200 baud") |
| hasError | bool | True when an error has occurred |
| rawMode | bool | Toggle raw ANSI text vs rendered HTML (default false) |
| plotData | QVariantList | Accumulated time-value pairs for plotter: [[t, v1, v2...], ...] |
| plotTimeWindow | int | Rolling window in seconds for plotter X-axis (default 10, range 1-120) |

## Error Handling Matrix

| Error Code | Message |
|------------|---------|
| DeviceNotFoundError | Device not found (unplugged?) |
| PermissionError | Permission denied - check /dev/tty* permissions |
| OpenError | Port already in use by another application |
| ResourceError | Device unexpectedly disconnected |
| TimeoutError | I/O timeout |
| NotOpenError | Port not open |
| No port selected | No port selected |
| Send while disconnected | Not connected - cannot send |

All errors trigger: auto-close, `hasError = true`, status bar turns red, `errorOccurred` signal emitted.

## ANSI Color Mapping (ESP-IDF Log Style)

| ANSI Code | Color | Hex |
|-----------|-------|-----|
| \033[30m / 90 | Black / Bright Black | #000000 / #5c6370 |
| \033[31m / 91 | Red (errors) | #e06c75 |
| \033[32m / 92 | Green (info) | #98c379 |
| \033[33m / 93 | Yellow (warnings) | #e5c07b |
| \033[34m / 94 | Blue | #61afef |
| \033[35m / 95 | Magenta | #c678dd |
| \033[36m / 96 | Cyan (debug) | #56b6c2 |
| \033[37m / 97 | White / Bright White | #abb2bf / #ffffff |
| \033[0m | Reset | Close font tag |

## Cross-Platform Support

The app uses platform auto-detection - no hardcoded platform theme.
Qt6 automatically selects the native look on each OS:

| Platform | Style | Notes |
|----------|-------|-------|
| GNOME | qgnomeplatform | Automatic if qgnomeplatform-qt6 is installed |
| KDE Plasma | KDE Platform Theme | Loads natively |
| Windows | Windows Vista/11 style | Native look-and-feel |
| Fallback | Fusion | Clean cross-platform style via qtquickcontrols2.conf |

## Build Instructions

### Linux (Debian / Ubuntu)
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/appQtTest
```

### Windows - Build on Windows (MSVC or MinGW)
```cmd
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
build\appQtTest.exe
```

### Windows - Cross-compile from Linux (Docker + Fedora)
```bash
./build-windows.sh
# Output: appQtTest.exe
```
Uses a Fedora Docker container with pre-built mingw64-qt6 packages.
First build: ~13 minutes (dnf install ~810 packages). Subsequent builds: seconds.

Or use VSCode tasks: Ctrl+Shift+B (Build), Ctrl+Shift+P → Run Task → Run App, F5 (Debug).

## Dependencies

### Debian 13
- qt6-base-dev, qt6-declarative-dev
- qt6-serialport-dev (for QSerialPort)
- qgnomeplatform-qt6 (optional, for GNOME native look)
- cmake, g++

### Windows
- Qt 6.x (official installer or vcpkg) with modules: QtCore, QtQuick, QtSerialPort
- CMake 3.16+
- MSVC 2022 or MinGW-w64

## Memory Benchmark (Debian 13, x86_64)

Measured at idle with serial port connected (no incoming data):

| Metric | Value |
|--------|-------|
| VSZ (virtual) | ~1664 MB |
| RSS (resident) | ~222 MB |
| VmData (heap) | ~188 MB |
| Heap allocated (massif peak) | ~7.8 MB |
| App-exclusive heap (massif useful) | ~1.8 MB |
| Shared libraries | ~339 MB |

The large VSZ/RSS is dominated by Qt6 and GPU driver shared libraries.
The application's own heap allocations are under 2 MB - very lean.
Most of the 222 MB RSS is shared with other Qt6 apps via the system's shared library cache.

## Known Issues

- `setHighDpiScaleFactorRoundingPolicy must be called before creating the QGuiApplication instance`
  Upstream qgnomeplatform warning. Harmless, affects all Qt6 apps using it.
- `Could not find color scheme ""`
  Upstream qgnomeplatform bug when launched from terminal without full D-Bus session. Falls back to light mode. Harmless.
- CMake QML plugin link warnings (qtquick2plugin, modelsplugin, etc.)
  Benign Debian packaging issue. Plugins loaded at QML runtime, not via static linking.

## Project History

1. Initial setup: Qt6 QML project with CMake (Main.qml with single button)
2. Fixed AUTOMOC + QTP0001 CMake errors, added .clangd config
3. Installed qgnomeplatform-qt6 for GNOME native look
4. Added qtquickcontrols2.conf for Fusion style (rounded buttons)
5. Created .vscode/ project settings (tasks, launch, settings, extensions)
6. Built serial monitor: SerialPortHandler C++ class + serial monitor QML UI
7. Added industrial-grade error handling with auto-disconnect and status messages
8. Added ANSI color parser for ESP-IDF colored log output
9. Added TabBar with SwipeView: Monitor tab and Plotter tab
10. Added serial plotter with auto-scaling Canvas, rolling time window, multi-channel support
