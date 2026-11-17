#include "ArduinoCliDownloader.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QCoreApplication>
#include <QProcess>

#// URL officielle de la dernière release d'arduino-cli selon l'OS
#ifdef Q_OS_WIN
const QString DOWNLOAD_URL = "https://github.com/arduino/arduino-cli/releases/download/v1.6.0/arduino-cli_1.6.0_Windows_64bit.zip";
const QString EXE_NAME = "arduino-cli.exe";
#elif defined(Q_OS_LINUX)
#ifdef __aarch64__
const QString DOWNLOAD_URL = "https://github.com/arduino/arduino-cli/releases/download/v1.6.0/arduino-cli_1.6.0_Linux_ARM64.tar.gz";
#else
const QString DOWNLOAD_URL = "https://github.com/arduino/arduino-cli/releases/download/v1.6.0/arduino-cli_1.6.0_Linux_64bit.tar.gz";
#endif
const QString EXE_NAME = "arduino-cli";
#endif

ArduinoCliDownloader::ArduinoCliDownloader(QObject* parent) : QObject(parent) {
    // Dossier local sécurisé : AppData/Local/StuIDE-Ino/bin (Windows) ou ~/.local/share/StuIDE-Ino/bin (Linux)
    m_installDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/bin";
}

QString ArduinoCliDownloader::getLocalCliPath() const {
    QDir dir(m_installDir);
    return dir.absoluteFilePath(EXE_NAME);
}

void ArduinoCliDownloader::ensureCliAvailable() {
    if (QFile::exists(getLocalCliPath())) {
        emit downloadFinished(true, "arduino-cli est déjà présent.");
        return;
    }

    // Si absent, on lance le téléchargement
    startDownload();
}

void ArduinoCliDownloader::startDownload() 
{
    QDir().mkpath(m_installDir);

    QNetworkAccessManager* manager = new QNetworkAccessManager(this);

    // Correction ici : on instancie QNetworkRequest correctement
    QNetworkRequest request;
    request.setUrl(QUrl(DOWNLOAD_URL));
    request.setHeader(QNetworkRequest::UserAgentHeader, "StuIDE-Ino");

    // On passe bien l'objet request par référence
    QNetworkReply* reply = manager->get(request);

    connect(reply, &QNetworkReply::downloadProgress, this, &ArduinoCliDownloader::downloadProgress);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QString archivePath = m_installDir + (DOWNLOAD_URL.endsWith(".zip") ? "/cli.zip" : "/cli.tar.gz");

            QFile file(archivePath);
            if (file.open(QIODevice::WriteOnly)) {
                file.write(reply->readAll());
                file.close();

                extractArchive(archivePath);
                file.remove(); // Nettoyage de l'archive

                emit downloadFinished(true, "Téléchargement et installation réussis !");
            }
            else {
                emit downloadFinished(false, "Impossible d'écrire l'archive sur le disque.");
            }
        }
        else {
            emit downloadFinished(false, "Erreur de téléchargement : " + reply->errorString());
        }
        reply->deleteLater();
        });
}

void ArduinoCliDownloader::extractArchive(const QString& filePath) {
    QStringList args;
#ifdef Q_OS_WIN
    args << "-xf" << filePath << "-C" << m_installDir;
    QProcess::execute("tar", args);
#else
    args << "-xzf" << filePath << "-C" << m_installDir;
    QProcess::execute("tar", args);
    QProcess::execute("chmod", QStringList() << "+x" << getLocalCliPath());
#endif
}