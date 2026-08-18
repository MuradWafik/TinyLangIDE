#pragma once

#include <expected>

#include "TinyLangUtils.h"
#include "TLSyntaxHighlighter.h"


enum class LintSaveMode
{
    SaveBeforeLint,
    LintSavedFileOnly
};


class CodeEditor final : public QPlainTextEdit
{
    Q_OBJECT
public:
    void InitializeLineNumbers();

    explicit CodeEditor(QString file_path, QWidget* parent = nullptr);

    void lineNumberAreaPaintEvent(const QPaintEvent *event) const;
    [[nodiscard]] int lineNumberAreaWidth() const;

    [[nodiscard]] std::expected<void, QString> Save() const;
    std::expected<void, QString> SaveAs(const QString& path);

    void SetCursorPosition(uint32_t line, uint32_t col);


    QFileInfo file_path;

protected:
    void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect &, int);

private:
    constexpr static QColor backgroundColor{27, 27, 27};
    constexpr static QColor hover_color{35, 35, 35};
    constexpr static QColor font_color{255, 255, 255};

    QString line_ending = "\n"; // TODO: setting for user to pick between lf vs crlf

    struct TabType
    {
        uint32_t size = 4;
        enum class Type
        {
            SpacesKey,
            TabKey
        } type = Type::SpacesKey;
    } tab_type; // TODO: Settings

    TLSyntaxHighlighter* syntax_highlighter;
    QWidget* lineNumberArea;

    [[nodiscard]] inline bool IsTLFile() const { return file_path.suffix() == "tl"; }
    [[nodiscard]] std::expected<void, QString> SaveTo(const QString& path) const;
};
