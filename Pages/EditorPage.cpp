#include "EditorPage.h"
#include "ui_EditorPage.h"

#include <QFileSystemModel>

#include "../CodeEditor.h"
#include "../TinyLangKnowledge/TinyLangUtils.h"


EditorPage::EditorPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::EditorPage)
    , close_action(new QAction("&Close", this))
    , file_model{new QFileSystemModel(this)}
{
    ui->setupUi(this);

    // cant afaik set in property editor
    ui->splitter->setSizes({220, 800});

    for (int i = 1; i < file_model->columnCount(); ++i) ui->fileTree->hideColumn(i);

    connect(ui->fileTree, &QTreeView::clicked, this, &EditorPage::OnFileOpened);
    connect(ui->editorTabs, &QTabWidget::tabCloseRequested, this, &EditorPage::OnTabClosed);

    connect(close_action, &QAction::triggered, this,
        [this]
        {
            if(const int index = ui->editorTabs->currentIndex();
                index >= 0)
            {
                OnTabClosed(index);
            }
        }
    );
}


EditorPage::~EditorPage()
{
    delete ui;
}

void EditorPage::Initialize(const QDir& dir)
{
    this->project_dir = dir;
    file_model->setRootPath(project_dir.path()); // Starts the worker thread to gather data

    this->ui->fileTree->setModel(file_model);
    this->ui->fileTree->setRootIndex(file_model->index(project_dir.path()));


    file_model->setNameFilters(
        QStringList()
        << "*.tl" << "*.json" << "*.txt" << "*.md"
    );
    file_model->setNameFilterDisables(false);

    ui->fileTree->header()->setStretchLastSection(true);
    ui->fileTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
}

void EditorPage::OnFileOpened(const QModelIndex& index)
{
    const QString path = QFileInfo(file_model->filePath(index)).canonicalFilePath();

    if(const auto it = openFiles.find(path); it != openFiles.end())
    {
        // if they clicked on a file that already has a tab, just open that tab
        ui->editorTabs->setCurrentWidget(it.value());
        return;
    }

    auto* editor = new CodeEditor(path);
    const int tab_index = ui->editorTabs->addTab(editor, file_model->fileName(index));

    ui->editorTabs->setCurrentIndex(tab_index);
    openFiles.insert(path, editor);
}

void EditorPage::OnTabClosed(const int index)
{
    QWidget* editor = ui->editorTabs->widget(index);

    const auto code_editor = qobject_cast<CodeEditor*>(editor);
    openFiles.remove(code_editor->file_path);

    ui->editorTabs->removeTab(index);
    delete editor;
}

void EditorPage::ContributeMenus(MenuRegistry& registry)
{
    registry.AddAction("file", "close", close_action);
}
