#pragma once

#include <QSyntaxHighlighter>
#include <QRegularExpression>

class LuaSyntaxHighlighter : public QSyntaxHighlighter {
    Q_OBJECT

public:
    LuaSyntaxHighlighter(QTextDocument *parent = nullptr);
    void setEnabled(bool enabled)
    {
        isEnabled = enabled;
    }

protected:
    void highlightBlock(const QString &text) override;

private:
    struct HighlightingRule
    {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    QList<HighlightingRule> highlightingRules;

    QRegularExpression commentStartExpression;
    QRegularExpression commentEndExpression;


    QTextCharFormat keywordFormat;
    QTextCharFormat symbolFormat;
    QTextCharFormat singleLineCommentFormat;
    QTextCharFormat multiLineCommentFormat;
    QTextCharFormat fullQuotationFormat;
    QTextCharFormat smallQuotationFormat;
    QTextCharFormat functionFormat;

    bool isEnabled = false;

};