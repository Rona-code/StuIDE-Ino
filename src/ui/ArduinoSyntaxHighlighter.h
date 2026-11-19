#pragma once

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>
#include <QVector>

class ArduinoSyntaxHighlighter : public QSyntaxHighlighter {
    Q_OBJECT
public:
    explicit ArduinoSyntaxHighlighter(QTextDocument* parent = nullptr);

protected:
    void highlightBlock(const QString& text) override;

private:
    struct HighlightRule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };

    QVector<HighlightRule> m_rules;
    QTextCharFormat m_multiLineCommentFormat;
    QRegularExpression m_commentStartExpression;
    QRegularExpression m_commentEndExpression;
};