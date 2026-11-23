// SerialMonitorWindow.cpp
#include "SerialMonitorWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSerialPortInfo>
#include <QScrollBar>
#include <QTime>

SerialMonitorWindow::SerialMonitorWindow(QWidget* parent)
    : QWidget(parent), m_serial(new QSerialPort(this))
{
    setWindowTitle(tr("Moniteur série"));
    resize(640, 360);

    auto* topBar = new QHBoxLayout();
    m_portCombo = new QComboBox(this);
    m_portCombo->setMinimumWidth(180);

    m_baudCombo = new QComboBox(this);
    for (int baud : {9600, 19200, 38400, 57600, 115200}) {
        m_baudCombo->addItem(QString::number(baud), baud);
    }
    m_baudCombo->setCurrentText(QStringLiteral("115200"));

    m_refreshBtn = new QPushButton(tr("Rafraîchir"), this);
    m_connectBtn = new QPushButton(tr("Ouvrir"), this);
    m_clearBtn = new QPushButton(tr("Effacer"), this);

    topBar->addWidget(m_portCombo);
    topBar->addWidget(m_baudCombo);
    topBar->addWidget(m_refreshBtn);
    topBar->addWidget(m_connectBtn);
    topBar->addStretch();
    topBar->addWidget(m_clearBtn);

    m_outputEdit = new QTextEdit(this);
    m_outputEdit->setReadOnly(true);
    m_outputEdit->setObjectName(QStringLiteral("serialOutput"));

    auto* bottomBar = new QHBoxLayout();
    m_inputEdit = new QLineEdit(this);
    m_inputEdit->setPlaceholderText(tr("Envoyer un message à la carte..."));
    m_inputEdit->setEnabled(false);

    m_eolCombo = new QComboBox(this);
    m_eolCombo->addItem(tr("Nouvelle ligne (\\n)"), QStringLiteral("\n"));
    m_eolCombo->addItem(tr("CR + LF (\\r\\n)"), QStringLiteral("\r\n"));
    m_eolCombo->addItem(tr("Aucun"), QString());

    m_sendBtn = new QPushButton(tr("Envoyer"), this);
    m_sendBtn->setEnabled(false);

    bottomBar->addWidget(m_inputEdit, 1);
    bottomBar->addWidget(m_eolCombo);
    bottomBar->addWidget(m_sendBtn);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(topBar);
    mainLayout->addWidget(m_outputEdit, 1);
    mainLayout->addLayout(bottomBar);

    connect(m_refreshBtn, &QPushButton::clicked, this, &SerialMonitorWindow::refreshPorts);
    connect(m_connectBtn, &QPushButton::clicked, this, &SerialMonitorWindow::toggleConnection);
    connect(m_clearBtn, &QPushButton::clicked, this, &SerialMonitorWindow::clearOutput);
    connect(m_serial, &QSerialPort::readyRead, this, &SerialMonitorWindow::onReadyRead);
    connect(m_sendBtn, &QPushButton::clicked, this, &SerialMonitorWindow::sendLine);
    connect(m_inputEdit, &QLineEdit::returnPressed, this, &SerialMonitorWindow::sendLine);
    connect(m_serial, &QSerialPort::errorOccurred, this, &SerialMonitorWindow::onSerialError);

    refreshPorts();
}

SerialMonitorWindow::~SerialMonitorWindow()
{
    if (m_serial && m_serial->isOpen()) {
        m_serial->close();
    }
}

void SerialMonitorWindow::setPreferredPort(const QString& portName)
{
    if (portName.isEmpty()) {
        return;
    }
    const int idx = m_portCombo->findData(portName);
    if (idx != -1) {
        m_portCombo->setCurrentIndex(idx);
    }
}

void SerialMonitorWindow::refreshPorts()
{
    const QString current = m_portCombo->currentData().toString();
    m_portCombo->clear();

    const auto ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo& info : ports) {
        const QString desc = info.description().isEmpty()
            ? info.portName()
            : QStringLiteral("%1 (%2)").arg(info.portName(), info.description());
        m_portCombo->addItem(desc, info.portName());
    }

    if (!current.isEmpty()) {
        const int idx = m_portCombo->findData(current);
        if (idx != -1) {
            m_portCombo->setCurrentIndex(idx);
        }
    }
}

