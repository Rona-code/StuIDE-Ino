#include "StuideWindow.h"
#include "../commun/TranslationManager.h"
#include <QDate>
#include <QFont>
#include <QSizePolicy>
#include <QWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>

StuideWindow::StuideWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setupUi();
 
    if (m_sketchManager.newSketch()) {
        QFile file(m_sketchManager.currentFilePath());
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            m_editor->setPlainText(file.readAll());
            file.close();
        }
    }

	resize(600, 600);
    updateWindowTitle();
}

void StuideWindow::setupUi()
{
    setupMenuBar();
    setupToolBar();
    setupCentralWidget();
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
    m_actionVerify = m_menuSketch->addAction("✓");
    m_actionVerify->setToolTip(tr("Vérifier"));
    m_actionUpload = m_menuSketch->addAction("➔");
    m_actionUpload->setToolTip(tr("Téléverser"));

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

    m_editor = new QTextEdit(this);
    m_editor->setFont(QFont("Consolas", 11));

    m_console = new QTextEdit(this);
    m_console->setReadOnly(true);
    m_console->setStyleSheet("background-color: #1e1e1e; color: #d4d4d4; font-family: Consolas;");

    m_splitter->addWidget(m_editor);
    m_splitter->addWidget(m_console);
    m_splitter->setSizes({ 550, 200 });

    setCentralWidget(m_splitter);
}

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
    m_actionVerify->setToolTip(tr("Vérifier"));
    m_actionUpload->setToolTip(tr("Téléverser"));

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

void StuideWindow::onNewSketch() {
    if (m_sketchManager.newSketch()) {
        updateWindowTitle();
        m_editor->clear();
        m_console->append(tr("Nouveau croquis créé."));
    }
}

void StuideWindow::onOpenSketch() {
    if (m_sketchManager.openSketch(this)) {
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

void StuideWindow::updateWindowTitle()
{
    // Utilise directement le nom géré par le SketchManager
    setWindowTitle(QString("%1 | StuIDE-Ino %2").arg(m_sketchManager.currentSketchName(), QString(APP_VERSION)));
}