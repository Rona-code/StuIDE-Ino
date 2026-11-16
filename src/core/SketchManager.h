#pragma once

#include <QString>
#include <QDir>
#include <QWidget>

class SketchManager {
public:
    SketchManager();

    bool newSketch();
    bool openSketch(QWidget* parent);
    bool saveSketch(const QString& content);
    bool saveAsSketch(QWidget* parent, const QString& content);

    QString currentFilePath() const { return m_currentFilePath; }
    QString currentSketchName() const { return m_currentSketchName; }
    QString defaultSketchbookDir() const;

    QString generateNewSketchName() const;

private:
    QString m_currentFilePath;
    QString m_currentSketchName;

    bool loadFile(const QString& filePath, QString& outContent);
};