#include "serialporthandler.h"
#include <QSerialPortInfo>
#include <QRegularExpression>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

SerialPortHandler::SerialPortHandler(QObject *parent)
    : QObject(parent)
    , m_serial(new QSerialPort(this))
    , m_baudRate(115200)
    , m_connected(false)
    , m_hasError(false)
    , m_rawMode(false)
    , m_plotTimeWindow(10)
{
    refreshPorts();
    m_plotTimer.start();
    connect(m_serial, &QSerialPort::readyRead, this, &SerialPortHandler::onReadyRead);
    connect(m_serial, &QSerialPort::errorOccurred, this, &SerialPortHandler::onErrorOccurred);
    setStatusMessage("Disconnected");
}

SerialPortHandler::~SerialPortHandler() {
    if (m_serial->isOpen()) {
        m_serial->close();
    }
}

QStringList SerialPortHandler::availablePorts() const {
    return m_ports;
}

QString SerialPortHandler::currentPort() const {
    return m_currentPort;
}

int SerialPortHandler::baudRate() const {
    return m_baudRate;
}

bool SerialPortHandler::connected() const {
    return m_connected;
}

QString SerialPortHandler::receivedData() const {
    return m_receivedData;
}

QString SerialPortHandler::displayData() const {
    return m_displayData;
}

QString SerialPortHandler::statusMessage() const {
    return m_statusMessage;
}

bool SerialPortHandler::hasError() const {
    return m_hasError;
}

bool SerialPortHandler::rawMode() const {
    return m_rawMode;
}

void SerialPortHandler::setCurrentPort(const QString &port) {
    if (m_currentPort != port) {
        m_currentPort = port;
        emit currentPortChanged();
    }
}

void SerialPortHandler::setBaudRate(int rate) {
    if (m_baudRate != rate) {
        m_baudRate = rate;
        emit baudRateChanged();
    }
}

void SerialPortHandler::setRawMode(bool raw) {
    if (m_rawMode != raw) {
        m_rawMode = raw;
        updateDisplayText();
        emit rawModeChanged();
    }
}

void SerialPortHandler::setStatusMessage(const QString &msg) {
    if (m_statusMessage != msg) {
        m_statusMessage = msg;
        emit statusMessageChanged();
    }
}

void SerialPortHandler::setHasError(bool err) {
    if (m_hasError != err) {
        m_hasError = err;
        emit hasErrorChanged();
    }
}

void SerialPortHandler::refreshPorts() {
    QStringList ports;
    const auto infos = QSerialPortInfo::availablePorts();
    for (const auto &info : infos) {
        ports.append(info.portName());
    }
    if (m_ports != ports) {
        m_ports = ports;
        emit availablePortsChanged();
    }
}

void SerialPortHandler::connectToPort() {
    if (m_currentPort.isEmpty()) {
        QString msg = "No port selected";
        setStatusMessage(msg);
        setHasError(true);
        emit errorOccurred(msg);
        return;
    }

    m_serial->setPortName(m_currentPort);
    m_serial->setBaudRate(m_baudRate);
    m_serial->setDataBits(QSerialPort::Data8);
    m_serial->setParity(QSerialPort::NoParity);
    m_serial->setStopBits(QSerialPort::OneStop);
    m_serial->setFlowControl(QSerialPort::NoFlowControl);

    if (m_serial->open(QIODevice::ReadWrite)) {
        m_connected = true;
        setHasError(false);
        setStatusMessage(QString("Connected @ %1 baud").arg(m_baudRate));
        emit connectedChanged();
    } else {
        QString msg = m_serial->errorString();
        setStatusMessage(msg);
        setHasError(true);
        emit errorOccurred(msg);
    }
}

void SerialPortHandler::disconnectFromPort() {
    if (m_serial->isOpen()) {
        m_serial->close();
    }
    m_connected = false;
    setHasError(false);
    setStatusMessage("Disconnected");
    emit connectedChanged();
}

