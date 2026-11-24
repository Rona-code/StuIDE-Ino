#pragma once

#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QVector>

struct LibraryInfo {
    QString name;
    QString author;
    QString sentence;
    QString installedVersion;
    QString latestVersion;
    QStringList availableVersions;
};

class LibraryManager : public QObject {
    Q_OBJECT
public:
    explicit LibraryManager(const QString& cliPath, QObject* parent = nullptr);
    ~LibraryManager() override;

    void listInstalled();
    void searchLibraries(const QString& query);
    void installLibrary(const QString& name, const QString& version = QString());
    void uninstallLibrary(const QString& name);
    void updateIndex();

signals:
    void installedLibrariesLoaded(const QVector<LibraryInfo>& libraries);
    void searchResultsLoaded(const QVector<LibraryInfo>& libraries);
    void libraryActionFinished(bool success, const QString& message);
    void outputLog(const QString& text);

private:
    enum class TaskType {
        None,
        ListInstalled,
        Search,
        Install,
        Uninstall,
        UpdateIndex
    };

    void runCliCommand(const QStringList& args, TaskType task);
    void handleProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    QVector<LibraryInfo> parseInstalledJson(const QByteArray& rawJson) const;
    QVector<LibraryInfo> parseSearchJson(const QByteArray& rawJson) const;

    QString m_cliPath;
    QProcess* m_process = nullptr;
    TaskType m_currentTask = TaskType::None;
};