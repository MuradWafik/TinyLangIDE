#include "CodeEditor.h"

#include <qfile.h>
#include <QMessageBox>
#include <QPainter>
#include <QTextBlock>
#include <QTimer>

#include "LineNumberArea.h"
#include "TinyLangUtils.h"


void CodeEditor::InitializeLineNumbers()
{
    lineNumberArea = new LineNumberArea(this);
    syntax_highlighter = new TLSyntaxHighlighter(document());

    connect(this, &CodeEditor::blockCountChanged, this, &CodeEditor::updateLineNumberAreaWidth);
    connect(this, &CodeEditor::updateRequest, this, &CodeEditor::updateLineNumberArea);
    connect(this, &CodeEditor::cursorPositionChanged, this, &CodeEditor::highlightCurrentLine);

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
}

CodeEditor::CodeEditor(QString file_path, QWidget* parent)
    : QPlainTextEdit{parent}
    , file_path{std::move(file_path)}
    , run_lint_timer{new QTimer(this)}
{
    QFile file{this->file_path.absoluteFilePath()};
    if(!file.open(QIODevice::ReadWrite | QIODevice::Text))
    {
        QMessageBox::warning(
            this,
            tr("Error"),
            tr("Unable to open file %1").arg(file.fileName())
        );
        return;
    }

    QTextStream stream(&file);
    this->setPlainText(stream.readAll());

    InitializeLineNumbers();

    if(IsTLFile()) run_lint_timer->start(run_lint_time_ms);

    connect(this, &CodeEditor::textChanged, this,  &CodeEditor::UpdateLintTimer);
    connect(run_lint_timer, &QTimer::timeout, this, &CodeEditor::RunLint);

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

void CodeEditor::UpdateLintTimer() const
{
    if(!IsTLFile() || !document()->isModified()) return;

    // automatically restarts
    run_lint_timer->start(run_lint_time_ms);
}

void CodeEditor::RunLint()
{
    if(lint_save_mode == LintSaveMode::SaveBeforeLint)
    {
        if(document()->isModified())
        {
            if(const auto result = Save();
                !result)
            {
                QMessageBox::warning(this, tr("Error Saving"), result.error());
                return;
            }
        }
    }

    const auto res = TinyLangUtils::CheckProject(this->file_path.absoluteFilePath());
    // will run in its own qprocess (for all editors doing so)

    run_lint_timer->stop();

    if(!res)
    {
        QMessageBox::warning(this, tr("Error Linting"), res.error());
        return;
    }

    emit OnFileLinted(this,res.value());
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


std::expected<void, QString> CodeEditor::Save() const
{
    return SaveTo(file_path.absoluteFilePath());
}

std::expected<void, QString> CodeEditor::SaveAs(const QString& path)
{
    if(const auto result = SaveTo(path); !result) return result;

    file_path = QFileInfo(path);
    document()->setModified(false);
    return {};
}

void CodeEditor::SetCursorPosition(const uint32_t line, const uint32_t col)
{
    if(line >= static_cast<uint32_t>(document()->blockCount())) return;

    auto cursor = textCursor();
    const QTextBlock block = document()->findBlockByLineNumber(line);
    const int col_clamped = std::min(static_cast<int>(col), block.length() - 1);
    const int abs_pos = block.position() + col_clamped;
    cursor.setPosition(abs_pos, QTextCursor::MoveAnchor);
    setTextCursor(cursor);
}

std::expected<void, QString> CodeEditor::SaveTo(const QString& path) const
{
    QFile file(path);

    if(!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        return std::unexpected(tr("Unable to open file %1").arg(path));
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    QTextBlock block = document()->begin();

    while(block.isValid())
    {
        out << block.text();
        block = block.next();
        if(block.isValid()) out << line_ending;
    }
    return {};
}
