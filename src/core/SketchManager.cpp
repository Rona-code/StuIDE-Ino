#include "SketchManager.h"
#include <QFile>
#include <QTextStream>
#include <QStandardPaths>
#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QDate>

SketchManager::SketchManager() {
    m_currentSketchName = generateNewSketchName();
}

QString SketchManager::defaultSketchbookDir() const {
    QString docs = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QDir dir(docs + "/StuIDE-Ino/Sketches");
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    return dir.absolutePath();
}

QString SketchManager::generateNewSketchName() const {
    QDate now = QDate::currentDate();
    return QString("sketch_%1%2a").arg(now.toString("MMM").toLower(), QString::number(now.day()));
}

bool SketchManager::newSketch() {
    m_currentSketchName = generateNewSketchName();
    QString baseDir = defaultSketchbookDir();
    QDir sketchDir(baseDir + "/" + m_currentSketchName);

    if (!sketchDir.exists()) {
        sketchDir.mkpath(".");
    }

    m_currentFilePath = sketchDir.absoluteFilePath(m_currentSketchName + ".ino");

    QFile file(m_currentFilePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << "void setup() {\n  // put your setup code here, to run once:\n\n}\n\n"
            << "void loop() {\n  // put your main code here, to run repeatedly:\n\n}\n";
        file.close();
        return true;
    }
    return false;
}

bool SketchManager::openSketch(QWidget* parent) {
    QString dir = defaultSketchbookDir();
    QString filePath = QFileDialog::getOpenFileName(parent, QObject::tr("Ouvrir un croquis"), dir, QObject::tr("Fichiers Arduino (*.ino)"));

    if (filePath.isEmpty()) return false;

    QString content;
    if (loadFile(filePath, content)) {
        m_currentFilePath = filePath;
        m_currentSketchName = QFileInfo(filePath).completeBaseName();
        return true;
    }
    else {
        QMessageBox::warning(parent, QObject::tr("Erreur"), QObject::tr("Impossible d'ouvrir le fichier sélectionné."));
        return false;
    }
}

bool SketchManager::saveSketch(const QString& content) {
    if (m_currentFilePath.isEmpty()) {
        return false;
    }

    QFile file(m_currentFilePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << content;
        file.close();
        return true;
    }
    return false;
}

bool SketchManager::saveAsSketch(QWidget* parent, const QString& content) {
    QString dir = defaultSketchbookDir();
    QString defaultPath = dir + "/" + m_currentSketchName + "/" + m_currentSketchName + ".ino";

    QString filePath = QFileDialog::getSaveFileName(
        parent,
        QObject::tr("Enregistrer le croquis sous..."),
        defaultPath,
        QObject::tr("Fichiers Arduino (*.ino)")
    );

    if (filePath.isEmpty()) return false;

    QFileInfo fileInfo(filePath);
    QDir targetDir = fileInfo.dir();
    if (!targetDir.exists()) {
        targetDir.mkpath(".");
    }

    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << content;
        file.close();

        m_currentFilePath = filePath;
        m_currentSketchName = fileInfo.completeBaseName();
        return true;
    }
    return false;
}

bool SketchManager::loadFile(const QString& filePath, QString& outContent) {
    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        outContent = in.readAll();
        file.close();
        return true;
    }
    return false;
}