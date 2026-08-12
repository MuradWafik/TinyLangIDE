#include "TLSyntaxHighlighter.h"

#include <QRegularExpression>


TLSyntaxHighlighter::TLSyntaxHighlighter(QTextDocument* parent)
    : QSyntaxHighlighter{parent}
{
    QTextCharFormat keywordFormat;
    keywordFormat.setForeground(keyword_color);
    keywordFormat.setFontWeight(QFont::Bold);

    for(const auto keyword : keywords)
    {
        highlighting_rules.append(HighlightingRule{
            QRegularExpression(
                QStringLiteral("\\b") +
                keyword.toString() +
                QStringLiteral("\\b")
            ),
            keywordFormat
        });
    }


    QTextCharFormat stringFormat;
    stringFormat.setForeground(string_color);

    highlighting_rules.emplace_back(
        QRegularExpression(R"("(?:\\.|[^"\\])*")"),
        stringFormat
    );


    QTextCharFormat commentFormat;
    commentFormat.setForeground(comment_color);

    highlighting_rules.emplace_back(
        QRegularExpression(R"(//[^\n]*)"),
        commentFormat
    );
}


void TLSyntaxHighlighter::highlightBlock(const QString& text)
{
    for(const auto& [pattern, format] : highlighting_rules)
    {
        auto matches = pattern.globalMatch(text);

        while(matches.hasNext())
        {
            const auto match = matches.next();

            setFormat(
                match.capturedStart(),
                match.capturedLength(),
                format
            );
        }
    }
}