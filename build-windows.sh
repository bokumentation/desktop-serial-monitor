#!/bin/bash
set -e

echo "=== Building Docker image ==="
docker build -t appqttest-windows -f .devcontainer/Dockerfile.windows .

CONTAINER_ID=$(docker create appqttest-windows)

echo "=== Extracting build artifacts ==="
docker cp "$CONTAINER_ID":/build/build-windows/appQtTest.exe ./appQtTest.exe
docker cp "$CONTAINER_ID":/build/build-windows ./build-windows-output
docker rm "$CONTAINER_ID" > /dev/null 2>&1 || true

echo "=== Bundling Windows DLLs ==="
SCRIPT_DIR=$(dirname "$(readlink -f "$0")")
SYSROOT=/usr/x86_64-w64-mingw32/sys-root/mingw
DEPLOY_DIR="$SCRIPT_DIR/appQtTest-windows"

rm -rf "$DEPLOY_DIR"
mkdir -p "$DEPLOY_DIR"
cp "$SCRIPT_DIR/appQtTest.exe" "$DEPLOY_DIR/"

run_bundle() {
    docker run --rm \
        -v "$SCRIPT_DIR":/work \
        -v "$DEPLOY_DIR":/deploy \
        fedora:latest bash -c '
set -e
SYSROOT=/usr/x86_64-w64-mingw32/sys-root/mingw
DEPLOY=/deploy
BIN=$SYSROOT/bin

echo "Installing MinGW Qt6 packages..."
dnf install -y --setopt=install_weak_deps=False \
    mingw64-qt6-qtbase mingw64-qt6-qtdeclarative mingw64-qt6-qtserialport mingw64-qt6-qtsvg \
    mingw64-winpthreads mingw64-gcc-c++ mingw64-filesystem 2>&1 | tail -2

cp $SYSROOT/bin/Qt6Core.dll $DEPLOY/
cp $SYSROOT/bin/Qt6Gui.dll $DEPLOY/
cp $SYSROOT/bin/Qt6Quick.dll $DEPLOY/
cp $SYSROOT/bin/Qt6Qml.dll $DEPLOY/
cp $SYSROOT/bin/Qt6QmlMeta.dll $DEPLOY/
cp $SYSROOT/bin/Qt6QmlModels.dll $DEPLOY/
cp $SYSROOT/bin/Qt6QmlWorkerScript.dll $DEPLOY/
cp $SYSROOT/bin/Qt6QmlBuiltins.dll $DEPLOY/
cp $SYSROOT/bin/Qt6Network.dll $DEPLOY/
cp $SYSROOT/bin/Qt6SerialPort.dll $DEPLOY/
cp $SYSROOT/bin/Qt6QuickControls2.dll $DEPLOY/
cp $SYSROOT/bin/Qt6QuickTemplates2.dll $DEPLOY/
cp $SYSROOT/bin/Qt6QuickLayouts.dll $DEPLOY/
cp $SYSROOT/bin/Qt6Svg.dll $DEPLOY/
cp $SYSROOT/bin/Qt6OpenGL.dll $DEPLOY/
cp $SYSROOT/bin/Qt6DBus.dll $DEPLOY/
cp $SYSROOT/bin/Qt6Xml.dll $DEPLOY/

echo "=== Copying QML plugin DLLs ==="
mkdir -p $DEPLOY/Qt6/plugins/platforms
cp $SYSROOT/lib/qt6/plugins/platforms/qwindows.dll $DEPLOY/Qt6/plugins/platforms/

mkdir -p $DEPLOY/Qt6/qml/QtQuick/Controls/Fusion
QMLDIR=$SYSROOT/lib/qt6/qml
cp $QMLDIR/QtQuick/Controls/Fusion/* $DEPLOY/Qt6/qml/QtQuick/Controls/Fusion/ 2>/dev/null || true

mkdir -p $DEPLOY/Qt6/qml/QtQuick/Templates
cp $QMLDIR/QtQuick/Templates/* $DEPLOY/Qt6/qml/QtQuick/Templates/ 2>/dev/null || true

mkdir -p $DEPLOY/Qt6/qml/QtQuick/Layouts
cp $QMLDIR/QtQuick/Layouts/* $DEPLOY/Qt6/qml/QtQuick/Layouts/ 2>/dev/null || true

mkdir -p $DEPLOY/Qt6/qml/QtQuick/Window
cp $QMLDIR/QtQuick/Window/* $DEPLOY/Qt6/qml/QtQuick/Window/ 2>/dev/null || true

mkdir -p $DEPLOY/Qt6/qml/QtQml
cp $QMLDIR/QtQml/* $DEPLOY/Qt6/qml/QtQml/ 2>/dev/null || true

mkdir -p $DEPLOY/Qt6/qml/QtQml/Models
cp $QMLDIR/QtQml/Models/* $DEPLOY/Qt6/qml/QtQml/Models/ 2>/dev/null || true

mkdir -p $DEPLOY/Qt6/qml/QtQml/WorkerScript
cp $QMLDIR/QtQml/WorkerScript/* $DEPLOY/Qt6/qml/QtQml/WorkerScript/ 2>/dev/null || true

echo "=== Done: $DEPLOY ==="
ls -lh $DEPLOY/
'
}

run_bundle

echo "=== Cleaning up ==="
rm -f "$SCRIPT_DIR/appQtTest.exe"

echo "Done: $DEPLOY_DIR/"
ls -lh "$DEPLOY_DIR/"
echo "Share appQtTest-windows/ folder with your Windows friend."
