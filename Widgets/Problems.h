#ifndef PROBLEMS_H
#define PROBLEMS_H

#include <QTreeWidget>
#include <QWidget>

#include "CodeEditor.h"
#include "TinyLangUtils.h"


QT_BEGIN_NAMESPACE
namespace Ui { class Problems; }
QT_END_NAMESPACE

class Problems final : public QWidget {
Q_OBJECT

public:
    explicit Problems(QWidget *parent = nullptr);
    ~Problems() override;

    void SetFileDiagnostics(CodeEditor* editor, const QVector<TinyLangUtils::LintItem>& items);
    void RemoveFile(CodeEditor* editor);

signals:
    void DiagnosticClicked(CodeEditor* editor, int line, int column);

private:
    Ui::Problems *ui;
    QHash<CodeEditor*, QTreeWidgetItem*> file_items;

    void OnItemClicked(QTreeWidgetItem *item, int column);

};




#endif //PROBLEMS_H
