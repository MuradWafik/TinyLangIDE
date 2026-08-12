#include "CodeEditor.h"

#include <qfile.h>
#include <QMessageBox>
#include <QPainter>
#include <QTextBlock>

#include "LineNumberArea.h"


void CodeEditor::InitializeLineNumbers()
{
    lineNumberArea = new LineNumberArea(this);
    syntax_highlighter = std::make_unique<TLSyntaxHighlighter>(document());

    connect(this, &CodeEditor::blockCountChanged, this, &CodeEditor::updateLineNumberAreaWidth);
    connect(this, &CodeEditor::updateRequest, this, &CodeEditor::updateLineNumberArea);
    connect(this, &CodeEditor::cursorPositionChanged, this, &CodeEditor::highlightCurrentLine);

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
}

CodeEditor::CodeEditor(QWidget *parent) : QPlainTextEdit(parent)
{
    InitializeLineNumbers();
}

CodeEditor::CodeEditor(QString file_path, QWidget* parent) : QPlainTextEdit{parent}, file_path{std::move(file_path)}
{
    QFile file{this->file_path};
    if(!file.open(QIODevice::ReadWrite | QIODevice::Text))
    {
        QMessageBox::warning(
            this,
            "Error",
            QString("Unable to open file %1").arg(file.fileName())
        );
        return;
    }

    QTextStream stream(&file);
    this->setPlainText(stream.readAll());

    InitializeLineNumbers();
}



int CodeEditor::lineNumberAreaWidth() const
{
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10)
    {
        max /= 10;
        ++digits;
    }

    digits = qMax(digits, 2); // At least 2 digits wide

    constexpr int leftPadding  = LineNumberArea::rightMargin / 2;
    constexpr int rightPadding = LineNumberArea::rightMargin;

    return
        leftPadding
        + (fontMetrics().averageCharWidth() * digits)
        + rightPadding;
}

void CodeEditor::updateLineNumberAreaWidth(int /* newBlockCount */)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void CodeEditor::updateLineNumberArea(const QRect &rect, const int dy)
{
    if (dy)
        lineNumberArea->scroll(0, dy);
    else
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void CodeEditor::resizeEvent(QResizeEvent *event)
{
    QPlainTextEdit::resizeEvent(event);

    const QRect cr = contentsRect();
    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void CodeEditor::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (!isReadOnly())
    {
        QTextEdit::ExtraSelection selection;

        const QColor lineColor = backgroundColor.lighter(160);

        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    setExtraSelections(extraSelections);
}

void CodeEditor::lineNumberAreaPaintEvent(const QPaintEvent *event) const
{
    QPainter painter(lineNumberArea);

    painter.fillRect(event->rect(), backgroundColor);

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = static_cast<int>(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + static_cast<int>(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom())
    {
        if (block.isVisible() && bottom >= event->rect().top())
        {
            QString number = QString::number(blockNumber + 1);
            painter.setPen(font_color);
            QRect rect(
                0, top,
                lineNumberArea->width() - LineNumberArea::rightMargin, fontMetrics().height()
            );

            painter.drawText(rect, Qt::AlignRight | Qt::AlignVCenter, number);
        }

        block = block.next();
        top = bottom;
        bottom = top + static_cast<int>(blockBoundingRect(block).height());
        ++blockNumber;
    }
}
