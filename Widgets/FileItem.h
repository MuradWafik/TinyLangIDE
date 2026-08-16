#pragma once
#include <QTreeWidget>

#include "CodeEditor.h"

// the problems widget stores these directly instead to allow for quick reference to the code editor in the tree when clicked
class FileItem final : public QTreeWidgetItem
{
public:
    explicit FileItem(CodeEditor* editor)
        : editor(editor)
    {}

    CodeEditor* editor;
};


class DiagnosticItem final : public QTreeWidgetItem
{
public:
    DiagnosticItem(const int line, const int column)
        : line(line), column(column)
    {}

    int line;
    int column;
};
