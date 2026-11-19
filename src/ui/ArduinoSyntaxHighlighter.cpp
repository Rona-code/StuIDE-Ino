#include "ArduinoSyntaxHighlighter.h"

ArduinoSyntaxHighlighter::ArduinoSyntaxHighlighter(QTextDocument* parent)
    : QSyntaxHighlighter(parent)
{
    // Types & mots-clés structurels
    QTextCharFormat keywordFormat;
    keywordFormat.setForeground(QColor("#569cd6")); // Bleu doux
    keywordFormat.setFontWeight(QFont::Bold);

    const QStringList keywords = {
        "void", "int", "long", "short", "char", "float", "double", "bool",
        "byte", "word", "String", "uint8_t", "uint16_t", "uint32_t",
        "int8_t", "int16_t", "int32_t", "const", "static", "volatile",
        "if", "else", "for", "while", "do", "switch", "case", "break",
        "continue", "return", "setup", "loop"
    };
    for (const QString& kw : keywords) {
        m_rules.append({
            QRegularExpression(QString("\\b%1\\b").arg(kw)),
            keywordFormat
            });
    }

    // Fonctions & objets Arduino courants
    QTextCharFormat arduinoApiFormat;
    arduinoApiFormat.setForeground(QColor("#dcdcaa")); // Jaune pâle

    const QStringList arduinoApis = {
        "pinMode", "digitalWrite", "digitalRead", "analogRead", "analogWrite",
        "analogReference", "delay", "delayMicroseconds", "millis", "micros",
        "map", "constrain", "min", "max", "abs", "sq", "sqrt", "pow",
        "attachInterrupt", "detachInterrupt", "noInterrupts", "interrupts",
        "tone", "noTone", "shiftIn", "shiftOut", "pulseIn", "Serial"
    };
    for (const QString& api : arduinoApis) {
        m_rules.append({
            QRegularExpression(QString("\\b%1\\b").arg(api)),
            arduinoApiFormat
            });
    }

    // Constantes (HIGH, LOW, INPUT, OUTPUT, LED_BUILTIN, true, false...)
    QTextCharFormat constantFormat;
    constantFormat.setForeground(QColor("#4fc1ff")); // Cyan

    const QStringList constants = {
        "HIGH", "LOW", "INPUT", "OUTPUT", "INPUT_PULLUP",
        "LED_BUILTIN", "true", "false", "nullptr", "NULL"
    };
    for (const QString& c : constants) {
        m_rules.append({
            QRegularExpression(QString("\\b%1\\b").arg(c)),
            constantFormat
            });
    }

    // Nombres (décimaux et hexadécimaux)
    QTextCharFormat numberFormat;
    numberFormat.setForeground(QColor("#b5cea8")); // Vert clair
    m_rules.append({
        QRegularExpression("\\b(0x[0-9a-fA-F]+|\\d+(\\.\\d+)?)\\b"),
        numberFormat
        });

    // Chaînes de caractères "..." et '...'
    QTextCharFormat stringFormat;
    stringFormat.setForeground(QColor("#ce9178")); // Orange brique
    m_rules.append({
        QRegularExpression("\"([^\"\\\\]|\\\\.)*\"|'([^'\\\\]|\\\\.)*'"),
        stringFormat
        });

    // Commentaires sur une ligne //
    QTextCharFormat singleLineCommentFormat;
    singleLineCommentFormat.setForeground(QColor("#6a9955")); // Vert commentaire
    singleLineCommentFormat.setFontItalic(true);
    m_rules.append({
        QRegularExpression("//[^\n]*"),
        singleLineCommentFormat
        });

    // Commentaires multi-lignes /* ... */
    m_multiLineCommentFormat.setForeground(QColor("#6a9955"));
    m_multiLineCommentFormat.setFontItalic(true);
    m_commentStartExpression = QRegularExpression("/\\*");
    m_commentEndExpression = QRegularExpression("\\*/");
}

void ArduinoSyntaxHighlighter::highlightBlock(const QString& text) {
    for (const HighlightRule& rule : std::as_const(m_rules)) {
        QRegularExpressionMatchIterator it = rule.pattern.globalMatch(text);
        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }

    setCurrentBlockState(0);
    int startIndex = 0;
    if (previousBlockState() != 1) {
        startIndex = text.indexOf(m_commentStartExpression);
    }

    while (startIndex >= 0) {
        QRegularExpressionMatch match = m_commentEndExpression.match(text, startIndex);
        int endIndex = match.capturedStart();
        int commentLength = 0;

        if (endIndex == -1) {
            setCurrentBlockState(1);
            commentLength = text.length() - startIndex;
        }
        else {
            commentLength = endIndex - startIndex + match.capturedLength();
        }

        setFormat(startIndex, commentLength, m_multiLineCommentFormat);
        startIndex = text.indexOf(m_commentStartExpression, startIndex + commentLength);
    }
}