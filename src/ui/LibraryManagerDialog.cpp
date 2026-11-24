#include "LibraryManagerDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidgetItem>
#include <QScrollBar>

LibraryManagerDialog::LibraryManagerDialog(LibraryManager* manager, QWidget* parent)
    : QDialog(parent), m_manager(manager)
{
    setWindowTitle(tr("Gestionnaire de bibliothèques"));
    resize(720, 480);

    auto* topLayout = new QHBoxLayout();
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(tr("Rechercher une bibliothèque (ex. Servo, Adafruit GFX, DHT)..."));
    m_searchBtn = new QPushButton(tr("Rechercher"), this);
    m_installedBtn = new QPushButton(tr("Installées"), this);
    m_updateIndexBtn = new QPushButton(tr("Mettre à jour l'index"), this);

    topLayout->addWidget(m_searchEdit, 1);
    topLayout->addWidget(m_searchBtn);
    topLayout->addWidget(m_installedBtn);
    topLayout->addWidget(m_updateIndexBtn);

    m_listWidget = new QListWidget(this);

    m_detailsLabel = new QLabel(tr("Sélectionnez une bibliothèque."), this);
    m_detailsLabel->setWordWrap(true);

    auto* actionLayout = new QHBoxLayout();
    m_versionCombo = new QComboBox(this);
    m_versionCombo->setMinimumWidth(130);
    m_installBtn = new QPushButton(tr("Installer"), this);
    m_uninstallBtn = new QPushButton(tr("Désinstaller"), this);
    m_uninstallBtn->setEnabled(false);

    actionLayout->addWidget(m_versionCombo);
    actionLayout->addWidget(m_installBtn);
    actionLayout->addWidget(m_uninstallBtn);
    actionLayout->addStretch();

    m_logEdit = new QTextEdit(this);
    m_logEdit->setReadOnly(true);
    m_logEdit->setMaximumHeight(110);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(topLayout);
    mainLayout->addWidget(m_listWidget, 1);
    mainLayout->addWidget(m_detailsLabel);
    mainLayout->addLayout(actionLayout);
    mainLayout->addWidget(m_logEdit);

    connect(m_searchBtn, &QPushButton::clicked, this, &LibraryManagerDialog::onSearchTriggered);
    connect(m_searchEdit, &QLineEdit::returnPressed, this, &LibraryManagerDialog::onSearchTriggered);
    connect(m_installedBtn, &QPushButton::clicked, this, [this]() {
        if (m_manager) {
            setBusy(true);
            m_manager->listInstalled();
        }
        });
    connect(m_updateIndexBtn, &QPushButton::clicked, this, &LibraryManagerDialog::onUpdateIndexClicked);
    connect(m_listWidget, &QListWidget::currentRowChanged, this, &LibraryManagerDialog::onLibrarySelected);
    connect(m_installBtn, &QPushButton::clicked, this, &LibraryManagerDialog::onInstallClicked);
    connect(m_uninstallBtn, &QPushButton::clicked, this, &LibraryManagerDialog::onUninstallClicked);

    if (m_manager) {
        connect(m_manager, &LibraryManager::installedLibrariesLoaded, this, &LibraryManagerDialog::onInstalledLoaded);
        connect(m_manager, &LibraryManager::searchResultsLoaded, this, &LibraryManagerDialog::onSearchLoaded);
        connect(m_manager, &LibraryManager::libraryActionFinished, this, &LibraryManagerDialog::onActionFinished);
        connect(m_manager, &LibraryManager::outputLog, this, &LibraryManagerDialog::onLogReceived);

        setBusy(true);
        m_manager->listInstalled();
    }
}

void LibraryManagerDialog::setBusy(bool busy)
{
    m_searchBtn->setEnabled(!busy);
    m_installedBtn->setEnabled(!busy);
    m_updateIndexBtn->setEnabled(!busy);
    m_installBtn->setEnabled(!busy && m_listWidget->currentRow() >= 0);
    m_uninstallBtn->setEnabled(!busy && m_listWidget->currentRow() >= 0);
}

