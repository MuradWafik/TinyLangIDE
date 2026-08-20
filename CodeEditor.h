#pragma once

#include <QJsonArray>
#include <optional>

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
    void SetSemanticSymbols(const QJsonArray& symbols);

    struct SymbolSelection
    {
        QString name;
        int line = 0;
        int col = 0;
    };

    [[nodiscard]] std::optional<SymbolSelection> GetSymbolUnderCursor() const;

    QFileInfo file_path;

protected:
    void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;
    void keyPressEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

signals:
    void GoToDefinitionRequested(const QString& file_path, int line, int col);

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect &, int);

private:
    QColor background_color{27, 27, 27}; // TODO: Settings
    QColor hover_color{35, 35, 35}; // TODO: Settings
    QColor font_color{220, 220, 220}; // TODO: Settings
    QColor symbol_color{78, 201, 176}; // TODO: Settings
    QColor occurrence_color{45, 75, 95}; // TODO: Settings

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
    QAction* go_to_definition;

    int font_size = 11; // TODO: Settings

    [[nodiscard]] inline bool IsTLFile() const { return file_path.suffix() == "tl"; }
    [[nodiscard]] std::expected<void, QString> SaveTo(const QString& path) const;
};
