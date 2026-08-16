#pragma once
#include <QPlainTextEdit>
#include <QRegularExpression>
#include <QSyntaxHighlighter>

template<typename T, typename... Args>
static constexpr auto make_array(Args&&... args) -> std::array<T,sizeof...(args)>
{
    return {T{std::forward<Args>(args)}...};
}


// currently works the same for the .tl and .json, but if the json file is standard highlighting works as expecte
class TLSyntaxHighlighter final : public QSyntaxHighlighter
{
    Q_OBJECT
public:
    explicit TLSyntaxHighlighter(QTextDocument* parent);

protected:
    void highlightBlock(const QString &text) override;

private:
    struct HighlightingRule
    {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    QVector<HighlightingRule> highlighting_rules;

    static constexpr QColor keyword_color{22, 45, 196};
    static constexpr QColor comment_color{61, 62, 64};
    static constexpr QColor string_color{28, 102, 22};

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
