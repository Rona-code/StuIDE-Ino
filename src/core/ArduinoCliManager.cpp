#include "ArduinoCliManager.h"
#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCoreApplication>
#include <QDir>
#include <QFile>

ArduinoCliManager::ArduinoCliManager(QObject* parent) : QObject(parent) {}

QString ArduinoCliManager::getCliPath() const {
    // Détermine le nom de l'exécutable selon l'OS
    QString exeName =
#ifdef Q_OS_WIN
        "arduino-cli.exe";
#else
        "arduino-cli";
#endif

    // Vérifie si arduino-cli est présent dans le dossier de l'exécutable de l'IDE
    QDir appDir(QCoreApplication::applicationDirPath());
    QString localPath = appDir.absoluteFilePath(exeName);
    if (QFile::exists(localPath)) {
        return localPath;
    }

    return exeName;
}

void ArduinoCliManager::refreshBoards() {
    QProcess* process = new QProcess(this);

    connect(process, &QProcess::finished, this, [this, process](int exitCode, QProcess::ExitStatus) {
        QList<BoardInfo> boardList;

        if (exitCode == 0) {
            QByteArray output = process->readAllStandardOutput();
            QJsonDocument doc = QJsonDocument::fromJson(output);

            if (doc.isObject()) {
                QJsonObject rootObj = doc.object();
                QJsonArray ports = rootObj["detected_ports"].toArray();

                for (const auto& pVal : ports) {
                    QJsonObject pObj = pVal.toObject();
                    QString port = pObj["port"].toObject()["address"].toString();

                    QJsonArray matchingBoards = pObj["matching_boards"].toArray();
                    if (!matchingBoards.isEmpty()) {
                        QJsonObject bObj = matchingBoards[0].toObject();
                        BoardInfo info;
                        info.name = bObj["name"].toString();
                        info.fqbn = bObj["fqbn"].toString();
                        info.port = port;
                        boardList.append(info);
                    }
                    else {
                        // Cas d'un port série actif mais sans correspondance exacte (ex: clone CH340)
                        BoardInfo info;
                        info.name = QString("Port inconnu (%1)").arg(port);
                        info.fqbn = "";
                        info.port = port;
                        boardList.append(info);
                    }
                }
            }
            emit boardsUpdated(boardList);
        }
        else {
            emit errorOccurred(tr("Impossible d'exécuter arduino-cli. Vérifiez son installation."));
        }

        process->deleteLater();
        });

    // Lance la commande en arrière-plan : arduino-cli board list --format json
    process->start(getCliPath(), { "board", "list", "--format", "json" });
}