void SerialPortHandler::sendData(const QString &data) {
    if (!m_serial->isOpen()) {
        setStatusMessage("Not connected - cannot send");
        setHasError(true);
        emit errorOccurred("Not connected");
        return;
    }
    m_serial->write(data.toUtf8());
}

void SerialPortHandler::clearReceivedData() {
    m_receivedData.clear();
    m_displayData.clear();
    emit receivedDataChanged();
    emit displayDataChanged();
}

void SerialPortHandler::onReadyRead() {
    QByteArray data = m_serial->readAll();
    QString text = QString::fromUtf8(data);
    m_receivedData += text;
    emit receivedDataChanged();
    updateDisplayText();

    m_parseBuffer += text;
    while (true) {
        int idx = m_parseBuffer.indexOf(QLatin1Char('\n'));
        if (idx < 0) break;
        QString line = m_parseBuffer.left(idx).trimmed();
        m_parseBuffer = m_parseBuffer.mid(idx + 1);
        if (!line.isEmpty()) {
            parsePlotValues(line);
        }
    }
}

void SerialPortHandler::onErrorOccurred(QSerialPort::SerialPortError error) {
    if (error == QSerialPort::NoError) return;

    QString msg;
    switch (error) {
    case QSerialPort::DeviceNotFoundError:
        msg = "Device not found (unplugged?)";
        break;
    case QSerialPort::PermissionError:
        msg = "Permission denied - check /dev/tty* permissions";
        break;
    case QSerialPort::OpenError:
        msg = "Port already in use by another application";
        break;
    case QSerialPort::ResourceError:
        msg = "Device unexpectedly disconnected";
        break;
    case QSerialPort::TimeoutError:
        msg = "I/O timeout";
        break;
    case QSerialPort::NotOpenError:
        msg = "Port not open";
        break;
    default:
        msg = "Serial error: " + m_serial->errorString();
        break;
    }

    if (m_serial->isOpen()) {
        m_serial->close();
    }
    m_connected = false;
    setHasError(true);
    setStatusMessage(msg);
    emit connectedChanged();
    emit errorOccurred(msg);
}

void SerialPortHandler::updateDisplayText() {
    m_displayData = ansiToHtml(m_receivedData, !m_rawMode);
    emit displayDataChanged();
}

static void appendOpenSpan(QString &html, const char *hex) {
    html += QLatin1String("<span style=\"color:");
    html += QLatin1String(hex);
    html += QLatin1String(";\">");
}
static void appendCloseSpan(QString &html) {
    html += QLatin1String("</span>");
}
static const char *hexForCode(int code) {
    switch (code) {
    case 30: return "#000000";
    case 31: return "#e06c75";
    case 32: return "#98c379";
    case 33: return "#e5c07b";
    case 34: return "#61afef";
    case 35: return "#c678dd";
    case 36: return "#56b6c2";
    case 37: return "#abb2bf";
    case 90: return "#5c6370";
    case 91: return "#e06c75";
    case 92: return "#98c379";
    case 93: return "#e5c07b";
    case 94: return "#61afef";
    case 95: return "#c678dd";
    case 96: return "#56b6c2";
    case 97: return "#ffffff";
    default: return nullptr;
    }
}

