#pragma once

#include <QObject>
#include <QSerialPort>
#include <QStringList>
#include <QVariantList>
#include <QElapsedTimer>
#include <QQmlEngine>

class SerialPortHandler : public QObject {
    Q_OBJECT
    Q_PROPERTY(QStringList availablePorts READ availablePorts NOTIFY availablePortsChanged)
    Q_PROPERTY(QString currentPort READ currentPort WRITE setCurrentPort NOTIFY currentPortChanged)
    Q_PROPERTY(int baudRate READ baudRate WRITE setBaudRate NOTIFY baudRateChanged)
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(QString receivedData READ receivedData NOTIFY receivedDataChanged)
    Q_PROPERTY(QString displayData READ displayData NOTIFY displayDataChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(bool hasError READ hasError NOTIFY hasErrorChanged)
    Q_PROPERTY(bool rawMode READ rawMode WRITE setRawMode NOTIFY rawModeChanged)
    Q_PROPERTY(QVariantList plotData READ plotData NOTIFY plotDataChanged)
    Q_PROPERTY(int plotTimeWindow READ plotTimeWindow WRITE setPlotTimeWindow NOTIFY plotTimeWindowChanged)
    Q_PROPERTY(QStringList plotChannels READ plotChannels NOTIFY plotChannelsChanged)
    QML_ELEMENT

public:
    explicit SerialPortHandler(QObject *parent = nullptr);
    ~SerialPortHandler();

    QStringList availablePorts() const;
    QString currentPort() const;
    int baudRate() const;
    bool connected() const;
    QString receivedData() const;
    QString displayData() const;
    QString statusMessage() const;
    bool hasError() const;
    bool rawMode() const;
    QVariantList plotData() const;
    int plotTimeWindow() const;
    QStringList plotChannels() const;

    void setCurrentPort(const QString &port);
    void setBaudRate(int rate);
    void setRawMode(bool raw);
    void setPlotTimeWindow(int seconds);

public slots:
    void refreshPorts();
    void connectToPort();
    void disconnectFromPort();
    void sendData(const QString &data);
    void clearReceivedData();
    void clearPlotData();

signals:
    void availablePortsChanged();
    void currentPortChanged();
    void baudRateChanged();
    void connectedChanged();
    void receivedDataChanged();
    void displayDataChanged();
    void statusMessageChanged();
    void hasErrorChanged();
    void rawModeChanged();
    void plotDataChanged();
    void plotTimeWindowChanged();
    void plotChannelsChanged();
    void errorOccurred(const QString &message);

private slots:
    void onReadyRead();
    void onErrorOccurred(QSerialPort::SerialPortError error);

private:
    void updateDisplayText();
    QString ansiToHtml(const QString &plain, bool includeAnsi) const;
    void setStatusMessage(const QString &msg);
    void setHasError(bool err);
    void parsePlotValues(const QString &line);
    void prunePlotData();

    QSerialPort *m_serial;
    QStringList m_ports;
    QString m_currentPort;
    int m_baudRate;
    QString m_receivedData;
    QString m_displayData;
    QString m_statusMessage;
    bool m_connected;
    bool m_hasError;
    bool m_rawMode;
    QVariantList m_plotData;
    int m_plotTimeWindow;
    QElapsedTimer m_plotTimer;
    QStringList m_plotChannels;
    QString m_parseBuffer;
};