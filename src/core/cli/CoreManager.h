#pragma once

#include <QString>
#include <QObject>
#include <QProcess>

class CoreManager : public QObject {
    Q_OBJECT
public:
    explicit CoreManager(const QString& cliPath, QObject* parent = nullptr);

    void installCore(const QString& coreName);

signals:
    void coreInstallFinished(bool success, const QString& message);

private:
    QString m_cliPath;
};