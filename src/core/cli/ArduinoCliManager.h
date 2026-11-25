#pragma once

#include <QString>
#include <QObject>
#include <QList>
#include <QVector>

struct BoardInfo {
    QString name;   // Nom commercial (ex: "Arduino Uno")
    QString fqbn;   // Identifiant de compilation (ex: "arduino:avr:uno")
    QString port;   // Port physique (ex: "COM3" ou "/dev/ttyACM0")
};

struct KnownBoardInfo {
    QString name;
    QString fqbn;
};

class ArduinoCliManager : public QObject {
    Q_OBJECT
public:
    explicit ArduinoCliManager(QObject* parent = nullptr);

    ~ArduinoCliManager() override = default;

	void compileSketch(const QString& sketchPath, const QString& fqbn, bool exportBinaries = false);
	void uploadSketch(const QString& sketchPath, const QString& fqbn, const QString& port);
    void refreshBoards();
    void fetchAllBoards();

	void resetCompileSuccess() { m_lastCompileSuccess = false; }
	bool isCompileSuccess() const { return m_lastCompileSuccess; }
    
	QString getCliPath() const;

signals:
    void boardsUpdated(const QList<BoardInfo>& boards);
    void errorOccurred(const QString& message);

	void compilationOutput(const QString& text);
	void compilationError(const QString& text);
	void compilationFinished(bool success, const QString& message);

	void allBoardsLoaded(const QVector<KnownBoardInfo>& boards);

private:
    bool m_lastCompileSuccess = false;
};