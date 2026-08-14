#include "EditorPage.h"

#include <QFileDialog>

#include "ui_EditorPage.h"

#include <QFileSystemModel>
#include <QMessageBox>

#include "CodeEditor.h"
#include "TinyLangKnowledge/TinyLangUtils.h"


EditorPage::EditorPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::EditorPage)
    , file_model{new QFileSystemModel(this)}
    , close_action(new QAction("&Close", this))
    , save_action{MakeAction("&Save", QKeySequence::Save, this)}
    , save_as_action{MakeAction("Save &As...", QKeySequence::SaveAs, this)}
    , build_action{MakeAction("&Build", QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_B), this)}
    , run_action{MakeAction("&Run", QKeySequence(Qt::Key_F5), this)}
{
    ui->setupUi(this);

    // cant afaik set in property editor
    ui->mainSplitter->setSizes({220, 670});
    ui->editorSplitter->setSizes({500, 100});

    for(int i = 1; i < file_model->columnCount(); ++i) ui->fileTree->hideColumn(i);

    connect(ui->fileTree, &QTreeView::clicked, this, &EditorPage::OnFileOpened);

    connect(ui->editorTabs, &QTabWidget::tabCloseRequested, this, &EditorPage::OnTabClosed);
    connect(close_action, &QAction::triggered, this, [this]() { OnTabClosed(); });

    connect(save_action, &QAction::triggered, this, &EditorPage::OnFileSaved);
    connect(save_as_action, &QAction::triggered, this, &EditorPage::OnSaveAs);

    connect(build_action, &QAction::triggered, this, &EditorPage::OnBuild);
    ui->buildButton->setDefaultAction(build_action);

    connect(run_action, &QAction::triggered, this, &EditorPage::OnRun);
    ui->runButton->setDefaultAction(run_action)
}


EditorPage::~EditorPage()
{
    delete ui;
}

void EditorPage::Initialize(const QDir& dir)
{
    this->project_dir = dir;

    file_model->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);

    file_model->setNameFilters({
        "*.tl",
        "*.json",
        "*.txt",
        "*.md"
    });

    file_model->setNameFilterDisables(false);

    file_model->setRootPath(project_dir.path()); // Starts the worker thread to gather data

    this->ui->fileTree->setModel(file_model);
    this->ui->fileTree->setRootIndex(file_model->index(project_dir.path()));
}

void EditorPage::OnFileOpened(const QModelIndex& index)
{
    if(file_model->isDir(index)) return; // dont try to open directories in tabs

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

void EditorPage::OnTabClosed(int index)
{
    if(index < 0) index = ui->editorTabs->currentIndex();
    if(index < 0) return;

    QWidget* editor = ui->editorTabs->widget(index);
    if(const auto code_editor = qobject_cast<CodeEditor*>(editor))
    {
        openFiles.remove(code_editor->file_path);
    }

    ui->editorTabs->removeTab(index);
    delete editor;
}

void EditorPage::OnSaveAs()
{
    const QString file_path = QFileDialog::getSaveFileName(
        this, tr("Save As"), "", tr("TinyLang Files (*.txt);;All Files (*)")
    );

    if(file_path.isEmpty())
    {
        QMessageBox::warning(this, tr("No File provided"), tr("No file provided when saving as"));
        return;
    }

    const int index = ui->editorTabs->currentIndex();
    if(index < 0) return;

    QWidget* editor = ui->editorTabs->widget(index);

    const auto code_editor = qobject_cast<CodeEditor*>(editor);
    if(auto save = SaveFile(code_editor->document(), file_path);
        !save)
    {
        QMessageBox::warning(this, tr("Error Saving"), save.error());
    }
}

std::expected<void, QString> EditorPage::SaveFile(const QTextDocument* doc, const QString& path) const
{
    QFile file(path);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        return std::unexpected(tr("Unable to open file %1").arg(path));
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    QTextBlock block = doc->begin();
    while(block.isValid())
    {
        out << block.text();

        block = block.next();
        if (block.isValid())
        {
            out << line_ending;
        }
    }
    return {};
}

void EditorPage::OnFileSaved()
{
    const int index = ui->editorTabs->currentIndex();
    if(index < 0) return;

    QWidget* editor = ui->editorTabs->widget(index);

    const auto code_editor = qobject_cast<CodeEditor*>(editor);
    if(const auto save = SaveFile(code_editor->document(), code_editor->file_path);
        !save)
    {
        QMessageBox::warning(this, tr("Error saving"), save.error());
    }
}

void EditorPage::OnBuild()
{
    const auto build =
        TinyLangUtils::BuildProject(project_dir, QDir(project_dir.filePath("build")));
    if(!build) QMessageBox::warning(this, tr("Error Building Project"), build.error());
}

void EditorPage::OnRun()
{
    // the page takes ownership of the process (owns the pointer and is set as the parent) thus can end if page is changed
    const auto run = TinyLangUtils::RunProject(this, project_dir);
    if(!run) QMessageBox::warning(this, tr("Error Building Project"), run.error());


}

void EditorPage::ContributeMenus(MenuRegistry& registry)
{
    registry.AddMenu("build", "Build");
    registry.AddAction("file", "close", close_action);
    registry.AddAction("file", "save", save_action);
    registry.AddAction("file", "save_as", save_as_action);
    registry.AddAction("build", "build", build_action);
    registry.AddAction("build", "run", run_action);
}
