#pragma once

#include <QTextEdit>
#include <QCompleter>

class ArduinoSyntaxHighlighter;
class QKeyEvent;

class CodeEditor : public QTextEdit {
    Q_OBJECT
public:
    explicit CodeEditor(QWidget* parent = nullptr);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void setupCompleter();
    void insertCompletion(const QString& completion);
    QString wordUnderCursor() const;

    ArduinoSyntaxHighlighter* m_highlighter = nullptr;
    QCompleter* m_completer = nullptr;
};