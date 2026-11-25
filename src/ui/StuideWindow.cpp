#include "StuideWindow.h"
#include "BoardManagerDialog.h"
#include "SerialMonitorWindow.h"
#include "LibraryManagerDialog.h"
#include "../commun/TranslationManager.h"
#include "../core/cli/CoreManager.h"
#include "../core/cli/ArduinoCliDownloader.h"
#include "../core/cli/ArduinoCliManager.h"
#include "../core/cli/LibraryManager.h"
#include "../core/sketch/CodeEditor.h"
#include <QDate>
#include <QFont>
#include <QSizePolicy>
#include <QWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QScrollBar>
#include <QAbstractItemView>
#include <QComboBox>
#include <QLabel>
#include <QSignalBlocker>
#include <QStatusBar>
#include <QDesktopServices>
#include <QDir>
#include <QUrl>

StuideWindow::StuideWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setupUi();
    initArduinoCliConnections();
    loadInitialSketch();

    resize(600, 600);
    updateWindowTitle();
}

/// --- Gestion de l'interface utilisateur ---
void StuideWindow::setupUi()
{
    setupMenuBar();
    setupToolBar();
    setupCentralWidget();
    setupStatusBar();
    retranslateUi();
}

void StuideWindow::setupMenuBar()
{
    QMenuBar* bar = menuBar();

    m_menuFile = bar->addMenu(tr("Fichier"));
    m_actionNew = m_menuFile->addAction(tr("Nouveau"));
    m_actionOpen = m_menuFile->addAction(tr("Ouvrir..."));
    m_actionSave = m_menuFile->addAction(tr("Enregistrer"));
    m_actionSaveAs = m_menuFile->addAction(tr("Enregistrer sous..."));

    m_menuEdit = bar->addMenu(tr("Édition"));
    m_actionUndo = m_menuEdit->addAction(tr("Annuler"));
    m_actionRedo = m_menuEdit->addAction(tr("Rétablir"));

    m_menuSketch = bar->addMenu(tr("Croquis"));
    m_actionShowSketchFolder = m_menuSketch->addAction(tr("Afficher le dossier des croquis"));
    m_actionShowSketchFolder->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_K));
    m_actionExportBinaries = m_menuSketch->addAction(tr("Exporter les binaires compilés"));
    m_actionExportBinaries->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_S));

    m_menuTools = bar->addMenu(tr("Outils"));
    m_actionBoardMgr = m_menuTools->addAction(tr("Gestionnaire de cartes..."));
    m_actionLibMgr = m_menuTools->addAction(tr("Gestionnaire de bibliothèques..."));
    m_menuTools->addSeparator();
    m_actionSerial = m_menuTools->addAction(tr("Port série"));

    m_menuHelp = bar->addMenu(tr("Aide"));
    m_actionAbout = m_menuHelp->addAction(tr("À propos de StuIDE-Ino"));

    connect(m_actionNew, &QAction::triggered, this, &StuideWindow::onNewSketch);
    connect(m_actionOpen, &QAction::triggered, this, &StuideWindow::onOpenSketch);
    connect(m_actionSave, &QAction::triggered, this, &StuideWindow::onSaveSketch);
    connect(m_actionSaveAs, &QAction::triggered, this, &StuideWindow::onSaveAsSketch);
	connect(m_actionVerify, &QAction::triggered, this, &StuideWindow::compileCurrentSketch);
    connect(m_actionUpload, &QAction::triggered, this, &StuideWindow::uploadCurrentSketch);
    connect(m_actionShowSketchFolder, &QAction::triggered, this, &StuideWindow::openSketchFolder);
    connect(m_actionExportBinaries, &QAction::triggered, this, &StuideWindow::exportCompiledBinaries);
    connect(m_actionBoardMgr, &QAction::triggered, this, [this]() {
		QString cliPath = m_cliDownloader ? m_cliDownloader->getLocalCliPath() : m_cliManager.getCliPath();
		auto* coreMgr = new CoreManager(cliPath, this);
		auto* dlg = new BoardManagerDialog(coreMgr, this);
		dlg->setAttribute(Qt::WA_DeleteOnClose);
        coreMgr->setParent(dlg);

        connect(dlg, &BoardManagerDialog::coresChanged, this, [this]() {
            m_cliManager.refreshBoards();
            m_cliManager.fetchAllBoards();
        });

		dlg->exec();
    });
    connect(m_actionLibMgr, &QAction::triggered, this, [this]() {
        const QString cliPath = m_cliDownloader ? m_cliDownloader->getLocalCliPath() : m_cliManager.getCliPath();
        LibraryManager libMgr(cliPath);
        LibraryManagerDialog dlg(&libMgr, this);
        dlg.exec();
        });
}