void LibraryManagerDialog::onSearchTriggered()
{
    if (!m_manager) {
        return;
    }
    setBusy(true);
    m_manager->searchLibraries(m_searchEdit->text());
}

void LibraryManagerDialog::onInstalledLoaded(const QVector<LibraryInfo>& libs)
{
    setBusy(false);
    populateList(libs, true);
}

void LibraryManagerDialog::onSearchLoaded(const QVector<LibraryInfo>& libs)
{
    setBusy(false);
    populateList(libs, false);
}

void LibraryManagerDialog::populateList(const QVector<LibraryInfo>& libs, bool fromInstalled)
{
    m_currentLibs = libs;
    m_listWidget->clear();
    m_versionCombo->clear();

    for (const LibraryInfo& lib : m_currentLibs) {
        QString verText = fromInstalled ? lib.installedVersion : lib.latestVersion;
        if (verText.isEmpty()) {
            verText = lib.installedVersion;
        }
        const QString title = verText.isEmpty()
            ? lib.name
            : QStringLiteral("%1 (%2)").arg(lib.name, verText);
        m_listWidget->addItem(title);
    }

    if (m_listWidget->count() > 0) {
        m_listWidget->setCurrentRow(0);
    }
    else {
        m_detailsLabel->setText(tr("Aucune bibliothèque trouvée."));
        m_installBtn->setEnabled(false);
        m_uninstallBtn->setEnabled(false);
    }
}

void LibraryManagerDialog::onLibrarySelected(int row)
{
    m_versionCombo->clear();
    if (row < 0 || row >= m_currentLibs.size()) {
        m_installBtn->setEnabled(false);
        m_uninstallBtn->setEnabled(false);
        return;
    }

    const LibraryInfo& lib = m_currentLibs.at(row);
    QString infoText = QStringLiteral("<b>%1</b>").arg(lib.name.toHtmlEscaped());
    if (!lib.author.isEmpty()) {
        infoText += tr(" — <i>par %1</i>").arg(lib.author.toHtmlEscaped());
    }
    if (!lib.installedVersion.isEmpty()) {
        infoText += tr(" — <b>Installée : %1</b>").arg(lib.installedVersion.toHtmlEscaped());
    }
    if (!lib.sentence.isEmpty()) {
        infoText += QStringLiteral("<br>%1").arg(lib.sentence.toHtmlEscaped());
    }
    m_detailsLabel->setText(infoText);

    if (!lib.availableVersions.isEmpty()) {
        for (int i = lib.availableVersions.size() - 1; i >= 0; --i) {
            m_versionCombo->addItem(lib.availableVersions.at(i));
        }
    }
    else if (!lib.latestVersion.isEmpty()) {
        m_versionCombo->addItem(lib.latestVersion);
    }

    m_installBtn->setEnabled(true);
    m_uninstallBtn->setEnabled(!lib.installedVersion.isEmpty());
}

void LibraryManagerDialog::onInstallClicked()
{
    const int row = m_listWidget->currentRow();
    if (!m_manager || row < 0 || row >= m_currentLibs.size()) {
        return;
    }
    const QString name = m_currentLibs.at(row).name;
    const QString version = m_versionCombo->currentText().trimmed();
    setBusy(true);
    m_manager->installLibrary(name, version);
}

void LibraryManagerDialog::onUninstallClicked()
{
    const int row = m_listWidget->currentRow();
    if (!m_manager || row < 0 || row >= m_currentLibs.size()) {
        return;
    }
    const QString name = m_currentLibs.at(row).name;
    setBusy(true);
    m_manager->uninstallLibrary(name);
}

void LibraryManagerDialog::onUpdateIndexClicked()
{
    if (!m_manager) {
        return;
    }
    setBusy(true);
    m_manager->updateIndex();
}

void LibraryManagerDialog::onActionFinished(bool success, const QString& message)
{
    setBusy(false);
    m_logEdit->append(message);
    if (success && m_manager) {
        m_manager->listInstalled();
    }
}

void LibraryManagerDialog::onLogReceived(const QString& text)
{
    m_logEdit->insertPlainText(text);
    QScrollBar* bar = m_logEdit->verticalScrollBar();
    bar->setValue(bar->maximum());
}