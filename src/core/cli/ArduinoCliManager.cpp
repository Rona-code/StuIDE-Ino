#include "ArduinoCliManager.h"
#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QDebug>

ArduinoCliManager::ArduinoCliManager(QObject* parent) : QObject(parent) {}

// --- Gestion de Carte ---
QString ArduinoCliManager::getCliPath() const {
    QString exeName =
#ifdef Q_OS_WIN
        "arduino-cli.exe";
#else
        "arduino-cli";
#endif

    QString standardPath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);

    QStringList possiblePaths;

    possiblePaths.append(standardPath + "/bin/" + exeName);

    QDir standardDir(standardPath);
    QString dirName = standardDir.dirName();
    standardDir.cdUp();
    possiblePaths.append(standardDir.absolutePath() + "/Stusoft/" + dirName + "/bin/" + exeName);

    possiblePaths.append(QCoreApplication::applicationDirPath() + "/bin/" + exeName);
    possiblePaths.append(QCoreApplication::applicationDirPath() + "/" + exeName);

    for (const QString& path : possiblePaths) {
        if (QFile::exists(path)) {
            return path;
        }
    }

    return standardPath + "/bin/" + exeName;
}

void ArduinoCliManager::refreshBoards() {
    QProcess* process = new QProcess(this);

    connect(process, &QProcess::errorOccurred, this, [this, process](QProcess::ProcessError error) {
        qDebug() << tr("ERREUR CRITIQUE QProcess : Impossible de lancer arduino-cli. Code erreur :") << error;
        emit errorOccurred(tr("Erreur critique de lancement du CLI (Code : %1)").arg(error));
        process->deleteLater();
        });

    connect(process, &QProcess::finished, this, [this, process](int exitCode, QProcess::ExitStatus) {
        qDebug() << tr("Processus terminé avec le code exit :") << exitCode;

        QList<BoardInfo> boardList;

        if (exitCode == 0) {
            QByteArray output = process->readAllStandardOutput();

            if (output.trimmed().isEmpty()) {
                process->deleteLater();
                return;
            }

            QJsonDocument doc = QJsonDocument::fromJson(output);

            if (doc.isObject()) {
                QJsonObject rootObj = doc.object();
                QJsonArray ports = rootObj["detected_ports"].toArray();

                for (const auto& pVal : ports) {
                    QJsonObject pObj = pVal.toObject();
                    QJsonObject portObj = pObj["port"].toObject();
                    QString port = portObj["address"].toString();
                    QJsonObject props = portObj["properties"].toObject();
                    QString vid = props["vid"].toString();

                    BoardInfo info;
                    info.port = port;

                    QJsonArray matchingBoards = pObj["matching_boards"].toArray();
                    if (!matchingBoards.isEmpty()) {
                        QJsonObject bObj = matchingBoards[0].toObject();
                        info.name = bObj["name"].toString();
                        info.fqbn = bObj["fqbn"].toString();
                    }
                    else {
                        if (vid == "0x1A86") {
                            info.name = QString(tr("Carte compatible (CH340)"));
                        }
                        else if (vid == "0x10C4") {
                            info.name = QString(tr("Carte compatible (CP210x)")).arg(port);
                        }
                        else {
                            info.name = QString(tr("Port série inconnu")).arg(port);
                        }
                        info.fqbn = "";
                    }

                    if (!port.isEmpty()) {
                        boardList.append(info);
                    }
                }
            }

            emit boardsUpdated(boardList);
        }
        else {
            QByteArray errOutput = process->readAllStandardError();
            qDebug() << "Erreur standard arduino-cli :" << QString::fromUtf8(errOutput);
            emit errorOccurred(tr("arduino-cli a échoué. Sortie : ") + QString::fromUtf8(errOutput));
        }

        process->deleteLater();
        });

    QString cliPath = getCliPath();
    qDebug() << tr("Chemin utilisé pour arduino-cli :") << cliPath;

    process->start(cliPath, { "board", "list", "--format", "json" });
}

// --- Compilation ---
void ArduinoCliManager::compileSketch(const QString& sketchPath, const QString& fqbn) {
    m_lastCompileSuccess = false;
    QProcess* process = new QProcess(this);

	connect(process, &QProcess::readyReadStandardOutput, this, [this, process]() {
		QString output = QString::fromUtf8(process->readAllStandardOutput());
		emit compilationOutput(output);
        qDebug().noquote() << output;
    });

	connect(process, &QProcess::readyReadStandardError, this, [this, process]() {
		QString errorOutput = QString::fromUtf8(process->readAllStandardError());
		emit compilationError(errorOutput);
		qDebug().noquote() << errorOutput;
	});

	connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [this, process](int exitCode,QProcess::ExitStatus) {
		if (exitCode != 0) {
            m_lastCompileSuccess = false;
            emit compilationFinished(false, tr("Erreur de compilation. Code exit : %1").arg(exitCode));
		}
		else {
		    m_lastCompileSuccess = true;
            emit compilationFinished(true, tr("Compilation réussie !"));
		}
		process->deleteLater();
	});

	QString cliPath = getCliPath();
	process->start(cliPath, { "compile", "--fqbn", fqbn, sketchPath });
}

void ArduinoCliManager::uploadSketch(const QString& sketchPath, const QString& fqbn, const QString& port) {
	QProcess* process = new QProcess(this);

	connect(process, &QProcess::readyReadStandardOutput, this, [this, process]() {
		QString output = QString::fromUtf8(process->readAllStandardOutput());
		emit compilationOutput(output);
		qDebug().noquote() << output;
	});

	connect(process, &QProcess::readyReadStandardError, this, [this, process]() {
		QString errorOutput = QString::fromUtf8(process->readAllStandardError());
		emit compilationError(errorOutput);
		qDebug().noquote() << errorOutput;
	});

	connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [this, process](int exitCode, QProcess::ExitStatus) {
		if (exitCode != 0) {
			emit compilationFinished(false, tr("Erreur de téléversement. Code exit : %1").arg(exitCode));
		}
		else {
			emit compilationFinished(true, tr("Téléversement réussi !"));
		}
		process->deleteLater();
	});

	QString cliPath = getCliPath();

	process->start(cliPath, { "upload", "--fqbn", fqbn, "--port", port, sketchPath });
}