void StuideWindow::setupToolBar()
{
    m_toolbar = addToolBar("MainToolBar");
    m_toolbar->setMovable(false);
    m_toolbar->setIconSize(QSize(20, 20));

    m_toolbar->addAction(m_actionVerify);
    m_toolbar->addAction(m_actionUpload);

    m_toolbar->addSeparator();

    m_boardComboBox = new QComboBox(this);
    m_boardComboBox->setMinimumWidth(220);
    m_boardComboBox->setPlaceholderText(tr("Aucune carte détectée"));
    m_toolbar->addWidget(m_boardComboBox);

    QWidget* spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_toolbar->addWidget(spacer);

    m_langComboBox = new QComboBox(this);
    m_langComboBox->addItem("FR", "fr");
    m_langComboBox->addItem("EN", "en");
    m_langComboBox->addItem("JP", "ja");
    m_langComboBox->setFixedWidth(75);

    QString currentLang = TranslationManager::instance().currentLanguage();
    int langIndex = m_langComboBox->findData(currentLang);
    if (langIndex != -1) {
        m_langComboBox->setCurrentIndex(langIndex);
    }

    connect(m_langComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, &StuideWindow::onLanguageChanged);

    m_toolbar->addWidget(m_langComboBox);
}

void StuideWindow::setupCentralWidget()
{
    m_splitter = new QSplitter(Qt::Vertical, this);

	m_editor = new CodeEditor(this);

	connect(m_editor, &QTextEdit::textChanged, this, [this]() {
		m_cliManager.resetCompileSuccess();
	});

    m_console = new QTextEdit(this);
    m_console->setReadOnly(true);
    m_console->setStyleSheet("background-color: #1e1e1e; color: #d4d4d4; font-family: Consolas;");

    m_splitter->addWidget(m_editor);
    m_splitter->addWidget(m_console);
    m_splitter->setSizes({ 550, 200 });

    setCentralWidget(m_splitter);
}

