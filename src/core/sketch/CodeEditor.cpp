#include "CodeEditor.h"
#include "../../ui/ArduinoSyntaxHighlighter.h"
#include <QAbstractItemView>
#include <QFont>
#include <QFontMetrics>
#include <QKeyEvent>
#include <QModelIndex>
#include <QScrollBar>
#include <QStringListModel>
#include <QTextBlock>
#include <QTextCursor>

CodeEditor::CodeEditor(QWidget* parent)
    : QTextEdit(parent)
{
    setFont(QFont("Consolas", 11));
    setStyleSheet("background-color: #1e1e1e; color: #d4d4d4; selection-background-color: #264f78;");
    installEventFilter(this);

    QFontMetrics fm(font());
    setTabStopDistance(fm.horizontalAdvance("    "));

    m_highlighter = new ArduinoSyntaxHighlighter(document());
    setupCompleter();
}

void CodeEditor::setupCompleter()
{
    const QStringList completions = {
        "void setup()", "void loop()", "pinMode(", "digitalWrite(", "digitalRead(",
        "analogRead(", "analogWrite(", "delay(", "delayMicroseconds(", "millis()",
        "micros()", "Serial.begin(9600);", "Serial.print(", "Serial.println(",
        "HIGH", "LOW", "INPUT", "OUTPUT", "INPUT_PULLUP", "LED_BUILTIN",
        "int", "float", "bool", "String", "const int", "for", "if", "else", "while"
    };

    m_completer = new QCompleter(completions, this);
    m_completer->setWidget(this);
    m_completer->popup()->installEventFilter(this);
    m_completer->setCompletionMode(QCompleter::PopupCompletion);
    m_completer->setCaseSensitivity(Qt::CaseInsensitive);

    connect(m_completer, QOverload<const QString&>::of(&QCompleter::activated),
        this, &CodeEditor::insertCompletion);

    connect(this, &QTextEdit::textChanged, this, [this]() {
        if (!m_completer) return;

        QString prefix = wordUnderCursor();
        if (prefix.size() < 2) {
            m_completer->popup()->hide();
            return;
        }

        if (prefix != m_completer->completionPrefix()) {
            m_completer->setCompletionPrefix(prefix);
            m_completer->popup()->setCurrentIndex(m_completer->completionModel()->index(0, 0));
        }

        QRect cr = cursorRect();
        cr.setWidth(m_completer->popup()->sizeHintForColumn(0)
            + m_completer->popup()->verticalScrollBar()->sizeHint().width());
        m_completer->complete(cr);
        });
}

QString CodeEditor::wordUnderCursor() const
{
    QTextCursor tc = textCursor();
    tc.select(QTextCursor::WordUnderCursor);
    return tc.selectedText();
}

void CodeEditor::insertCompletion(const QString& completion)
{
    QTextCursor tc = textCursor();
    QString prefix = m_completer->completionPrefix();
    int extra = completion.length() - prefix.length();

    tc.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, prefix.length());
    tc.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, prefix.length());
    tc.removeSelectedText();
    tc.insertText(completion.left(prefix.length() + (extra > 0 ? extra : 0)));
    setTextCursor(tc);
}

bool CodeEditor::eventFilter(QObject* watched, QEvent* event)
{
    bool isPopup = (m_completer && watched == m_completer->popup());

    if ((watched == this || isPopup) && event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);

        if (m_completer && m_completer->popup()->isVisible()) {
            if (keyEvent->key() == Qt::Key_Escape) {
                m_completer->popup()->hide();
                setFocus();
                return true;
            }
            if (keyEvent->key() == Qt::Key_Return ||
                keyEvent->key() == Qt::Key_Enter ||
                keyEvent->key() == Qt::Key_Tab) {

                QModelIndex idx = m_completer->popup()->currentIndex();
                QString chosen = idx.isValid()
                    ? idx.data(m_completer->completionRole()).toString()
                    : m_completer->currentCompletion();

                insertCompletion(chosen);
                m_completer->popup()->hide();
                setFocus();
                return true;
            }
        }

        if (watched == this) {
            if (keyEvent->key() == Qt::Key_Tab) {
                textCursor().insertText("    ");
                return true;
            }

            if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
                QTextCursor cursor = textCursor();
                QString currentLine = cursor.block().text();

                QString indent;
                for (const QChar& ch : currentLine) {
                    if (ch == ' ' || ch == '\t') {
                        indent.append(ch);
                    }
                    else {
                        break;
                    }
                }

                bool opensBrace = currentLine.trimmed().endsWith('{');

                cursor.insertBlock();
                cursor.insertText(indent);
                if (opensBrace) {
                    cursor.insertText("    ");
                }
                setTextCursor(cursor);
                return true;
            }
        }
    }

    return QTextEdit::eventFilter(watched, event);
}