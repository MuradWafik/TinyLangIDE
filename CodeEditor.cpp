#include "CodeEditor.h"

#include <QApplication>
#include <QContextMenuEvent>
#include <QFile>
#include <QFontDatabase>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QTextBlock>
#include <QTimer>

#include "IMenuProvider.h"
#include "LineNumberArea.h"
#include "TinyLangUtils.h"


void CodeEditor::InitializeLineNumbers()
{
    QFont editor_font(QStringLiteral("Ubuntu Sans Mono"));
    editor_font.setStyleHint(QFont::Monospace);
    editor_font.setPointSize(12);
    setFont(editor_font);
    document()->setDocumentMargin(6);

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

    go_to_definition = MakeAction(
        tr("Go to Definition"), QKeySequence(Qt::CTRL | Qt::Key_B), this);
    go_to_definition->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    addAction(go_to_definition);
    connect(go_to_definition, &QAction::triggered, this, [this]
    {
        if(const auto sel = GetSymbolUnderCursor(); sel.has_value())
        {
            emit GoToDefinitionRequested(this->file_path.absoluteFilePath(), sel->line, sel->col);
        }
    });
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
    return 10 + (fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits) + 12;
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
    QList<QTextEdit::ExtraSelection> extra_selections;

    if(!isReadOnly())
    {
        QTextEdit::ExtraSelection selection;

        const QColor line_color = background_color.lighter(160);

        selection.format.setBackground(line_color);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extra_selections.append(selection);
    }

    if(const auto sel = GetSymbolUnderCursor(); sel.has_value())
    {
        const auto* doc = document();
        const QRegularExpression reg(QStringLiteral("\\b") + QRegularExpression::escape(sel->name) + QStringLiteral("\\b"));
        QTextCursor search_cursor = textCursor();
        search_cursor.setPosition(0);

        while(true)
        {
            search_cursor = doc->find(reg, search_cursor);
            if(search_cursor.isNull()) break;

            QTextEdit::ExtraSelection symbol_sel;
            symbol_sel.format.setBackground(occurrence_color);
            symbol_sel.cursor = search_cursor;
            extra_selections.append(symbol_sel);
        }
    }

    setExtraSelections(extra_selections);
}

void CodeEditor::lineNumberAreaPaintEvent(const QPaintEvent *event) const
{
    QPainter painter(lineNumberArea);

    painter.fillRect(event->rect(), background_color);

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = static_cast<int>(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + static_cast<int>(blockBoundingRect(block).height());

    const QColor line_num_color(110, 118, 129);

    while (block.isValid() && top <= event->rect().bottom())
    {
        if (block.isVisible() && bottom >= event->rect().top())
        {
            QString number = QString::number(blockNumber + 1);
            painter.setPen(line_num_color);
            QRect rect(
                0, top,
                lineNumberArea->width() - 8, fontMetrics().height()
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
    return {};
}

void CodeEditor::SetCursorPosition(uint32_t line, uint32_t col)
{
    // switch to 0 based indexing, keeping users call as is for simplicity when calling
    if(line > 0) --line;
    if(col > 0) --col;

    if(line >= static_cast<uint32_t>(document()->blockCount())) return;

    auto cursor = textCursor();
    const QTextBlock block = document()->findBlockByLineNumber(line);
    const int col_clamped = std::min(static_cast<int>(col), block.length() - 1);
    const int abs_pos = block.position() + col_clamped;
    cursor.setPosition(abs_pos, QTextCursor::MoveAnchor);
    setTextCursor(cursor);
}

void CodeEditor::SetSemanticSymbols(const QJsonArray& symbols)
{
    if(syntax_highlighter)
    {
        syntax_highlighter->SetSemanticSymbols(symbols);
    }
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

    document()->setModified(false);
    return {};
}

std::optional<CodeEditor::SymbolSelection> CodeEditor::GetSymbolUnderCursor() const
{
    QTextCursor cursor = textCursor();
    QString symbol = cursor.selectedText().trimmed();

    int start_pos = cursor.selectionStart();
    if(symbol.isEmpty())
    {
        cursor.select(QTextCursor::WordUnderCursor);
        symbol = cursor.selectedText().trimmed();
        start_pos = cursor.selectionStart();
    }

    if(symbol.isEmpty() || (!symbol.at(0).isLetter() && symbol.at(0) != '_'))
    {
        return std::nullopt;
    }

    const int line = cursor.blockNumber() + 1;
    const int col = start_pos - cursor.block().position() + 1;

    return SymbolSelection{
        .name = std::move(symbol),
        .line = line,
        .col = col
    };
}

void CodeEditor::contextMenuEvent(QContextMenuEvent* event)
{
    QMenu* menu = createStandardContextMenu(event->pos());
    if(const auto sel = GetSymbolUnderCursor(); sel.has_value())
    {
        menu->addSeparator();
        go_to_definition->setText(tr("Go to Definition (%1)").arg(sel->name));
        menu->addAction(go_to_definition);
    }
    menu->exec(event->globalPos());
    delete menu;
}

void CodeEditor::mousePressEvent(QMouseEvent* event)
{
    if(event->button() == Qt::LeftButton && (event->modifiers() & Qt::ControlModifier))
    {
        const QTextCursor click_cursor = cursorForPosition(event->pos());
        setTextCursor(click_cursor);

        if(const auto sel = GetSymbolUnderCursor(); sel.has_value())
        {
            emit GoToDefinitionRequested(file_path.absoluteFilePath(), sel->line, sel->col);
            event->accept();
            return;
        }
    }

    QPlainTextEdit::mousePressEvent(event);
}

void CodeEditor::keyPressEvent(QKeyEvent* event)
{
    if(event->key() == Qt::Key_Tab && event->modifiers() == Qt::NoModifier)
    {
        if(tab_type.type == TabType::Type::SpacesKey)
        {
            insertPlainText(QString(tab_type.size, ' '));
        }
        else
        {
            setTabStopDistance(QFontMetrics(this->font()).horizontalAdvance(' ') * tab_type.size);
        }
        return;
    }
    QPlainTextEdit::keyPressEvent(event);
}
