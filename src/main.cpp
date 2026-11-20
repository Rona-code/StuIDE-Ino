#include <QCoreApplication>
#include <QApplication>
#include <QIcon>

#include "commun/AppInitializer.h"
#include "commun/TranslationManager.h"
#include "ui/StuideWindow.h"

int main(int argc, char* argv[])
{
    QCoreApplication::setOrganizationName("Stusoft");
    QCoreApplication::setApplicationName("StuIDE-Ino");

    QApplication app(argc, argv);
	app.setWindowIcon(QIcon(":/resources/Icone.png"));

    // Charge la langue de base
    TranslationManager::instance().loadSavedLanguage();

    if (!AppInitializer::init())
        return 0;

    StuideWindow mainWindow;
    mainWindow.show();

    return app.exec();
}