void StuideWindow::setupStatusBar()
{
    QStatusBar* bar = statusBar();

    m_statusInfoLabel = new QLabel(tr("Prêt"), this);
    bar->addWidget(m_statusInfoLabel, 1);

    m_statusBoardCombo = new QComboBox(this);
    m_statusBoardCombo->setEditable(true);
    m_statusBoardCombo->setInsertPolicy(QComboBox::NoInsert);
    m_statusBoardCombo->setMinimumWidth(240);
    m_statusBoardCombo->setPlaceholderText(tr("Sélectionner ou saisir un FQBN..."));
    m_statusBoardCombo->setToolTip(
        tr("Carte cible (FQBN) — cliquez pour choisir une carte ou saisissez un FQBN (ex. esp32:esp32:esp32)")
    );
    m_statusBoardCombo->setStyleSheet(
        "QComboBox {"
        "  border: none;"
        "  background: transparent;"
        "  padding: 2px 6px;"
        "  font-weight: bold;"
        "}"
        "QComboBox:hover {"
        "  background-color: rgba(0, 0, 0, 0.06);"
        "  border-radius: 3px;"
        "}"
    );

    bar->addPermanentWidget(m_statusBoardCombo);

    if (m_boardComboBox) {
        connect(m_boardComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
                if (index < 0 || !m_boardComboBox || !m_statusBoardCombo) {
                    return;
                }

                const QVariant data = m_boardComboBox->itemData(index);
                QString detectedFqbn;
                QString portLabel;

                if (data.canConvert<QStringList>()) {
                    const QStringList list = data.toStringList();
                    if (!list.isEmpty()) {
                        detectedFqbn = list.at(0).trimmed();
                    }
                    if (list.size() >= 2) {
                        portLabel = list.at(1).trimmed();
                    }
                }

                if (!detectedFqbn.isEmpty()) {
                    const int foundIdx = m_statusBoardCombo->findData(detectedFqbn);
                    if (foundIdx != -1) {
                        m_statusBoardCombo->setCurrentIndex(foundIdx);
                    }
                    else {
                        m_statusBoardCombo->setCurrentText(detectedFqbn);
                    }
                    if (m_statusInfoLabel) {
                        m_statusInfoLabel->setText(
                            portLabel.isEmpty() ? tr("Carte détectée") : tr("Port : %1").arg(portLabel)
                        );
                    }
                }
                else if (!portLabel.isEmpty()) {
                    if (m_statusInfoLabel) {
                        m_statusInfoLabel->setText(tr("Port %1 — choisissez la carte (FQBN) à droite").arg(portLabel));
                    }
                }
            });
    }
}

static QString formatReadableBoardLabel(const KnownBoardInfo& board)
{
    const QString name = board.name.trimmed();
    const QString fqbn = board.fqbn.trimmed();

    if (name.isEmpty()) {
        return fqbn;
    }

    // Exemple de simplification si on veut un libellé court pour l'ESP32
    if (name.compare(QStringLiteral("ESP32"), Qt::CaseInsensitive) == 0
        || fqbn.contains(QStringLiteral(":esp32:"), Qt::CaseInsensitive) && name.startsWith(QStringLiteral("ESP32"), Qt::CaseInsensitive)) {
        // Garde un libellé lisible sans rallonge excessive
        if (name.size() > 28) {
            return name.left(25).trimmed() + QStringLiteral("...");
        }
    }

    if (name.size() > 28) {
        return name.left(25).trimmed() + QStringLiteral("...");
    }

    return name;
}

void StuideWindow::onAllBoardsLoaded(const QVector<KnownBoardInfo>& boards)
{
    if (!m_statusBoardCombo || boards.isEmpty()) {
        return;
    }

    const QString previousFqbn = currentTargetFqbn();

    QSignalBlocker blocker(m_statusBoardCombo);
    m_statusBoardCombo->clear();

    for (const KnownBoardInfo& board : boards) {
        if (board.fqbn.isEmpty()) {
            continue;
        }

        const QString label = formatReadableBoardLabel(board);
        const int idx = m_statusBoardCombo->count();
        m_statusBoardCombo->addItem(label, board.fqbn);

        const QString tip = board.name.isEmpty()
            ? board.fqbn
            : QStringLiteral("%1\nFQBN : %2").arg(board.name, board.fqbn);
        m_statusBoardCombo->setItemData(idx, tip, Qt::ToolTipRole);
    }

    if (!previousFqbn.isEmpty()) {
        const int idx = m_statusBoardCombo->findData(previousFqbn);
        if (idx != -1) {
            m_statusBoardCombo->setCurrentIndex(idx);
        }
        else {
            m_statusBoardCombo->setCurrentText(previousFqbn);
        }
    }
}

QString StuideWindow::currentTargetFqbn() const
{
    if (!m_statusBoardCombo) {
        return {};
    }

    const int idx = m_statusBoardCombo->currentIndex();
    const QString text = m_statusBoardCombo->currentText().trimmed();

    if (!text.contains(QLatin1Char(' ')) && text.count(QLatin1Char(':')) >= 2) {
        return text;
    }

    if (idx != -1 && text == m_statusBoardCombo->itemText(idx)) {
        const QString dataFqbn = m_statusBoardCombo->itemData(idx).toString().trimmed();
        if (!dataFqbn.isEmpty()) {
            return dataFqbn;
        }
    }

    return text;
}

