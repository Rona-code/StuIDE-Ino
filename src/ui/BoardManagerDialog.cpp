#include "BoardManagerDialog.h"
#include "../core/cli/CoreManager.h"

#include <QComboBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPointer>
#include <QPushButton>
#include <QScrollArea>
#include <QStringList>
#include <QStyle>
#include <QVBoxLayout>
#include <functional>

namespace {

    // Carte représentant un paquet de cartes (style IDE 2.x)
    QFrame* createCoreCard(
        const CorePackageInfo& core,
        QWidget* parent,
        bool busy,
        std::function<void(const QString&, const QString&)> onInstall,
        std::function<void(const QString&)> onRemove)
    {
        auto* card = new QFrame(parent);
        card->setObjectName("coreCard");
        card->setFrameShape(QFrame::StyledPanel);
        card->setStyleSheet(
            "QFrame#coreCard {"
            "  border: 1px solid #d0d7de;"
            "  border-radius: 6px;"
            "  background-color: #ffffff;"
            "  padding: 10px;"
            "}"
            "QLabel {"
            "  background-color: transparent;"
            "  color: #1f2328;"
            "  border: none;"
            "  padding: 0px;"
            "}"
            "QComboBox {"
            "  background-color: #f6f8fa;"
            "  color: #1f2328;"
            "  border: 1px solid #c5cdd5;"
            "  border-radius: 4px;"
            "  padding: 4px 8px;"
            "  min-width: 150px;"
            "}"
            "QComboBox QAbstractItemView {"
            "  background-color: #ffffff;"
            "  color: #1f2328;"
            "  selection-background-color: #0969da;"
            "  selection-color: #ffffff;"
            "}"
            "QPushButton {"
            "  background-color: #0969da;"
            "  color: #ffffff;"
            "  border: none;"
            "  border-radius: 4px;"
            "  padding: 6px 12px;"
            "  font-weight: bold;"
            "}"
            "QPushButton:hover {"
            "  background-color: #0550ae;"
            "}"
            "QPushButton:disabled {"
            "  background-color: #8c959f;"
            "}"
        );

        auto* mainRow = new QHBoxLayout(card);
        mainRow->setSpacing(14);
        mainRow->setContentsMargins(6, 6, 6, 6);

        // Colonne gauche : titre, auteur, statut et aperçu des cartes
        auto* infoCol = new QVBoxLayout();
        infoCol->setSpacing(4);

        auto* titleLayout = new QHBoxLayout();
        auto* titleLabel = new QLabel(
            QString("<span style='font-size:13px; font-weight:bold; color:#1f2328;'>%1</span> "
                "<span style='color:#57606a;'>by %2</span>")
            .arg(core.name.toHtmlEscaped(), core.maintainer.toHtmlEscaped()),
            card);
        titleLabel->setTextFormat(Qt::RichText);
        titleLayout->addWidget(titleLabel);

        if (!core.installedVersion.isEmpty()) {
            auto* badge = new QLabel(QString(" %1 installed ").arg(core.installedVersion), card);
            badge->setStyleSheet(
                "background-color: #dafbe1;"
                "color: #116329;"
                "border-radius: 4px;"
                "padding: 2px 6px;"
                "font-weight: bold;"
            );
            titleLayout->addWidget(badge);
        }

        titleLayout->addStretch();
        infoCol->addLayout(titleLayout);

        if (!core.boards.isEmpty()) {
            const QString previewBoards = core.boards.mid(0, 6).join(", ")
                + (core.boards.size() > 6 ? "..." : "");
            auto* boardsLabel = new QLabel(QObject::tr("Cartes incluses : %1").arg(previewBoards), card);
            boardsLabel->setWordWrap(true);
            boardsLabel->setStyleSheet("color: #424a53;");
            infoCol->addWidget(boardsLabel);
        }

        mainRow->addLayout(infoCol, 1);

        // Colonne droite : sélection de version + actions
        auto* actionCol = new QVBoxLayout();
        actionCol->setSpacing(6);

        auto* versionCombo = new QComboBox(card);
        if (!core.versions.isEmpty()) {
            versionCombo->addItems(core.versions);
        }
        else if (!core.installedVersion.isEmpty()) {
            versionCombo->addItem(core.installedVersion);
        }
        versionCombo->setEnabled(!busy);
        actionCol->addWidget(versionCombo);

        auto* btnRow = new QHBoxLayout();
        auto* installBtn = new QPushButton(
            core.installedVersion.isEmpty() ? QObject::tr("INSTALL") : QObject::tr("UPDATE"),
            card);
        installBtn->setEnabled(!busy);

        QPointer<QComboBox> versionComboPtr(versionCombo);
        QObject::connect(installBtn, &QPushButton::clicked, card, [onInstall, coreId = core.id, versionComboPtr]() {
            const QString selectedVersion = versionComboPtr ? versionComboPtr->currentText() : QString();
            if (onInstall) {
                onInstall(coreId, selectedVersion);
            }
            });
        btnRow->addWidget(installBtn);

        if (!core.installedVersion.isEmpty()) {
            auto* removeBtn = new QPushButton(QObject::tr("REMOVE"), card);
            removeBtn->setEnabled(!busy);
            removeBtn->setStyleSheet(
                "QPushButton {"
                "  background-color: #cf222e;"
                "  color: #ffffff;"
                "}"
                "QPushButton:hover {"
                "  background-color: #a40e26;"
                "}"
                "QPushButton:disabled {"
                "  background-color: #8c959f;"
                "}"
            );
            QObject::connect(removeBtn, &QPushButton::clicked, card, [onRemove, coreId = core.id]() {
                if (onRemove) {
                    onRemove(coreId);
                }
                });
            btnRow->addWidget(removeBtn);
        }

        actionCol->addLayout(btnRow);
        mainRow->addLayout(actionCol);

        return card;
    }

} // namespace

