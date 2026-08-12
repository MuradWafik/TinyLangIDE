#pragma once

#include <QPlainTextEdit>
#include <QWidget>

#include "TLSyntaxHighlighter.h"

class CodeEditor final : public QPlainTextEdit
{
    Q_OBJECT
public:
    void InitializeLineNumbers();

    explicit CodeEditor(QWidget *parent = nullptr);
    explicit CodeEditor(QString file_path, QWidget* parent = nullptr);

    void lineNumberAreaPaintEvent(const QPaintEvent *event) const;
    int lineNumberAreaWidth() const;
    QString file_path;

protected:
    void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect &, int);

private:
    constexpr static QColor backgroundColor{27, 27, 27};
    constexpr static QColor hover_color{35, 35, 35};
    constexpr static QColor font_color{255, 255, 255};

    std::unique_ptr<TLSyntaxHighlighter> syntax_highlighter;
    QWidget* lineNumberArea;
};
