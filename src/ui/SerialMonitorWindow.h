// SerialMonitorWindow.h
#pragma once

#include <QWidget>
#include <QSerialPort>
#include <QByteArray>
#include <QComboBox>
#include <QPushButton>
#include <QTextEdit>
#include <QLineEdit>

class SerialMonitorWindow : public QWidget {
    Q_OBJECT
public:
    explicit SerialMonitorWindow(QWidget* parent = nullptr);
    ~SerialMonitorWindow() override;

    void setPreferredPort(const QString& portName);

private slots:
    void refreshPorts();
    void toggleConnection();
    void onReadyRead();
    void sendLine();
    void clearOutput();
    void onSerialError(QSerialPort::SerialPortError error);

private:
    QSerialPort* m_serial = nullptr;
    QComboBox* m_portCombo = nullptr;
    QComboBox* m_baudCombo = nullptr;
    QPushButton* m_refreshBtn = nullptr;
    QPushButton* m_connectBtn = nullptr;
    QPushButton* m_clearBtn = nullptr;
    QTextEdit* m_outputEdit = nullptr;
    QLineEdit* m_inputEdit = nullptr;
    QComboBox* m_eolCombo = nullptr;
    QPushButton* m_sendBtn = nullptr;

    QByteArray m_rxBuffer;
    bool m_atLineStart = true;
};