void SerialMonitorWindow::toggleConnection()
{
    if (m_serial->isOpen()) {
        m_serial->close();
        m_connectBtn->setText(tr("Ouvrir"));
        m_portCombo->setEnabled(true);
        m_baudCombo->setEnabled(true);
        m_refreshBtn->setEnabled(true);
        m_inputEdit->setEnabled(false);
        m_sendBtn->setEnabled(false);
        return;
    }

    const QString portName = m_portCombo->currentData().toString();
    if (portName.isEmpty()) {
        m_outputEdit->append(tr("[Erreur] Aucun port série sélectionné."));
        return;
    }

    const int baudRate = m_baudCombo->currentData().toInt();
    m_serial->setPortName(portName);
    m_serial->setBaudRate(baudRate);
    m_serial->setDataBits(QSerialPort::Data8);
    m_serial->setParity(QSerialPort::NoParity);
    m_serial->setStopBits(QSerialPort::OneStop);
    m_serial->setFlowControl(QSerialPort::NoFlowControl);

    if (!m_serial->open(QIODevice::ReadWrite)) {
        m_outputEdit->append(tr("[Erreur] Impossible d'ouvrir %1 : %2").arg(portName, m_serial->errorString()));
        return;
    }

    m_serial->setDataTerminalReady(true);
    m_rxBuffer.clear();
    m_atLineStart = true;

    m_connectBtn->setText(tr("Fermer"));
    m_portCombo->setEnabled(false);
    m_baudCombo->setEnabled(false);
    m_refreshBtn->setEnabled(false);
    m_inputEdit->setEnabled(true);
    m_sendBtn->setEnabled(true);
    m_outputEdit->append(tr("--- Connecté à %1 @ %2 bauds ---").arg(portName).arg(baudRate));
}

void SerialMonitorWindow::clearOutput()
{
    m_outputEdit->clear();
    m_rxBuffer.clear();
    m_atLineStart = true;
}

void SerialMonitorWindow::onReadyRead()
{
    const QByteArray chunk = m_serial->readAll();
    if (chunk.isEmpty()) {
        return;
    }

    m_rxBuffer.append(chunk);
    const QString nowStr = QTime::currentTime().toString(QStringLiteral("HH:mm:ss.zzz"));

    int nlPos = -1;
    while ((nlPos = m_rxBuffer.indexOf('\n')) != -1) {
        QByteArray lineBytes = m_rxBuffer.left(nlPos + 1);
        m_rxBuffer.remove(0, nlPos + 1);

        QString line = QString::fromUtf8(lineBytes);
        if (m_atLineStart) {
            line.prepend(QStringLiteral("[%1] ").arg(nowStr));
        }

        m_outputEdit->insertPlainText(line);
        m_atLineStart = true;
    }

    if (!m_rxBuffer.isEmpty()) {
        QString partial = QString::fromUtf8(m_rxBuffer);
        if (m_atLineStart) {
            partial.prepend(QStringLiteral("[%1] ").arg(nowStr));
            m_atLineStart = false;
        }
        m_outputEdit->insertPlainText(partial);
        m_rxBuffer.clear();
    }

    QScrollBar* bar = m_outputEdit->verticalScrollBar();
    bar->setValue(bar->maximum());
}

void SerialMonitorWindow::sendLine()
{
    if (!m_serial->isOpen()) {
        return;
    }
    const QString text = m_inputEdit->text();
    const QString eol = m_eolCombo->currentData().toString();
    const QByteArray payload = (text + eol).toUtf8();

    if (m_serial->write(payload) != -1) {
        m_inputEdit->clear();
    }
}

void SerialMonitorWindow::onSerialError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError || error == QSerialPort::TimeoutError) {
        return;
    }
    m_outputEdit->append(tr("[Erreur série] %1").arg(m_serial->errorString()));
    if (m_serial->isOpen() && error == QSerialPort::ResourceError) {
        toggleConnection();
        refreshPorts();
    }
}