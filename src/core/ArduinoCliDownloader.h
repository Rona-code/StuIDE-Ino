#pragma once

#include <QString>
#include <QObject>

class ArduinoCliDownloader : public QObject {
    Q_OBJECT
public:
    explicit ArduinoCliDownloader(QObject* parent = nullptr);

    // Retourne le chemin complet vers l'exécutable local s'il existe
    QString getLocalCliPath() const;

    // Vérifie la présence de l'outil et lance le téléchargement si besoin
    void ensureCliAvailable();

signals:
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void downloadFinished(bool success, const QString& message);

private:
    void startDownload();
    void extractArchive(const QString& filePath);

    QString m_installDir;
};