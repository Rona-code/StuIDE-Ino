#include <QCoreApplication>
#include <QApplication>
#include <QIcon>
#include <QFile>
#include <QStandardPaths>
#include <QDebug>
#include <QDir>

#include "commun/AppInitializer.h"
#include "commun/TranslationManager.h"
#include "ui/StuideWindow.h"

int main(int argc, char* argv[])
{
    QCoreApplication::setOrganizationName("Stusoft");
    QCoreApplication::setApplicationName("StuIDE-Ino");

    QApplication a(argc, argv);

	a.setQuitOnLastWindowClosed(false);
	a.setWindowIcon(QIcon(":/resources/Icone.png"));

    // Charge la langue de base
    TranslationManager::instance().loadSavedLanguage();

    if (!AppInitializer::init())
        return 0;

    StuideWindow w;

    w.show();

    return a.exec();
}