void StuideWindow::updateWindowTitle()
{
    setWindowTitle(QString("%1 | StuIDE-Ino %2").arg(m_sketchManager.currentSketchName(), QString(APP_VERSION)));
}

void StuideWindow::closeEvent(QCloseEvent* event)
{
    event->accept();
    QCoreApplication::quit();
}

// --- Traduction et gestion de langue ---
void StuideWindow::retranslateUi()
{
    m_menuFile->setTitle(tr("Fichier"));
    m_actionNew->setText(tr("Nouveau"));
    m_actionOpen->setText(tr("Ouvrir..."));
    m_actionSave->setText(tr("Enregistrer"));
    m_actionSaveAs->setText(tr("Enregistrer sous..."));

    m_menuEdit->setTitle(tr("Édition"));
    m_actionUndo->setText(tr("Annuler"));
    m_actionRedo->setText(tr("Rétablir"));

    m_menuSketch->setTitle(tr("Croquis"));
    if (m_actionShowSketchFolder) {
        m_actionShowSketchFolder->setText(tr("Afficher le dossier des croquis"));
    }
    if (m_actionExportBinaries) {
        m_actionExportBinaries->setText(tr("Exporter les binaires compilés"));
    }

    m_menuTools->setTitle(tr("Outils"));
    m_actionBoardMgr->setText(tr("Gestionnaire de cartes..."));
    m_actionLibMgr->setText(tr("Gestionnaire de bibliothèques..."));
    m_actionSerial->setText(tr("Port série"));

    m_menuHelp->setTitle(tr("Aide"));
    m_actionAbout->setText(tr("À propos de StuIDE-Ino"));

    m_editor->setPlaceholderText(tr("// Écrivez votre code Arduino ici..."));
    m_console->setPlaceholderText(tr("Console (logs de compilation)..."));
    if (m_boardComboBox->count() == 0) {
        m_boardComboBox->setPlaceholderText(tr("Aucune carte détectée"));
    }
}

void StuideWindow::changeEvent(QEvent* event)
{
    QMainWindow::changeEvent(event);
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
}

void StuideWindow::onLanguageChanged(int index)
{
    QString langCode = m_langComboBox->itemData(index).toString();
    TranslationManager::instance().setLanguage(langCode);
}

// --- Gestion des croquis ---
void StuideWindow::onNewSketch() {
    if (m_sketchManager.newSketch()) {
		m_cliManager.resetCompileSuccess();
        updateWindowTitle();
        m_editor->clear();
        m_console->append(tr("Nouveau croquis créé."));
    }
}

void StuideWindow::onOpenSketch() {
    if (m_sketchManager.openSketch(this)) {
		m_cliManager.resetCompileSuccess();
        updateWindowTitle();

        QFile file(m_sketchManager.currentFilePath());
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            m_editor->setPlainText(file.readAll());
            file.close();
        }
        m_console->append(tr("Croquis ouvert : ") + m_sketchManager.currentFilePath());
    }
}

void StuideWindow::onSaveSketch() {
    if (m_sketchManager.currentFilePath().isEmpty()) {
        onSaveAsSketch();
        return;
    }

    if (m_sketchManager.saveSketch(m_editor->toPlainText())) {
        m_console->append(tr("Croquis enregistré avec succès : ") + m_sketchManager.currentFilePath());
    }
    else {
        QMessageBox::critical(this, tr("Erreur"), tr("Échec lors de l'enregistrement du fichier."));
    }
}

void StuideWindow::onSaveAsSketch() {
    if (m_sketchManager.saveAsSketch(this, m_editor->toPlainText())) {
        updateWindowTitle();
        m_console->append(tr("Croquis enregistré sous : ") + m_sketchManager.currentFilePath());
    }
}

