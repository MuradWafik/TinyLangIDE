#include "TLSyntaxHighlighter.h"

#include <QRegularExpression>


TLSyntaxHighlighter::TLSyntaxHighlighter(QTextDocument* parent)
    : QSyntaxHighlighter{parent}
{
    AddRule(QStringLiteral(R"(\b[a-z_][A-Za-z0-9_]*\b)"), variable_color);
    AddRule(QStringLiteral(R"(\b[A-Z][A-Za-z0-9_]*\b)"), type_color);
    AddRule(QStringLiteral(R"(\b[A-Za-z_][A-Za-z0-9_]*(?=\s*\())"), function_color);

    for(const auto keyword : keywords)
    {
        AddRule(QStringLiteral("\\b") + keyword.toString() + QStringLiteral("\\b"), keyword_color);
    }

    AddRule(QStringLiteral(R"(\b\d+(\.\d+)?\b)"), QColor(181, 206, 168));
    AddRule(QStringLiteral(R"(//[^\n]*)"), comment_color);
}

void TLSyntaxHighlighter::SetSemanticSymbols(const QJsonArray& symbols)
{
    semantic_functions.clear();
    semantic_types.clear();
    semantic_variables.clear();

    for(const auto& val : symbols)
    {
        const auto obj = val.toObject();
        const QString name = obj["name"].toString();
        const QString kind = obj["kind"].toString();

        if(name.isEmpty()) continue;
        if(kind == "function" || kind == "native_function" || kind == "method") semantic_functions.insert(name);
        else if(kind == "struct" || kind == "interface" || kind == "enum") semantic_types.insert(name);
        else if(kind == "variable") semantic_variables.insert(name);
    }

    rehighlight();
}

void TLSyntaxHighlighter::AddRule(const QString& pattern, const QColor& color, const QFont::Weight weight)
{
    AddRule(QRegularExpression(pattern), color, weight);
}

void TLSyntaxHighlighter::AddRule(const QRegularExpression& regex, const QColor& color, const QFont::Weight weight)
{
    QTextCharFormat format;
    format.setForeground(color);
    if(weight != QFont::Normal)
    {
        format.setFontWeight(weight);
    }
    highlighting_rules.emplace_back(regex, format);
}


void TLSyntaxHighlighter::ApplyRegexMatches(const QString& text, const QRegularExpression& regex, const QTextCharFormat& format)
{
    auto matches = regex.globalMatch(text);
    while(matches.hasNext())
    {
        const auto match = matches.next();
        setFormat(match.capturedStart(), match.capturedLength(), format);
    }
}

void TLSyntaxHighlighter::HighlightSemanticSymbols(const QString& text)
{
    QTextCharFormat func_fmt;
    func_fmt.setForeground(function_color);

    QTextCharFormat type_fmt;
    type_fmt.setForeground(type_color);

    QTextCharFormat var_fmt;
    var_fmt.setForeground(variable_color);

    static const QRegularExpression token_regex(QStringLiteral(R"(\b[A-Za-z_][A-Za-z0-9_]*\b)"));
    auto matches = token_regex.globalMatch(text);

    while(matches.hasNext())
    {
        const auto match = matches.next();
        const QString name = match.captured();

        if(semantic_functions.contains(name)) setFormat(match.capturedStart(), match.capturedLength(), func_fmt);
        else if(semantic_types.contains(name)) setFormat(match.capturedStart(), match.capturedLength(), type_fmt);
        else if(semantic_variables.contains(name)) setFormat(match.capturedStart(), match.capturedLength(), var_fmt);
    }
}

