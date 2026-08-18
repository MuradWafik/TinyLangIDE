// You may need to build the project (run Qt uic code generator) to get "ui_Problems.h" resolved

#include "Problems.h"
#include "ui_Problems.h"
#include "Widgets/FileItem.h"


Problems::Problems(QWidget *parent) :
    QWidget(parent), ui(new Ui::Problems)
{
    ui->setupUi(this);
    ui->treeWidget->setFocusPolicy(Qt::NoFocus);    
    connect(ui->treeWidget, &QTreeWidget::itemClicked, this, &Problems::OnItemClicked);
}

Problems::~Problems()
{
    delete ui;
}

void Problems::SetDiagnostics(const QVector<TinyLangUtils::LintItem>& items) const
{
    ui->treeWidget->clear();

    QMap<QString, QVector<TinyLangUtils::LintItem>> grouped;
    for (const auto& item : items)
    {
        grouped[item.file].append(item);
    }

    for (auto it = grouped.constBegin(); it != grouped.constEnd(); ++it)
    {
        const QString& file_path = it.key();
        const auto& file_items = it.value();

        auto* file_item = new FileItem(file_path);
        const QFileInfo file_info(file_path);
        file_item->setText(0, file_info.fileName().isEmpty() ? file_path : file_info.fileName());
        file_item->setToolTip(0, file_path);

        for (const auto& lint_item : file_items)
        {
            auto* diagnostic = new DiagnosticItem(lint_item.line, lint_item.col);
            diagnostic->setText(0, lint_item.ToString());
            file_item->addChild(diagnostic);
        }

        ui->treeWidget->addTopLevelItem(file_item);
        file_item->setExpanded(true);
    }
}

void Problems::ClearDiagnostics() const
{
    ui->treeWidget->clear();
}

void Problems::OnItemClicked(QTreeWidgetItem* item, int)
{
    const auto* diagnostic = dynamic_cast<DiagnosticItem*>(item);
    if(!diagnostic) return;

    const auto* file_item = dynamic_cast<FileItem*>(diagnostic->parent());
    if(!file_item || file_item->file_path.isEmpty()) return;

    emit DiagnosticClicked(file_item->file_path, diagnostic->line, diagnostic->column);
}