void StuideWindow::openSketchFolder()
{
    const QString filePath = m_sketchManager.currentFilePath();
    QString folderPath;

    if (!filePath.isEmpty()) {
        const QFileInfo info(filePath);
        folderPath = info.exists() ? info.absolutePath() : info.absoluteDir().absolutePath();
    }

    if (folderPath.isEmpty() || !QDir(folderPath).exists()) {
        m_console->append(tr("Erreur : aucun dossier de croquis valide trouvé."));
        return;
    }

    QDesktopServices::openUrl(QUrl::fromLocalFile(folderPath));
}

void StuideWindow::exportCompiledBinaries()
{
    m_console->clear();

    const QString currentContent = m_editor->toPlainText();
    if (!m_sketchManager.saveSketch(currentContent)) {
        if (!m_sketchManager.saveAsSketch(this, currentContent)) {
            return;
        }
    }

    const QString sketchPath = m_sketchManager.currentFilePath();
    const QString fqbn = currentTargetFqbn();

    if (fqbn.isEmpty()) {
        m_console->append(tr("Erreur : aucun FQBN défini. Sélectionnez ou saisissez une carte dans la barre d'état en bas."));
        if (m_statusBoardCombo) {
            m_statusBoardCombo->setFocus();
            m_statusBoardCombo->showPopup();
        }
        return;
    }

    m_console->append(tr("Compilation et export des binaires en cours pour %1...").arg(fqbn));
    m_cliManager.compileSketch(sketchPath, fqbn, true);
}

// --- Gestion de l'Arduino CLI ---
void StuideWindow::loadInitialSketch()
{
    if (m_sketchManager.newSketch()) {
        QFile file(m_sketchManager.currentFilePath());
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            m_editor->setPlainText(file.readAll());
            file.close();
        }
    }
}

void StuideWindow::initArduinoCli() {
    m_cliDownloader = new ArduinoCliDownloader(this);

    connect(m_cliDownloader, &ArduinoCliDownloader::downloadFinished, this, [this](bool success, const QString& message) {
        m_console->append(message);
        if (success) {

            QString cliPath = m_cliDownloader->getLocalCliPath();

            CoreManager* coreManager = new CoreManager(cliPath, this);

            m_console->append(tr("Installation automatique du cœur AVR en arrière-plan..."));

            connect(coreManager, &CoreManager::coreInstallFinished, this, [this, coreManager](bool ok, const QString& msg) {
                m_console->append(msg);
                if (ok) {
                    m_cliManager.refreshBoards();
                    m_cliManager.fetchAllBoards();
                }
                coreManager->deleteLater();
            });

            coreManager->installCore("arduino:avr");
        }
        });

    connect(m_actionSerial, &QAction::triggered, this, [this]() {
        if (!m_serialMonitor) {
            m_serialMonitor = new SerialMonitorWindow(this);
            m_serialMonitor->setWindowFlag(Qt::Window, true);
            m_serialMonitor->setAttribute(Qt::WA_DeleteOnClose, false);
        }

        if (m_boardComboBox && m_boardComboBox->currentIndex() != -1) {
            const QVariant data = m_boardComboBox->currentData();
            if (data.canConvert<QStringList>()) {
                const QStringList list = data.toStringList();
                if (list.size() >= 2) {
                    m_serialMonitor->setPreferredPort(list.at(1).trimmed());
                }
            }
        }

        m_serialMonitor->show();
        m_serialMonitor->raise();
        m_serialMonitor->activateWindow();
        });

    m_cliDownloader->ensureCliAvailable();

    const QString localCli = m_cliDownloader->getLocalCliPath();
    if (!localCli.isEmpty() && QFileInfo::exists(localCli)) {
        m_cliManager.fetchAllBoards();
    }
}

