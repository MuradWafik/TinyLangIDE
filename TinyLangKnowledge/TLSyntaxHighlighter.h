#pragma once
#include <QJsonArray>
#include <QJsonObject>
#include <QPlainTextEdit>
#include <QRegularExpression>
#include <QSet>
#include <QSyntaxHighlighter>

template<typename T, typename... Args>
static constexpr auto make_array(Args&&... args) -> std::array<T,sizeof...(args)>
{
    return {T{std::forward<Args>(args)}...};
}

class TLSyntaxHighlighter final : public QSyntaxHighlighter
{
    Q_OBJECT
public:
    explicit TLSyntaxHighlighter(QTextDocument* parent);
    void SetSemanticSymbols(const QJsonArray& symbols);

protected:
    void highlightBlock(const QString &text) override;

private:
    struct HighlightingRule
    {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    QVector<HighlightingRule> highlighting_rules;

    QSet<QString> semantic_functions;
    QSet<QString> semantic_types;
    QSet<QString> semantic_variables;

    QColor keyword_color{86, 156, 214};
    QColor comment_color{106, 153, 85};
    QColor string_color{206, 145, 120};
    QColor function_color{220, 220, 170};
    QColor type_color{78, 201, 176};
    QColor variable_color{156, 220, 254};

    void AddRule(const QString& pattern, const QColor& color, QFont::Weight weight = QFont::Normal);
    void AddRule(const QRegularExpression& regex, const QColor& color, QFont::Weight weight = QFont::Normal);

    void ApplyRegexMatches(const QString& text, const QRegularExpression& regex, const QTextCharFormat& format);
    void HighlightSemanticSymbols(const QString& text);
    void HighlightInterpolatedString(int str_start, const QString& str_content);
    void HighlightInterpolatedExpression(int expr_global_pos, const QString& expr_text);

    static constexpr  auto keywords =
        make_array<QLatin1StringView>
        (
            "fn",
            "var",
            "if",
            "else",
            "while",
            "return",
            "true",
            "false",
            "break",
            "continue",
            "int",
            "any",
            "float",
            "bool",
            "String",
            "char",
            "void",
            "native",
            "module",
            "struct",
            "self",
            "for",
            "in",
            "range",
            "enum",
            "interface",
            "extend",
            "switch",
            "import",
            "export",
            "as",
            "is"
    );
};
