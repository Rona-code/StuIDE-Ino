#include "CoreManager.h"
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>

CoreManager::CoreManager(const QString& cliPath, QObject* parent)
    : QObject(parent), m_cliPath(cliPath) {}

void CoreManager::installCore(const QString& coreName) {
    QProcess* process = new QProcess(this);

    connect(process, &QProcess::finished, this, [this, process, coreName](int exitCode, QProcess::ExitStatus) {
        if (exitCode == 0) {
            emit coreInstallFinished(true, tr("Cœur %1 installé avec succès !").arg(coreName));
        }
        else {
            QByteArray errOutput = process->readAllStandardError();
            QString errorMsg = QString::fromUtf8(errOutput);
            emit coreInstallFinished(false, tr("Échec de l'installation du cœur %1 : %2").arg(coreName, errorMsg));
        }
        process->deleteLater();
        });

    // Commande : arduino-cli core install <coreName>
    process->start(m_cliPath, { "core", "install", coreName });
}

void CoreManager::uninstallCore(const QString& coreName) {
    QProcess* process = new QProcess(this);

    connect(process, &QProcess::finished, this, [this, process, coreName](int exitCode, QProcess::ExitStatus) {
        if (exitCode == 0) {
            emit coreUninstallFinished(true, tr("Cœur %1 désinstallé.").arg(coreName));
        }
        else {
            QString errorMsg = QString::fromUtf8(process->readAllStandardError());
            emit coreUninstallFinished(false, tr("Échec de la désinstallation de %1 : %2").arg(coreName, errorMsg));
        }
        process->deleteLater();
        });

    process->start(m_cliPath, { "core", "uninstall", coreName });
}

void CoreManager::updateIndex() {
    QProcess* process = new QProcess(this);

    connect(process, &QProcess::finished, this, [this, process](int exitCode, QProcess::ExitStatus) {
        if (exitCode == 0) {
            emit indexUpdateFinished(true, tr("Index des cartes mis à jour."));
        }
        else {
            QString errorMsg = QString::fromUtf8(process->readAllStandardError());
            emit indexUpdateFinished(false, tr("Échec de mise à jour de l'index : %1").arg(errorMsg));
        }
        process->deleteLater();
        });

    process->start(m_cliPath, { "core", "update-index" });
}

void CoreManager::addAdditionalUrl(const QString& url) {
    QProcess* process = new QProcess(this);

    connect(process, &QProcess::finished, this, [this, process, url](int exitCode, QProcess::ExitStatus) {
        if (exitCode == 0) {
            updateIndex();
        }
        else {
            QString errorMsg = QString::fromUtf8(process->readAllStandardError());
            emit coreError(tr("Impossible d'ajouter l'URL %1 : %2").arg(url, errorMsg));
        }
        process->deleteLater();
        });

    process->start(m_cliPath, { "config", "add", "board_manager.additional_urls", url });
}

void CoreManager::fetchCores() {
    // 1. Récupère les cœurs déjà installés
    auto* listProc = new QProcess(this);
    connect(listProc, &QProcess::finished, this, [this, listProc](int exitCode, QProcess::ExitStatus) {
        QMap<QString, QString> installedVersions;
        if (exitCode == 0) {
            QJsonDocument doc = QJsonDocument::fromJson(listProc->readAllStandardOutput());

            auto extractInstalled = [&installedVersions](const QJsonArray& arr) {
                for (const auto& val : arr) {
                    QJsonObject obj = val.toObject();
                    QString id = obj["id"].toString();
                    if (id.isEmpty()) {
                        id = obj["package_id"].toString();
                    }

                    // Selon la version d'arduino-cli, la version installée peut être sous "version" ou "installed_version"
                    QString ver = obj["installed_version"].toString();
                    if (ver.isEmpty()) {
                        ver = obj["version"].toString();
                    }
                    if (ver.isEmpty() && obj["release"].isObject()) {
                        ver = obj["release"].toObject()["version"].toString();
                    }

                    if (!id.isEmpty() && !ver.isEmpty()) {
                        installedVersions[id] = ver;
                    }
                }
                };

            if (doc.isArray()) {
                extractInstalled(doc.array());
            }
            else if (doc.isObject()) {
                QJsonObject root = doc.object();
                if (root.contains("installed_platforms") && root["installed_platforms"].isArray()) {
                    extractInstalled(root["installed_platforms"].toArray());
                }
                else if (root.contains("platforms") && root["platforms"].isArray()) {
                    extractInstalled(root["platforms"].toArray());
                }
            }
        }
        listProc->deleteLater();

        // 2. Récupère le catalogue disponible via core search
        auto* searchProc = new QProcess(this);
        connect(searchProc, &QProcess::finished, this, [this, searchProc, installedVersions](int sExit, QProcess::ExitStatus) {
            QVector<CorePackageInfo> cores;
            if (sExit == 0) {
                QJsonDocument sDoc = QJsonDocument::fromJson(searchProc->readAllStandardOutput());
                QJsonArray platformsArr;
                if (sDoc.isObject()) {
                    platformsArr = sDoc.object()["platforms"].toArray();
                }
                else if (sDoc.isArray()) {
                    platformsArr = sDoc.array();
                }

                for (const auto& pVal : platformsArr) {
                    QJsonObject pObj = pVal.toObject();
                    QJsonObject releaseObj = pObj["release"].toObject();

                    CorePackageInfo info;
                    info.id = pObj["id"].toString();
                    info.name = releaseObj["name"].toString();
                    if (info.name.isEmpty()) {
                        info.name = info.id;
                    }
                    info.maintainer = pObj["maintainer"].toString();
                    info.installedVersion = installedVersions.value(info.id);

                    QJsonObject allReleases = pObj["releases"].toObject();
                    info.versions = allReleases.keys();
                    // Tri décroissant approximatif pour mettre la version la plus récente en haut
                    std::sort(info.versions.begin(), info.versions.end(), [](const QString& a, const QString& b) {
                        return a > b;
                        });

                    QJsonArray boardsArr = releaseObj["boards"].toArray();
                    for (const auto& bVal : boardsArr) {
                        QString bName = bVal.toObject()["name"].toString();
                        if (!bName.isEmpty()) {
                            info.boards.append(bName);
                        }
                    }

                    if (!info.id.isEmpty()) {
                        cores.append(info);
                    }
                }
            }
            else {
                emit coreError(QString::fromUtf8(searchProc->readAllStandardError()));
            }
            searchProc->deleteLater();
            emit coresLoaded(cores);
            });

        searchProc->start(m_cliPath, { "core", "search", "--format", "json" });
        });

    listProc->start(m_cliPath, { "core", "list", "--format", "json" });
}