void StuideWindow::initArduinoCliConnections() {
    connect(&m_cliManager, &ArduinoCliManager::boardsUpdated, this, &StuideWindow::onBoardsUpdated);
    connect(&m_cliManager, &ArduinoCliManager::errorOccurred, this, &StuideWindow::onCliErrorOccurred);

	connect(&m_cliManager, &ArduinoCliManager::compilationOutput, this, [this](const QString& text) {
		m_console->insertPlainText(text);
		m_console->verticalScrollBar()->setValue(m_console->verticalScrollBar()->maximum());
	});

	connect(&m_cliManager, &ArduinoCliManager::compilationError, this, [this](const QString& text) {
		m_console->insertPlainText(text);
		m_console->verticalScrollBar()->setValue(m_console->verticalScrollBar()->maximum());
	});

	connect(&m_cliManager, &ArduinoCliManager::compilationFinished, this, [this](bool success, const QString& message) {
		m_console->append("\n" + message);
	});

    connect(&m_cliManager, &ArduinoCliManager::allBoardsLoaded, this, &StuideWindow::onAllBoardsLoaded);

    initArduinoCli();
}

void StuideWindow::onBoardsUpdated(const QList<BoardInfo>& boards) {
    m_boardComboBox->clear();

    if (boards.isEmpty()) {
        m_boardComboBox->addItem(tr("Aucune carte détectée"));
        return;
    }

    for (const auto& board : boards) {
        QString label = QString("%1 (%2)").arg(board.name, board.port);
        m_boardComboBox->addItem(label, QVariant::fromValue(QStringList{ board.fqbn, board.port }));
    }
}

void StuideWindow::compileCurrentSketch()
{
    m_console->clear();

    const QString currentContent = m_editor->toPlainText();
    if (!m_sketchManager.saveSketch(currentContent)) {
        if (!m_sketchManager.saveAsSketch(this, currentContent)) {
            return;
        }
    }

    const QString sketchPath = m_sketchManager.currentFilePath();
    const QString fqbn = currentTargetFqbn();

    if (fqbn.isEmpty()) {
        m_console->append(tr("Erreur : aucun FQBN défini. Sélectionnez ou saisissez une carte dans la barre d'état en bas."));
        if (m_statusBoardCombo) {
            m_statusBoardCombo->setFocus();
            m_statusBoardCombo->showPopup();
        }
        return;
    }

    m_console->append(tr("Compilation en cours pour %1...").arg(fqbn));
    m_cliManager.compileSketch(sketchPath, fqbn);
}

void StuideWindow::uploadCurrentSketch()
{
    m_console->clear();

    if (!m_cliManager.isCompileSuccess()) {
        m_console->append(tr("Veuillez compiler le croquis avec succès avant de téléverser."));
        return;
    }

    const QString currentContent = m_editor->toPlainText();
    if (!m_sketchManager.saveSketch(currentContent)) {
        if (!m_sketchManager.saveAsSketch(this, currentContent)) {
            return;
        }
    }

    const QString sketchPath = m_sketchManager.currentFilePath();
    const QString fqbn = currentTargetFqbn();
    QString port;

    if (m_boardComboBox && m_boardComboBox->count() > 0 && m_boardComboBox->currentIndex() != -1) {
        const QVariant data = m_boardComboBox->currentData();
        if (data.canConvert<QStringList>()) {
            const QStringList list = data.toStringList();
            if (list.size() >= 2) {
                port = list.at(1).trimmed();
            }
        }
    }

    if (fqbn.isEmpty()) {
        m_console->append(tr("Erreur : aucun FQBN défini. Sélectionnez ou saisissez une carte dans la barre d'état en bas."));
        return;
    }

    if (port.isEmpty()) {
        QMessageBox::warning(this, tr("Erreur"), tr("Aucun port série sélectionné."));
        return;
    }

    m_console->append(tr("Téléversement en cours pour %1 (port %2)...").arg(fqbn, port));
    m_cliManager.uploadSketch(sketchPath, fqbn, port);
}

void StuideWindow::onCliErrorOccurred(const QString& errorMsg) {
    m_console->append(tr("Erreur Arduino-CLI : ") + errorMsg);
}