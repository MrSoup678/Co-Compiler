//
// Created by trico on 24-3-24.
//

#include "luasyntaxhighlighting.h"

LuaSyntaxHighlighter::LuaSyntaxHighlighter(QTextDocument *parent) : QSyntaxHighlighter(parent)
{
    HighlightingRule rule;

    keywordFormat.setForeground(QColor(2, 227, 152));

    const QString keywordPatterns[] = {
            QStringLiteral("and"),QStringLiteral("break"),QStringLiteral("do"),QStringLiteral("else"), QStringLiteral("elseif"), QStringLiteral("end"),
            QStringLiteral("false"),QStringLiteral("for"),QStringLiteral("function"),QStringLiteral("goto"),QStringLiteral("if"), QStringLiteral("in"),
            QStringLiteral("local"),QStringLiteral("nil"),QStringLiteral("not"),QStringLiteral("or"),QStringLiteral("repeat"), QStringLiteral("return"),
            QStringLiteral("in"), QStringLiteral("true"), QStringLiteral("until"), QStringLiteral("while")
    };

    for (const QString &pattern : keywordPatterns) {
        rule.pattern = QRegularExpression("\\b"+pattern+"\\b");
        rule.format = keywordFormat;
        highlightingRules.append(rule);
    }

    symbolFormat.setForeground(QColor(214, 179, 4));

    const QString symbolPatterns[] = {
            QStringLiteral("+"),QStringLiteral("-"), QStringLiteral("*"),QStringLiteral("/"), QStringLiteral("%"),QStringLiteral("^"), QStringLiteral("#"),
            QStringLiteral("&"),QStringLiteral("~"), QStringLiteral("|"), QStringLiteral("<<"), QStringLiteral(">>"), QStringLiteral("//"),
            QStringLiteral("=="), QStringLiteral("-="), QStringLiteral("<="), QStringLiteral(">="), QStringLiteral("<"), QStringLiteral(">"), QStringLiteral("="),
            QStringLiteral("("), QStringLiteral(")"), QStringLiteral("{"), QStringLiteral("}"), QStringLiteral("["), QStringLiteral("]"), QStringLiteral("::"),
            QStringLiteral(";"), QStringLiteral(":"), QStringLiteral(","), QStringLiteral("."), QStringLiteral(".."), QStringLiteral("..."),
    };

    for (const QString &pattern : symbolPatterns) {
        rule.pattern = QRegularExpression(QRegularExpression::escape(pattern));
        rule.format = symbolFormat;
        highlightingRules.append(rule);
    }

    fullQuotationFormat.setForeground(Qt::darkGreen);
    rule.pattern = QRegularExpression(QStringLiteral("\".*\""));
    rule.format = fullQuotationFormat;
    highlightingRules.append(rule);

    smallQuotationFormat.setForeground(Qt::darkGreen);
    rule.pattern = QRegularExpression(QStringLiteral("'.*'"));
    rule.format = smallQuotationFormat;
    highlightingRules.append(rule);

    functionFormat.setForeground(QColor(18, 148, 199));
    rule.pattern = QRegularExpression(QStringLiteral("[A-Za-z0-9_]+(?=\\()"));
    rule.format = functionFormat;
    highlightingRules.append(rule);

    singleLineCommentFormat.setForeground(Qt::darkGray);
    rule.pattern = QRegularExpression(QStringLiteral("--[^\n]*"));
    rule.format = singleLineCommentFormat;
    highlightingRules.append(rule);

    multiLineCommentFormat.setForeground(Qt::darkGray);

    commentStartExpression = QRegularExpression(QStringLiteral("\\-\\-\\[\\["));
    commentEndExpression = QRegularExpression(QStringLiteral("\\]\\]\\-\\-"));
}

void LuaSyntaxHighlighter::highlightBlock(const QString &text)
{
    if(!isEnabled)
        return;

    for (const HighlightingRule &rule : std::as_const(highlightingRules)) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }

    setCurrentBlockState(0);

    int startIndex = 0;
    if (previousBlockState() != 1)
        startIndex = text.indexOf(commentStartExpression);

    while (startIndex >= 0) {
        QRegularExpressionMatch match = commentEndExpression.match(text, startIndex);
        int endIndex = match.capturedStart();
        int commentLength = 0;
        if (endIndex == -1) {
            setCurrentBlockState(1);
            commentLength = text.length() - startIndex;
        } else {
            commentLength = endIndex - startIndex
                            + match.capturedLength();
        }
        setFormat(startIndex, commentLength, multiLineCommentFormat);
        startIndex = text.indexOf(commentStartExpression, startIndex + commentLength);
    }
}
