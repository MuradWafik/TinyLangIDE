#pragma once

#include <QPlainTextEdit>
#include <QWidget>

class CodeEditor : public QPlainTextEdit
{
    Q_OBJECT
public:
    explicit CodeEditor(QWidget *parent = nullptr);

    void lineNumberAreaPaintEvent(const QPaintEvent *event) const;
    int lineNumberAreaWidth() const;

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

    QWidget *lineNumberArea;
};
