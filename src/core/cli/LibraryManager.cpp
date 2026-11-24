#include "LibraryManager.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>

LibraryManager::LibraryManager(const QString& cliPath, QObject* parent)
    : QObject(parent), m_cliPath(cliPath), m_process(new QProcess(this))
{
    connect(m_process, &QProcess::readyReadStandardError, this, [this]() {
        const QString err = QString::fromUtf8(m_process->readAllStandardError());
        if (!err.isEmpty()) {
            emit outputLog(err);
        }
        });

    connect(m_process, &QProcess::readyReadStandardOutput, this, [this]() {
        if (m_currentTask == TaskType::Install
            || m_currentTask == TaskType::Uninstall
            || m_currentTask == TaskType::UpdateIndex) {
            const QString out = QString::fromUtf8(m_process->readAllStandardOutput());
            if (!out.isEmpty()) {
                emit outputLog(out);
            }
        }
        });

    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
        this, &LibraryManager::handleProcessFinished);
}

LibraryManager::~LibraryManager()
{
    if (m_process) {
        m_process->disconnect(this);
        if (m_process->state() != QProcess::NotRunning) {
            m_process->kill();
            m_process->waitForFinished(1000);
        }
    }
}

void LibraryManager::runCliCommand(const QStringList& args, TaskType task)
{
    if (m_cliPath.isEmpty()) {
        emit libraryActionFinished(false, tr("Chemin vers arduino-cli introuvable."));
        return;
    }

    if (m_process->state() != QProcess::NotRunning) {
        emit libraryActionFinished(false, tr("Une opération sur les bibliothèques est déjà en cours."));
        return;
    }

    m_currentTask = task;
    m_process->start(m_cliPath, args);
}

void LibraryManager::listInstalled()
{
    runCliCommand({ QStringLiteral("lib"), QStringLiteral("list"), QStringLiteral("--format"), QStringLiteral("json") },
        TaskType::ListInstalled);
}

void LibraryManager::searchLibraries(const QString& query)
{
    QStringList args = { QStringLiteral("lib"), QStringLiteral("search") };
    const QString trimmed = query.trimmed();
    if (!trimmed.isEmpty()) {
        args << trimmed;
    }
    args << QStringLiteral("--format") << QStringLiteral("json");
    runCliCommand(args, TaskType::Search);
}

void LibraryManager::installLibrary(const QString& name, const QString& version)
{
    const QString cleanName = name.trimmed();
    if (cleanName.isEmpty()) {
        emit libraryActionFinished(false, tr("Nom de bibliothèque vide."));
        return;
    }

    const QString spec = version.trimmed().isEmpty()
        ? cleanName
        : QStringLiteral("%1@%2").arg(cleanName, version.trimmed());

    emit outputLog(tr("Installation de %1...\n").arg(spec));
    runCliCommand({ QStringLiteral("lib"), QStringLiteral("install"), spec }, TaskType::Install);
}

void LibraryManager::uninstallLibrary(const QString& name)
{
    const QString cleanName = name.trimmed();
    if (cleanName.isEmpty()) {
        emit libraryActionFinished(false, tr("Nom de bibliothèque vide."));
        return;
    }

    emit outputLog(tr("Désinstallation de %1...\n").arg(cleanName));
    runCliCommand({ QStringLiteral("lib"), QStringLiteral("uninstall"), cleanName }, TaskType::Uninstall);
}

void LibraryManager::updateIndex()
{
    emit outputLog(tr("Mise à jour de l'index des bibliothèques...\n"));
    runCliCommand({ QStringLiteral("lib"), QStringLiteral("update-index") }, TaskType::UpdateIndex);
}

void LibraryManager::handleProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    const TaskType finishedTask = m_currentTask;
    m_currentTask = TaskType::None;

    const QByteArray stdOut = m_process->readAllStandardOutput();
    const bool ok = (exitStatus == QProcess::NormalExit && exitCode == 0);

    switch (finishedTask) {
    case TaskType::ListInstalled:
        if (ok) {
            emit installedLibrariesLoaded(parseInstalledJson(stdOut));
        }
        else {
            emit libraryActionFinished(false, tr("Échec de la récupération des bibliothèques installées."));
        }
        break;

    case TaskType::Search:
        if (ok) {
            emit searchResultsLoaded(parseSearchJson(stdOut));
        }
        else {
            emit libraryActionFinished(false, tr("Échec de la recherche de bibliothèques."));
        }
        break;

    case TaskType::Install:
        emit libraryActionFinished(ok, ok
            ? tr("Bibliothèque installée avec succès.")
            : tr("Échec de l'installation de la bibliothèque."));
        break;

    case TaskType::Uninstall:
        emit libraryActionFinished(ok, ok
            ? tr("Bibliothèque désinstallée.")
            : tr("Échec de la désinstallation de la bibliothèque."));
        break;

    case TaskType::UpdateIndex:
        emit libraryActionFinished(ok, ok
            ? tr("Index des bibliothèques mis à jour.")
            : tr("Échec de la mise à jour de l'index."));
        break;

    case TaskType::None:
        break;
    }
}

QVector<LibraryInfo> LibraryManager::parseInstalledJson(const QByteArray& rawJson) const
{
    QVector<LibraryInfo> result;
    const QJsonDocument doc = QJsonDocument::fromJson(rawJson);
    if (!doc.isObject()) {
        return result;
    }

    const QJsonArray installedArray = doc.object().value(QStringLiteral("installed_libraries")).toArray();
    for (const QJsonValue& val : installedArray) {
        const QJsonObject entry = val.toObject();
        const QJsonObject libObj = entry.value(QStringLiteral("library")).toObject();

        LibraryInfo info;
        info.name = libObj.value(QStringLiteral("name")).toString();
        info.author = libObj.value(QStringLiteral("author")).toString();
        info.sentence = libObj.value(QStringLiteral("sentence")).toString();
        info.installedVersion = libObj.value(QStringLiteral("version")).toString();

        if (!info.name.isEmpty()) {
            result.append(info);
        }
    }

    return result;
}

QVector<LibraryInfo> LibraryManager::parseSearchJson(const QByteArray& rawJson) const
{
    QVector<LibraryInfo> result;
    const QJsonDocument doc = QJsonDocument::fromJson(rawJson);
    if (!doc.isObject()) {
        return result;
    }

    const QJsonArray libsArray = doc.object().value(QStringLiteral("libraries")).toArray();
    for (const QJsonValue& val : libsArray) {
        const QJsonObject libObj = val.toObject();

        LibraryInfo info;
        info.name = libObj.value(QStringLiteral("name")).toString();

        const QJsonObject latestObj = libObj.value(QStringLiteral("latest")).toObject();
        info.latestVersion = latestObj.value(QStringLiteral("version")).toString();
        info.author = latestObj.value(QStringLiteral("author")).toString();
        info.sentence = latestObj.value(QStringLiteral("sentence")).toString();

        const QJsonArray versionsArr = libObj.value(QStringLiteral("available_versions")).toArray();
        for (const QJsonValue& v : versionsArr) {
            const QString verStr = v.toString().trimmed();
            if (!verStr.isEmpty()) {
                info.availableVersions.append(verStr);
            }
        }

        if (!info.name.isEmpty()) {
            result.append(info);
        }
    }

    return result;
}