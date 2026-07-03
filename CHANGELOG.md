# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased]

### Added

- Initial Qt6 QML project with CMake build system
- Serial monitor with ANSI color parsing for ESP-IDF log output
- Real-time serial plotter with auto-scaling Canvas and multi-channel support
- JSON key-value data support for plotter with named channels
- TabBar with SwipeView: Monitor tab and Plotter tab
- Industrial-grade error handling with auto-disconnect and status messages
- Cross-platform support: GNOME, KDE Plasma, and Windows
- Docker + Fedora cross-compilation for Windows (MinGW)
- Static MinGW runtime linking for standalone Windows .exe
- Qt6 DLL bundling for Windows deployment
- `.clang-format` with Qt C++ standard style
- `.editorconfig` for cross-editor settings
- `.githooks/pre-commit` with clang-format + whitespace checks
- VSCode project settings: tasks.json, launch.json, settings.json
- clangd-22 intellisense configuration
- Memory benchmark documented in AGENTS.md
