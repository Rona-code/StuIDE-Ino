#pragma once

#include <QDialog>
#include <QVector>
#include <QLineEdit>
#include <QPushButton>
#include <QListWidget>
#include <QLabel>
#include <QComboBox>
#include <QTextEdit>
#include "../core/cli/LibraryManager.h"

class LibraryManagerDialog : public QDialog {
    Q_OBJECT
public:
    explicit LibraryManagerDialog(LibraryManager* manager, QWidget* parent = nullptr);

private slots:
    void onSearchTriggered();
    void onInstalledLoaded(const QVector<LibraryInfo>& libs);
    void onSearchLoaded(const QVector<LibraryInfo>& libs);
    void onLibrarySelected(int row);
    void onInstallClicked();
    void onUninstallClicked();
    void onUpdateIndexClicked();
    void onActionFinished(bool success, const QString& message);
    void onLogReceived(const QString& text);

private:
    void populateList(const QVector<LibraryInfo>& libs, bool fromInstalled);
    void setBusy(bool busy);

    LibraryManager* m_manager = nullptr;
    QVector<LibraryInfo> m_currentLibs;

    QLineEdit* m_searchEdit = nullptr;
    QPushButton* m_searchBtn = nullptr;
    QPushButton* m_installedBtn = nullptr;
    QPushButton* m_updateIndexBtn = nullptr;
    QListWidget* m_listWidget = nullptr;
    QLabel* m_detailsLabel = nullptr;
    QComboBox* m_versionCombo = nullptr;
    QPushButton* m_installBtn = nullptr;
    QPushButton* m_uninstallBtn = nullptr;
    QTextEdit* m_logEdit = nullptr;
};