import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: window
    width: 800; height: 550; visible: true
    title: "Serial Monitor - ESP32-C3"
    font.weight: Font.Normal

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 4

        RowLayout {
            spacing: 8
            Text { text: "Port:"; Layout.alignment: Qt.AlignVCenter }
            ComboBox {
                id: portCombo
                model: serialHandler.availablePorts
                Layout.fillWidth: true
                onCurrentTextChanged: serialHandler.currentPort = currentText
            }
            Button {
                text: "Refresh"
                onClicked: serialHandler.refreshPorts()
            }
            Text { text: "Baud:"; Layout.alignment: Qt.AlignVCenter }
            ComboBox {
                id: baudCombo
                model: [9600, 19200, 38400, 57600, 74880, 115200, 230400, 460800, 921600]
                currentIndex: model.indexOf(115200)
                onCurrentTextChanged: serialHandler.baudRate = parseInt(currentText)
            }
            Button {
                id: connectBtn
                text: serialHandler.connected ? "Disconnect" : "Connect"
                onClicked: {
                    if (serialHandler.connected) {
                        serialHandler.disconnectFromPort()
                    } else {
                        serialHandler.connectToPort()
                    }
                }
            }
        }

        TabBar {
            id: tabBar
            TabButton { text: "Monitor" }
            TabButton { text: "Plotter" }
        }

        SwipeView {
            id: swipeView
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: tabBar.currentIndex

            Item {
                id: monitorTab
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 4
                    RowLayout {
                        CheckBox {
                            id: rawCheck
                            text: "Raw"
                            checked: false
                            onCheckedChanged: serialHandler.rawMode = checked
                        }
                        Item { Layout.fillWidth: true }
                        Button {
                            text: "Clear"
                            onClicked: serialHandler.clearReceivedData()
                        }
                    }
                    ScrollView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        ScrollBar.vertical.policy: ScrollBar.AlwaysOn
                        TextArea {
                            id: outputArea
                            readOnly: true
                            font.family: "monospace"
                            font.pixelSize: 13
                            textFormat: Text.RichText
                            text: serialHandler.displayData
                            wrapMode: TextEdit.Wrap
                            background: Rectangle { color: "#1e1e1e" }
                            color: "#d4d4d4"
                            selectByMouse: true
                        }
                    }
                    RowLayout {
                        spacing: 8
                        TextField {
                            id: inputField
                            Layout.fillWidth: true
                            placeholderText: "Type to send..."
                            onAccepted: {
                                serialHandler.sendData(text + "\n")
                                text = ""
                            }
                        }
                        Button {
                            text: "Send"
                            onClicked: {
                                serialHandler.sendData(inputField.text + "\n")
                                inputField.text = ""
                            }
                        }
                    }
                }
            }

            Item {
                id: plotterTab
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 4
                    RowLayout {
                        Text { text: "Window:"; Layout.alignment: Qt.AlignVCenter }
                        ComboBox {
                            model: [5, 10, 15, 30, 60]
                            currentIndex: model.indexOf(serialHandler.plotTimeWindow)
                            onCurrentTextChanged: serialHandler.plotTimeWindow = parseInt(currentText)
                            displayText: currentText + "s"
                        }
                        Item { Layout.fillWidth: true }
                        Button {
                            text: "Clear"
                            onClicked: serialHandler.clearPlotData()
                        }
                    }
                    Canvas {
                        id: plotCanvas
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        property var colors: ["#e06c75", "#61afef", "#98c379", "#e5c07b", "#c678dd", "#56b6c2"]
                        property var channelNames: []

                        Connections {
                            target: serialHandler
                            function onPlotDataChanged() {
                                plotCanvas.requestPaint()
                            }
                        }

                        onPaint: {
                            var ctx = getContext("2d")
                            var w = width
                            var h = height
                            ctx.clearRect(0, 0, w, h)

                            var data = serialHandler.plotData
                            if (!data || data.length < 2) {
                                ctx.fillStyle = "#888888"
                                ctx.font = "14px monospace"
                                ctx.textAlign = "center"
                                ctx.fillText("Waiting for data... (send CSV values like 12.3,45.6)", w / 2, h / 2)
                                return
                            }

                            var channels = detectChannels(data)
                            var timeWindow = serialHandler.plotTimeWindow
                            var now = data[data.length - 1][0]
                            var xMin = now - timeWindow
                            var xMax = now

                            var yMin = 1e20, yMax = -1e20
                            for (var i = 0; i < data.length; i++) {
                                var row = data[i]
                                var t = row[0]
                                for (var c = 0; c < channels; c++) {
                                    var v = row[c + 1]
                                    if (v < yMin) yMin = v
                                    if (v > yMax) yMax = v
                                }
                            }
                            if (yMin === yMax) { yMin -= 1; yMax += 1 }
                            var margin = { left: 55, right: 20, top: 15, bottom: 30 }
                            var pw = w - margin.left - margin.right
                            var ph = h - margin.top - margin.bottom

                            function xToPx(t) { return margin.left + ((t - xMin) / (xMax - xMin)) * pw }
                            function yToPx(y) { return margin.top + (1 - (y - yMin) / (yMax - yMin)) * ph }

                            ctx.fillStyle = "#1a1a1a"
                            ctx.fillRect(0, 0, w, h)

                            ctx.strokeStyle = "#333333"
                            ctx.lineWidth = 0.5
                            var ySteps = 5
                            for (var i = 0; i <= ySteps; i++) {
                                var yy = margin.top + (ph / ySteps) * i
                                ctx.beginPath()
                                ctx.moveTo(margin.left, yy)
                                ctx.lineTo(w - margin.right, yy)
                                ctx.stroke()
                            }
                            var xSteps = 5
                            for (var j = 0; j <= xSteps; j++) {
                                var xx = margin.left + (pw / xSteps) * j
                                ctx.beginPath()
                                ctx.moveTo(xx, margin.top)
                                ctx.lineTo(xx, h - margin.bottom)
                                ctx.stroke()
                            }

                            ctx.fillStyle = "#cccccc"
                            ctx.font = "10px monospace"
                            ctx.textAlign = "right"
                            for (var k = 0; k <= ySteps; k++) {
                                var val = yMin + (yMax - yMin) * (k / ySteps)
                                var yy = margin.top + ph - (ph / ySteps) * k
                                ctx.fillText(val.toFixed(2), margin.left - 5, yy + 4)
                            }
                            ctx.textAlign = "center"
                            for (var l = 0; l <= xSteps; l++) {
                                var val = xMin + (xMax - xMin) * (l / xSteps)
                                var xx = margin.left + (pw / xSteps) * l
                                ctx.fillText(val.toFixed(1) + "s", xx, h - margin.bottom + 14)
                            }

                            for (var ch = 0; ch < channels; ch++) {
                                ctx.strokeStyle = colors[ch % colors.length]
                                ctx.lineWidth = 1.5
                                ctx.beginPath()
                                var started = false
                                for (var p = 0; p < data.length; p++) {
                                    var row = data[p]
                                    var t = row[0]
                                    var v = row[ch + 1]
                                    if (t < xMin) continue
                                    var sx = xToPx(t)
                                    var sy = yToPx(v)
                                    if (!started) { ctx.moveTo(sx, sy); started = true }
                                    else { ctx.lineTo(sx, sy) }
                                }
                                ctx.stroke()
                            }

                            ctx.fillStyle = "#cccccc"
                            ctx.font = "11px monospace"
                            ctx.textAlign = "left"
                            var chNames = serialHandler.plotChannels
                            for (var lg = 0; lg < channels; lg++) {
                                ctx.fillStyle = colors[lg % colors.length]
                                var ly = margin.top + 14 + lg * 16
                                var name = (chNames && lg < chNames.length) ? chNames[lg] : ("Ch" + (lg + 1))
                                ctx.fillText(name, margin.left + 8, ly)
                            }
                        }

                        function detectChannels(dataList) {
                            var maxCh = 0
                            for (var i = 0; i < dataList.length; i++) {
                                var len = dataList[i].length - 1
                                if (len > maxCh) maxCh = len
                            }
                            return maxCh > 6 ? 6 : maxCh
                        }

                        MouseArea {
                            anchors.fill: parent
                            acceptedButtons: Qt.NoButton
                            onWheel: function(wheel) {
                                wheel.accepted = true
                            }
                        }
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: statusLabel.implicitHeight + 8
            radius: 4
            color: serialHandler.hasError ? "#4a2020" : (serialHandler.connected ? "#1a3a1a" : "#2a2a2a")
            border.width: 1
            border.color: serialHandler.hasError ? "#e06c75" : (serialHandler.connected ? "#98c379" : "#3a3a3a")

            Label {
                id: statusLabel
                anchors.centerIn: parent
                text: serialHandler.statusMessage
                color: serialHandler.hasError ? "#e06c75" : (serialHandler.connected ? "#98c379" : "#888888")
                font.pixelSize: 12
                font.family: "monospace"
            }
        }
    }
}