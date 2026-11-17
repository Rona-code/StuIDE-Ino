#pragma once

#include "../core/SketchManager.h"
#include "../core/ArduinoCliDownloader.h"
#include "../core/ArduinoCliManager.h"
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

class StuideWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit StuideWindow(QWidget* parent = nullptr);
	~StuideWindow() = default;

protected:
	void changeEvent(QEvent* event) override;

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
	void retranslateUi();
	void updateWindowTitle();
    void initArduinoCli();

    // Éléments de l'interface
    QSplitter* m_splitter;
    QTextEdit* m_editor;
    QTextEdit* m_console;
    QToolBar* m_toolbar;
    QComboBox* m_boardComboBox;
    QComboBox* m_langComboBox;

    // Menus
    QMenu* m_menuFile;
    QMenu* m_menuEdit;
    QMenu* m_menuSketch;
    QMenu* m_menuTools;
    QMenu* m_menuHelp;

    // Actions traductibles
    QAction* m_actionNew;
    QAction* m_actionOpen;
    QAction* m_actionSave;
	QAction* m_actionSaveAs;
    QAction* m_actionUndo;
    QAction* m_actionRedo;
    QAction* m_actionVerify;
    QAction* m_actionUpload;
    QAction* m_actionBoardMgr;
    QAction* m_actionLibMgr;
    QAction* m_actionSerial;
    QAction* m_actionAbout;

	ArduinoCliDownloader* m_cliDownloader;
	ArduinoCliManager m_cliManager;
    SketchManager m_sketchManager;

    QString m_currentFilePath;
};