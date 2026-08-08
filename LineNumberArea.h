#pragma once

#include <QWidget>
#include "CodeEditor.h"

class LineNumberArea final : public QWidget
{
public:
    explicit LineNumberArea(CodeEditor *editor) : QWidget(editor)
    {
        codeEditor = editor;
        this->setContentsMargins(0, 0, 10, 0);
    }

    QSize sizeHint() const override
    {
        return QSize(codeEditor->lineNumberAreaWidth(), 0);
    }

    constexpr static int rightMargin = 6;

protected:
    void paintEvent(QPaintEvent *event) override
    {
        codeEditor->lineNumberAreaPaintEvent(event);
    }

private:
    CodeEditor *codeEditor;
};

