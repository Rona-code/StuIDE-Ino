#pragma once

#include "ArduinoSyntaxHighlighter.h"
#include "../core/sketch/SketchManager.h"
#include "../core/cli/ArduinoCliDownloader.h"
#include "../core/cli/ArduinoCliManager.h"
#include <QMainWindow>
#include <QSplitter>
#include <QTextEdit>
#include <QToolBar>
#include <QComboBox>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QString>
#include <QEvent>
#include <QLabel>

class StuideWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit StuideWindow(QWidget* parent = nullptr);
	~StuideWindow() = default;

protected:
	void changeEvent(QEvent* event) override;
	void closeEvent(QCloseEvent* event) override;

private slots:
	void onLanguageChanged(int index);

	void onNewSketch();
	void onOpenSketch();
	void onSaveSketch();
	void onSaveAsSketch();

private:
	void setupUi();
	void setupMenuBar();
	void setupToolBar();
	void setupCentralWidget();
	void setupStatusBar();
	void retranslateUi();
	void updateWindowTitle();
    void initArduinoCli();
	void initArduinoCliConnections();
	void loadInitialSketch();
	void onAllBoardsLoaded(const QVector<KnownBoardInfo>& boards);
	void onBoardsUpdated(const QList<BoardInfo>& boards);
	void onCliErrorOccurred(const QString& errorMsg);
	void compileCurrentSketch();
	void uploadCurrentSketch();

    QString currentTargetFqbn() const;

    // Éléments de l'interface
    QSplitter* m_splitter = nullptr;
    QTextEdit* m_editor = nullptr;
    QTextEdit* m_console = nullptr;
    QToolBar* m_toolbar = nullptr;
    QComboBox* m_boardComboBox = nullptr;
    QComboBox* m_langComboBox = nullptr;
    QComboBox* m_statusBoardCombo = nullptr;
	QLabel* m_statusInfoLabel = nullptr;

    // Menus
    QMenu* m_menuFile = nullptr;
    QMenu* m_menuEdit = nullptr;
    QMenu* m_menuSketch = nullptr;
    QMenu* m_menuTools = nullptr;
    QMenu* m_menuHelp = nullptr;

    // Actions traductibles
    QAction* m_actionNew = nullptr;
    QAction* m_actionOpen = nullptr;
    QAction* m_actionSave = nullptr;
	QAction* m_actionSaveAs = nullptr;
    QAction* m_actionUndo = nullptr;
    QAction* m_actionRedo = nullptr;
    QAction* m_actionVerify = nullptr;
    QAction* m_actionUpload = nullptr;
    QAction* m_actionBoardMgr = nullptr;
    QAction* m_actionLibMgr = nullptr;
    QAction* m_actionSerial = nullptr;
    QAction* m_actionAbout = nullptr;

	ArduinoCliDownloader* m_cliDownloader = nullptr;
	ArduinoCliManager m_cliManager;
    SketchManager m_sketchManager;

    QString m_currentFilePath;
};