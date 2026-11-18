#pragma once

#include <QString>
#include <QObject>
#include <QList>

struct BoardInfo {
    QString name;   // Nom commercial (ex: "Arduino Uno")
    QString fqbn;   // Identifiant de compilation (ex: "arduino:avr:uno")
    QString port;   // Port physique (ex: "COM3" ou "/dev/ttyACM0")
};

class ArduinoCliManager : public QObject {
    Q_OBJECT
public:
    explicit ArduinoCliManager(QObject* parent = nullptr);

	void compileSketch(const QString& sketchPath, const QString& fqbn);
    void refreshBoards();

signals:
    void boardsUpdated(const QList<BoardInfo>& boards);
    void errorOccurred(const QString& message);

	void compilationOutput(const QString& text);
	void compilationError(const QString& text);
	void compilationFinished(bool success, const QString& message);

private:
    QString getCliPath() const;
};