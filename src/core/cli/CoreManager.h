#pragma once

#include <QString>
#include <QObject>
#include <QProcess>
#include <QVector>

struct CorePackageInfo {
	QString id;
	QString name;
	QString maintainer;
	QString installedVersion;
	QStringList versions;
	QStringList boards;
};

class CoreManager : public QObject {
    Q_OBJECT
public:
    explicit CoreManager(const QString& cliPath, QObject* parent = nullptr);

    void installCore(const QString& coreName);
	void uninstallCore(const QString& coreName);
	void updateIndex();
	void addAdditionalUrl(const QString& url);
	void fetchCores();

signals:
    void coreInstallFinished(bool success, const QString& message);
	void coreUninstallFinished(bool success, const QString& message);
	void indexUpdateFinished(bool success, const QString& message);
	void coresLoaded(const QVector<CorePackageInfo>& cores);
	void coreError(const QString& message);

private:
    QString m_cliPath;
};