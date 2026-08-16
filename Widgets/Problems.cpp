// You may need to build the project (run Qt uic code generator) to get "ui_Problems.h" resolved

#include "Problems.h"
#include "ui_Problems.h"
#include "Widgets/FileItem.h"


Problems::Problems(QWidget *parent) :
    QWidget(parent), ui(new Ui::Problems)
{
    ui->setupUi(this);

    connect(ui->treeWidget, &QTreeWidget::itemClicked,
            this, &Problems::OnItemClicked);
}

Problems::~Problems()
{
    delete ui;
}

void Problems::SetFileDiagnostics(CodeEditor* editor, const QVector<TinyLangUtils::LintItem>& items)
{
    QTreeWidgetItem* file_item = file_items.value(editor, nullptr);

    if (!file_item)
    {
        auto* new_file_item = new FileItem(editor);
        new_file_item->setText(0, editor->file_path.path());

        ui->treeWidget->addTopLevelItem(new_file_item);

        file_items.insert(editor, new_file_item);

        connect(editor, &QObject::destroyed, this, [this, editor]
        {
            RemoveFile(editor);
        });

        file_item = new_file_item;
    }

    qDeleteAll(file_item->takeChildren());

    for (const auto& lint_item : items)
    {
        auto* diagnostic = new DiagnosticItem(
            lint_item.line,
            lint_item.col
        );

        diagnostic->setText(0, lint_item.ToString());

        file_item->addChild(diagnostic);
    }

    file_item->setExpanded(true);
}

void Problems::RemoveFile(CodeEditor* editor)
{
    const auto it = file_items.find(editor);

    if(it == file_items.end()) return;

    delete it.value();
    file_items.erase(it);
}

void Problems::OnItemClicked(QTreeWidgetItem* item, int)
{
    const auto* diagnostic = dynamic_cast<DiagnosticItem*>(item);
    if(!diagnostic) return;

    const auto* file_item = dynamic_cast<FileItem*>(diagnostic->parent());
    if(!file_item || !file_item->editor) return;

    emit DiagnosticClicked(file_item->editor, diagnostic->line, diagnostic->column);
}