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

    // Lance la recherche des cartes de manière asynchrone
    void refreshBoards();

signals:
    // Signal émis quand la liste des cartes est prête
    void boardsUpdated(const QList<BoardInfo>& boards);
    // Signal en cas d'erreur
    void errorOccurred(const QString& message);

private:
    QString getCliPath() const;
};