QString SerialPortHandler::ansiToHtml(const QString &plain, bool includeAnsi) const {
    QString out = plain;
    out.replace(QChar('&'), QString() + QChar('&') + QChar('a') + QChar('m') + QChar('p') + QChar(';'));
    out.replace(QChar('<'), QString() + QChar('&') + QChar('l') + QChar('t') + QChar(';'));
    out.replace(QChar('>'), QString() + QChar('&') + QChar('g') + QChar('t') + QChar(';'));
    out.replace(QChar('\n'), QString() + QChar('<') + QChar('b') + QChar('r') + QChar('>'));
    out.replace(QChar('\r'), QString());

    if (includeAnsi) {
        static const QRegularExpression ansiRx(QLatin1String("\x1b\\[([0-9;]*)m"));
        bool spanOpen = false;
        QString html;
        int lastEnd = 0;

        QRegularExpressionMatchIterator it = ansiRx.globalMatch(out);
        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            if (match.capturedStart() > lastEnd) {
                html += out.mid(lastEnd, match.capturedStart() - lastEnd);
            }
            QString codes = match.captured(1);
            bool needClose = false;
            bool gotColor = false;

            if (codes.isEmpty()) {
                needClose = true;
            } else {
                const QStringList parts = codes.split(QLatin1Char(';'));
                for (const QString &p : parts) {
                    int code = p.toInt();
                    if (code == 0) {
                        needClose = true;
                    }
                    if (hexForCode(code)) {
                        gotColor = true;
                    }
                }
            }

            if (needClose && spanOpen) {
                appendCloseSpan(html);
                spanOpen = false;
            }

            if (gotColor) {
                const QStringList parts = codes.split(QLatin1Char(';'));
                for (const QString &p : parts) {
                    int code = p.toInt();
                    const char *hex = hexForCode(code);
                    if (hex) {
                        if (spanOpen) {
                            appendCloseSpan(html);
                        }
                        appendOpenSpan(html, hex);
                        spanOpen = true;
                    }
                }
            }
            lastEnd = match.capturedEnd();
        }
        if (lastEnd < out.length()) {
            html += out.mid(lastEnd);
        }
        if (spanOpen) {
            appendCloseSpan(html);
        }
        return html;
    }

    return out;
}

QVariantList SerialPortHandler::plotData() const {
    return m_plotData;
}

int SerialPortHandler::plotTimeWindow() const {
    return m_plotTimeWindow;
}

QStringList SerialPortHandler::plotChannels() const {
    return m_plotChannels;
}

void SerialPortHandler::setPlotTimeWindow(int seconds) {
    if (seconds < 1) seconds = 1;
    if (seconds > 120) seconds = 120;
    if (m_plotTimeWindow != seconds) {
        m_plotTimeWindow = seconds;
        prunePlotData();
        emit plotTimeWindowChanged();
    }
}

void SerialPortHandler::clearPlotData() {
    m_plotData.clear();
    m_plotChannels.clear();
    emit plotDataChanged();
    emit plotChannelsChanged();
}

void SerialPortHandler::parsePlotValues(const QString &line) {
    QVariantList row;
    QStringList names;

    if (line.startsWith(QLatin1Char('{'))) {
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(line.toUtf8(), &err);
        if (err.error != QJsonParseError::NoError) return;
        if (!doc.isObject()) return;
        QJsonObject obj = doc.object();
        for (auto it = obj.begin(); it != obj.end(); ++it) {
            if (it.value().isDouble()) {
                names.append(it.key());
                row.append(it.value().toDouble());
            }
        }
    } else {
        static const QRegularExpression numRx("[+-]?([0-9]*[.])?[0-9]+");
        QRegularExpressionMatchIterator it = numRx.globalMatch(line);
        int idx = 1;
        while (it.hasNext()) {
            QRegularExpressionMatch m = it.next();
            bool ok = false;
            double v = m.captured().toDouble(&ok);
            if (ok) {
                names.append(QStringLiteral("Ch%1").arg(idx));
                row.append(v);
                idx++;
            }
            if (row.size() >= 6) break;
        }
    }

    if (row.isEmpty()) return;

    if (m_plotChannels != names) {
        m_plotChannels = names;
        emit plotChannelsChanged();
    }

    double elapsed = m_plotTimer.elapsed() / 1000.0;
    row.prepend(elapsed);
    m_plotData.append(QVariant::fromValue(row));
    prunePlotData();
    emit plotDataChanged();
}

void SerialPortHandler::prunePlotData() {
    if (m_plotData.isEmpty()) return;
    double now = m_plotTimer.elapsed() / 1000.0;
    double cutoff = now - m_plotTimeWindow;
    while (!m_plotData.isEmpty()) {
        QVariantList row = m_plotData.first().toList();
        if (row.isEmpty()) { m_plotData.removeFirst(); continue; }
        double t = row.first().toDouble();
        if (t < cutoff) {
            m_plotData.removeFirst();
        } else {
            break;
        }
    }
}