void TLSyntaxHighlighter::HighlightInterpolatedExpression(const int expr_global_pos, const QString& expr_text)
{
    static const QSet<QString> keyword_set = []() {
        QSet<QString> set;
        for(const auto kw : keywords) set.insert(kw.toString());
        return set;
    }();

    QTextCharFormat kw_fmt;
    kw_fmt.setForeground(keyword_color);

    QTextCharFormat func_fmt;
    func_fmt.setForeground(function_color);

    QTextCharFormat type_fmt;
    type_fmt.setForeground(type_color);

    QTextCharFormat var_fmt;
    var_fmt.setForeground(variable_color);

    QTextCharFormat num_fmt;
    num_fmt.setForeground(QColor(181, 206, 168));

    static const QRegularExpression id_regex(QStringLiteral(R"(\b[A-Za-z_][A-Za-z0-9_]*\b)"));
    auto id_matches = id_regex.globalMatch(expr_text);
    while(id_matches.hasNext())
    {
        const auto id_m = id_matches.next();
        const QString name = id_m.captured();
        const int token_pos = expr_global_pos + id_m.capturedStart();
        const int token_len = id_m.capturedLength();

        if(keyword_set.contains(name)) setFormat(token_pos, token_len, kw_fmt);
        else if(semantic_functions.contains(name)) setFormat(token_pos, token_len, func_fmt);
        else if(semantic_types.contains(name)) setFormat(token_pos, token_len, type_fmt);
        else if(semantic_variables.contains(name)) setFormat(token_pos, token_len, var_fmt);
        else setFormat(token_pos, token_len, var_fmt);
    }

    static const QRegularExpression num_regex(QStringLiteral(R"(\b\d+(\.\d+)?\b)"));
    auto num_matches = num_regex.globalMatch(expr_text);
    while(num_matches.hasNext())
    {
        const auto num_m = num_matches.next();
        setFormat(expr_global_pos + num_m.capturedStart(), num_m.capturedLength(), num_fmt);
    }
}

void TLSyntaxHighlighter::HighlightInterpolatedString(const int str_start, const QString& str_content)
{
    QTextCharFormat str_fmt;
    str_fmt.setForeground(string_color);
    setFormat(str_start, str_content.length(), str_fmt);

    QTextCharFormat brace_fmt;
    brace_fmt.setForeground(QColor(220, 220, 220));

    QTextCharFormat code_fmt;
    code_fmt.setForeground(QColor(212, 212, 212));

    int depth = 0;
    int expr_start = -1;

    for(int i = 0; i < str_content.length(); ++i)
    {
        if(str_content[i] == '{' && (i == 0 || str_content[i - 1] != '\\'))
        {
            if(depth == 0)
            {
                expr_start = i + 1;
                setFormat(str_start + i, 1, brace_fmt);
            }
            ++depth;
            continue;
        }

        if(str_content[i] != '}' || depth <= 0 || (i > 0 && str_content[i - 1] == '\\')) continue;

        --depth;
        if(depth != 0) continue;

        setFormat(str_start + i, 1, brace_fmt);
        const int expr_len = i - expr_start;
        if(expr_len > 0)
        {
            setFormat(str_start + expr_start, expr_len, code_fmt);
            HighlightInterpolatedExpression(str_start + expr_start, str_content.mid(expr_start, expr_len));
        }
    }
}

void TLSyntaxHighlighter::highlightBlock(const QString& text)
{
    for(const auto& [pattern, format] : highlighting_rules)
    {
        ApplyRegexMatches(text, pattern, format);
    }

    HighlightSemanticSymbols(text);

    QTextCharFormat str_fmt;
    str_fmt.setForeground(string_color);
    static const QRegularExpression normal_str_regex(QStringLiteral(R"RAW((?<!\$)"(?:\\.|[^"\\])*")RAW"));
    static const QRegularExpression char_regex(QStringLiteral(R"RAW('(?:\\.|[^'\\])*')RAW"));
    ApplyRegexMatches(text, normal_str_regex, str_fmt);
    ApplyRegexMatches(text, char_regex, str_fmt);

    static const QRegularExpression interp_regex(QStringLiteral(R"RAW(\$"(?:\\.|[^"\\])*")RAW"));
    auto interp_matches = interp_regex.globalMatch(text);
    while(interp_matches.hasNext())
    {
        const auto match = interp_matches.next();
        HighlightInterpolatedString(match.capturedStart(), match.captured());
    }

    QTextCharFormat comment_fmt;
    comment_fmt.setForeground(comment_color);
    static const QRegularExpression comment_regex(QStringLiteral(R"(//[^\n]*)"));
    ApplyRegexMatches(text, comment_regex, comment_fmt);
}