BoardManagerDialog::BoardManagerDialog(CoreManager* coreManager, QWidget* parent)
    : QDialog(parent)
    , m_coreManager(coreManager)
{
    setupUi();

    if (m_coreManager) {
        connect(m_coreManager, &CoreManager::coresLoaded, this, [this](const QVector<CorePackageInfo>& cores) {
            m_allCores = cores;
            m_busy = false;
            setStatusText(tr("Liste à jour (%1 paquets).").arg(cores.size()));
            applyFilters();
            });

        connect(m_coreManager, &CoreManager::coreInstallFinished, this, [this](bool ok, const QString& msg) {
            m_busy = false;
            if (ok) {
                emit coresChanged();
                setStatusText(msg.isEmpty() ? tr("Installation terminée, actualisation...") : msg);
                m_coreManager->fetchCores();
            }
            else {
                setStatusText(msg.isEmpty() ? tr("Échec de l'installation.") : msg, true);
                applyFilters();
            }
            });

        connect(m_coreManager, &CoreManager::coreUninstallFinished, this, [this](bool ok, const QString& msg) {
            m_busy = false;
            if (ok) {
                emit coresChanged();
                setStatusText(msg.isEmpty() ? tr("Désinstallation terminée, actualisation...") : msg);
                m_coreManager->fetchCores();
            }
            else {
                setStatusText(msg.isEmpty() ? tr("Échec de la désinstallation.") : msg, true);
                applyFilters();
            }
            });

        connect(m_coreManager, &CoreManager::coreError, this, [this](const QString& err) {
            m_busy = false;
            setStatusText(err, true);
            applyFilters();
            });

        m_busy = true;
        setStatusText(tr("Chargement des paquets..."));
        m_coreManager->fetchCores();
    }
    else {
        applyFilters();
    }
}

BoardManagerDialog::~BoardManagerDialog()
{
    if (m_coreManager) {
        m_coreManager->disconnect(this);
        m_coreManager->blockSignals(true);
    }
}

