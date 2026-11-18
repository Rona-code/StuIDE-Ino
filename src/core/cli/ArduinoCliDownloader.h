#pragma once

#include <QString>
#include <QObject>

class ArduinoCliDownloader : public QObject {
    Q_OBJECT
public:
    explicit ArduinoCliDownloader(QObject* parent = nullptr);

    QString getLocalCliPath() const;

    void ensureCliAvailable();

signals:
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void downloadFinished(bool success, const QString& message);

private:
    void startDownload();
    void extractArchive(const QString& filePath);

    QString m_installDir;
};