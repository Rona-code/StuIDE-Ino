#include "CoreManager.h"
#include <QDebug>

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