void BoardManagerDialog::setupUi()
{
    setWindowTitle(tr("Gestionnaire de cartes"));
    resize(740, 540);

    auto* mainLayout = new QVBoxLayout(this);

    // Barre de recherche et filtre
    auto* filterLayout = new QHBoxLayout();
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(tr("Filtrer votre recherche (ex. ESP32, AVR, Nano)..."));

    m_typeCombo = new QComboBox(this);
    m_typeCombo->addItem(tr("Tout"), "all");
    m_typeCombo->addItem(tr("Installés"), "installed");

    filterLayout->addWidget(m_searchEdit, 1);
    filterLayout->addWidget(m_typeCombo);
    mainLayout->addLayout(filterLayout);

    // Zone défilante des cartes
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);

    m_listContainer = new QWidget(m_scrollArea);
    m_listContainer->setStyleSheet("background-color: #f6f8fa;");

    m_listLayout = new QVBoxLayout(m_listContainer);
    m_listLayout->setSpacing(8);
    m_listLayout->addStretch();

    m_scrollArea->setWidget(m_listContainer);
    mainLayout->addWidget(m_scrollArea);

    // Barre de statut en bas
    m_statusLabel = new QLabel(tr("Prêt."), this);
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setStyleSheet(
        "QLabel { color: #57606a; padding-top: 4px; }"
        "QLabel[error=\"true\"] { color: #cf222e; font-weight: bold; padding-top: 4px; }"
    );
    mainLayout->addWidget(m_statusLabel);

    connect(m_searchEdit, &QLineEdit::textChanged, this, &BoardManagerDialog::applyFilters);
    connect(m_typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &BoardManagerDialog::applyFilters);
}

void BoardManagerDialog::setStatusText(const QString& text, bool isError)
{
    if (!m_statusLabel) return;
    m_statusLabel->setProperty("error", isError);
    if (m_statusLabel->style()) {
        m_statusLabel->style()->unpolish(m_statusLabel);
        m_statusLabel->style()->polish(m_statusLabel);
    }
    m_statusLabel->setText(text);
}

void BoardManagerDialog::applyFilters()
{
    const QString query = m_searchEdit->text().trimmed().toLower();
    const QString mode = m_typeCombo->currentData().toString();

    QVector<CorePackageInfo> filtered;
    for (const auto& core : std::as_const(m_allCores)) {
        if (mode == "installed" && core.installedVersion.isEmpty()) {
            continue;
        }

        if (!query.isEmpty()) {
            const bool matchName = core.name.toLower().contains(query);
            const bool matchId = core.id.toLower().contains(query);
            const bool matchBoards = core.boards.join(' ').toLower().contains(query);
            if (!matchName && !matchId && !matchBoards) {
                continue;
            }
        }

        filtered.append(core);
    }

    rebuildList(filtered);
}

void BoardManagerDialog::rebuildList(const QVector<CorePackageInfo>& items)
{
    if (!m_listLayout) return;

    while (m_listLayout->count() > 1) {
        QLayoutItem* item = m_listLayout->takeAt(0);
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }

    if (items.isEmpty()) {
        auto* emptyLabel = new QLabel(tr("Aucun paquet trouvé."), m_listContainer);
        emptyLabel->setAlignment(Qt::AlignCenter);
        emptyLabel->setStyleSheet("color: #656d76; padding: 20px;");
        m_listLayout->insertWidget(0, emptyLabel);
        return;
    }

    for (int i = 0; i < items.size(); ++i) {
        QFrame* card = createCoreCard(
            items[i],
            m_listContainer,
            m_busy,
            [this](const QString& coreId, const QString& version) {
                onInstallRequested(coreId, version);
            },
            [this](const QString& coreId) {
                onRemoveRequested(coreId);
            }
        );
        m_listLayout->insertWidget(i, card);
    }
}

void BoardManagerDialog::onInstallRequested(const QString& coreId, const QString& version)
{
    if (!m_coreManager || m_busy) return;

    const QString target = version.isEmpty() ? coreId : QString("%1@%2").arg(coreId, version);
    m_busy = true;
    setStatusText(tr("Installation de %1 en cours...").arg(target));
    applyFilters();
    m_coreManager->installCore(target);
}

void BoardManagerDialog::onRemoveRequested(const QString& coreId)
{
    if (!m_coreManager || m_busy) return;

    m_busy = true;
    setStatusText(tr("Suppression de %1 en cours...").arg(coreId));
    applyFilters();
    m_coreManager->uninstallCore(coreId);
}