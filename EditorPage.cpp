#include "EditorPage.h"
#include "ui_EditorPage.h"

#include <QFileSystemModel>

#include "CodeEditor.h"
#include "TinyLangUtils.h"


EditorPage::EditorPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::EditorPage)
{
    ui->setupUi(this);

    file_model = new QFileSystemModel(this);

    // connect(file_model, QFileSystemModel:)
    connect(ui->fileTree, &QTreeView::clicked, this, &EditorPage::OnFileOpened);
    connect(ui->editorTabs, &QTabWidget::tabCloseRequested, this, &EditorPage::OnTabClosed);
}


EditorPage::~EditorPage()
{
    delete ui;
}

void EditorPage::Initialize(const QDir* dir)
{
    this->project_dir = dir;
    file_model->setRootPath(project_dir->path()); // Starts the worker thread to gather data

    this->ui->fileTree->setModel(file_model);
    this->ui->fileTree->setRootIndex(file_model->index(project_dir->path()));
}

void EditorPage::OnFileOpened(const QModelIndex& index) const
{
    // auto tabs = this->ui
    auto* stacked = this->ui->stackedWidget;
    stacked->setCurrentIndex(stacked->indexOf(ui->editorTabPage));

    ui->editorTabs->addTab(new CodeEditor(file_model->filePath(index)), file_model->fileName(index));
}

void EditorPage::OnTabClosed(const int index) const
{
    QWidget* editor = ui->editorTabs->widget(index);
    ui->editorTabs->removeTab(index);
    delete editor;
}
