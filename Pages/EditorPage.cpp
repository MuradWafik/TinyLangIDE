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
    , clean_action{MakeAction("&Clean", {}, this)}
{
    ui->setupUi(this);

    // cant afaik set in property editor
    ui->mainSplitter->setSizes({220, 670});
    ui->editorSplitter->setSizes({500, 100});

    for(int i = 1; i < file_model->columnCount(); ++i) ui->fileTree->hideColumn(i);

    ConnectSignals();
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
    if(file_model->isDir(index)) return;

    OpenFile(file_model->filePath(index));
}

void EditorPage::OnTabClosed(int index)
{
    if(index < 0) index = ui->editorTabs->currentIndex();
    if(index < 0) return;

    const auto code_editor = GetOpenEditor();

    openFiles.remove(code_editor->file_path.canonicalPath());
    this->ui->problemsPage->RemoveFile(code_editor);


    const QWidget* editor = ui->editorTabs->widget(index);
    ui->editorTabs->removeTab(index);
    delete editor;
}

void EditorPage::OnSaveAs()
{
    auto* editor = GetOpenEditor();
    if(!editor) return;

    const QString path = QFileDialog::getSaveFileName(
        this, tr("Save As"), editor->file_path.path(),
        tr("TinyLang Files (*.tl);;Text Files (*.txt);;All Files (*)")
    );

    if(path.isEmpty()) return;

    const QString canonical_path = QFileInfo(path).canonicalFilePath();

    if(openFiles.contains(canonical_path) &&
        openFiles.value(canonical_path) != editor)
    {
        QMessageBox::warning(this, tr("File Already Open"), tr("That file is already open in another tab."));
        return;
    }

    const QString old_path = editor->file_path.canonicalPath();
    if(const auto result = editor->SaveAs(canonical_path); !result)
    {
        QMessageBox::warning(this, tr("Error Saving"), result.error());
        return;
    }

    openFiles.remove(old_path);
    openFiles.insert(canonical_path, editor);

    ui->editorTabs->setTabText(
        ui->editorTabs->indexOf(editor),
        QFileInfo(canonical_path).fileName()
    );
}


void EditorPage::OnFileSaved()
{
    const auto* editor = GetOpenEditor();
    if(!editor) return;

    if(const auto save = editor->Save(); !save)
    {
        QMessageBox::warning(this, tr("Error saving"), save.error());
    }
}

void EditorPage::OnBuild()
{
    const auto build =
        TinyLangUtils::BuildProject(project_dir, QDir(project_dir.filePath(build_dir)));
    if(!build) QMessageBox::warning(this, tr("Error Building Project"), build.error());
}

void EditorPage::OnClean() const
{
    QDir build = project_dir.filePath(build_dir);
    build.removeRecursively();
}

void EditorPage::ConnectProcessSignals()
{
    connect(running_process, &QProcess::readyReadStandardOutput, this, [this]
    {
        ui->output->setTextColor(Qt::white);
        ui->output->append(running_process->readAllStandardOutput());
    });

    connect(running_process, &QProcess::readyReadStandardError, this, [this]
    {
        ui->output->setTextColor(Qt::white);
        ui->output->append(running_process->readAllStandardError());
    });

    connect(
        running_process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
        [this](const int exitCode, const QProcess::ExitStatus status)
        {
            ui->output->setTextColor(Qt::darkGray);
            ui->output->append(QStringLiteral("Process finished with exit code %1").arg(exitCode));
            running_process->deleteLater();
            running_process = nullptr;
        }
    );
}

CodeEditor* EditorPage::GetOpenEditor() const
{
    const int index = ui->editorTabs->currentIndex();
    if(index < 0) return nullptr;

    QWidget* editor = ui->editorTabs->widget(index);
    return qobject_cast<CodeEditor*>(editor);
}

CodeEditor* EditorPage::OpenFile(const QString& path)
{
    const QString canonical_path = QFileInfo(path).canonicalFilePath();

    if(const auto it = openFiles.find(canonical_path);
       it != openFiles.end())
    {
        ui->editorTabs->setCurrentWidget(it.value());
        return dynamic_cast<CodeEditor*>(it.value());
    }

    auto* editor = new CodeEditor(canonical_path);

    connect(
        editor,
        &CodeEditor::OnFileLinted,
        this,
        &EditorPage::OnFileLinted
    );

    const int tab_index = ui->editorTabs->addTab(
        editor,
        QFileInfo(canonical_path).fileName()
    );

    ui->editorTabs->setCurrentIndex(tab_index);

    openFiles.insert(canonical_path, editor);

    return editor;
}

void EditorPage::OnRun()
{
    if(running_process) return;

    // the page takes ownership of the process (owns the pointer and is set as the parent) thus can end if page is changed
    auto run = TinyLangUtils::RunProject(this, project_dir);
    if(!run)
    {
        QMessageBox::warning(this, tr("Error Running Project"), run.error());
        return;
    }

    this->ui->outputTabs->setCurrentWidget(this->ui->outputPage);
    this->ui->output->clear(); // when they rerun, clear the output text

    running_process = run.value();
    ConnectProcessSignals();
}

void EditorPage::OnFileLinted(CodeEditor* editor, const QVector<TinyLangUtils::LintItem>& result) const
{
    // TODO: symbol for warning and error
    auto* problems = this->ui->problemsPage;
    problems->SetFileDiagnostics(editor, result);
}

void EditorPage::OnProblemClicked(CodeEditor* editor, const int line, const int column)
{
    ui->editorTabs->setCurrentWidget(editor);
    editor->SetCursorPosition(line, column);
    editor->setFocus();
}

void EditorPage::ContributeMenus(MenuRegistry& registry)
{
    registry.AddMenu("build", "Build");
    registry.AddAction("file", "close", close_action);
    registry.AddAction("file", "save", save_action);
    registry.AddAction("file", "save_as", save_as_action);
    registry.AddAction("build", "build", build_action);
    registry.AddAction("build", "run", run_action);
    registry.AddAction("build", "clean", clean_action);
}

void EditorPage::ConnectSignals()
{
    connect(ui->fileTree, &QTreeView::clicked, this, &EditorPage::OnFileOpened);

    connect(ui->editorTabs, &QTabWidget::tabCloseRequested, this, &EditorPage::OnTabClosed);
    connect(close_action, &QAction::triggered, this, [this] { OnTabClosed(); });

    connect(save_action, &QAction::triggered, this, &EditorPage::OnFileSaved);
    connect(save_as_action, &QAction::triggered, this, &EditorPage::OnSaveAs);
    connect(clean_action, &QAction::triggered, this, &EditorPage::OnClean);

    connect(build_action, &QAction::triggered, this, &EditorPage::OnBuild);
    ui->buildButton->setDefaultAction(build_action);

    connect(run_action, &QAction::triggered, this, &EditorPage::OnRun);
    ui->runButton->setDefaultAction(run_action);

    connect(ui->problemsPage, &Problems::DiagnosticClicked, this, &EditorPage::OnProblemClicked);

}