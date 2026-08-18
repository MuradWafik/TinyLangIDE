#include "EditorPage.h"

#include <QFileDialog>

#include "ui_EditorPage.h"

#include <QFileSystemModel>
#include <QMessageBox>
#include <QTimer>

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
    , project_lint_timer{new QTimer(this)}
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

    file_model->setReadOnly(false);
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
    RunProjectLint();
}

void EditorPage::OnTabClosed(int index)
{
    if(index < 0) index = ui->editorTabs->currentIndex();
    if(index < 0) return;

    const auto* editor = qobject_cast<CodeEditor*>(ui->editorTabs->widget(index));
    if(!editor) return;

    if(editor->document()->isModified())
    {
        const auto btn = QMessageBox::question(
            this,
            tr("Save Changes"),
            tr("Do you want to save changes to '%1' before closing?").arg(editor->file_path.fileName()),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
            QMessageBox::Save
        );

        if(btn == QMessageBox::Cancel) return;

        if(btn == QMessageBox::Save)
        {
            if(const auto result = editor->Save(); !result)
            {
                QMessageBox::warning(this, tr("Error Saving"), result.error());
                return;
            }
        }
    }

    openFiles.remove(editor->file_path.canonicalPath());
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

    const int idx = ui->editorTabs->indexOf(editor);
    if(idx >= 0)
    {
        QString title = QFileInfo(canonical_path).fileName();
        if(editor->document()->isModified()) title += " *";
        ui->editorTabs->setTabText(idx, title);
    }
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

bool EditorPage::SaveAllModifiedFiles()
{
    for(int i = 0; i < ui->editorTabs->count(); ++i)
    {
        auto* editor = qobject_cast<CodeEditor*>(ui->editorTabs->widget(i));
        if(editor && editor->document()->isModified())
        {
            if(const auto result = editor->Save(); !result)
            {
                QMessageBox::warning(this, tr("Error Saving File"), result.error());
                return false;
            }
        }
    }
    return true;
}

void EditorPage::OnBuild()
{
    if(save_before_run && !SaveAllModifiedFiles()) return;

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

    connect(editor, &CodeEditor::textChanged, this, &EditorPage::ScheduleProjectLint);
    connect(
        editor->document(), &QTextDocument::modificationChanged,
        this, [this, editor](const bool modified)
        {
            if(const int idx = ui->editorTabs->indexOf(editor);
                idx >= 0)
            {
                QString title = editor->file_path.fileName();
                if(modified) title += " *";
                ui->editorTabs->setTabText(idx, title);
            }
        }
    );

    const int tab_index = ui->editorTabs->addTab(
        editor,
        QFileInfo(canonical_path).fileName()
    );

    ui->editorTabs->setCurrentIndex(tab_index);

    openFiles.insert(canonical_path, editor);

    return editor;
}

void EditorPage::ScheduleProjectLint() const
{
    project_lint_timer->start(run_lint_time_ms);
}

void EditorPage::RunProjectLint() const
{
    project_lint_timer->stop();
    if(project_dir.path().isEmpty()) return;

    if(const auto res = TinyLangUtils::CheckProject(project_dir.path()))
    {
        ui->problemsPage->SetDiagnostics(res.value());
    }
}

void EditorPage::OnRun()
{
    if(running_process) return;
    if(save_before_run && !SaveAllModifiedFiles()) return;

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



void EditorPage::OnProblemClicked(const QString& file_path, const int line, const int column)
{
    if(CodeEditor* editor = OpenFile(file_path))
    {
        editor->SetCursorPosition(line, column);
        ui->problemsPage->clearFocus();
        // when called right away nothing happens
        QTimer::singleShot(150, editor, [editor]
        {
            editor->setFocus();
            editor->activateWindow();
        });
    }
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
    connect(project_lint_timer, &QTimer::timeout, this, &EditorPage::RunProjectLint);
    connect(ui->fileTree, &FileTree::FileOpenRequested, this,
        [this](const QString& path)
        {
            OpenFile(path);
        }
    );

    connect(ui->fileTree, &FileTree::FileRenamed, this, &EditorPage::OnFileRenamed);
    connect(ui->fileTree, &FileTree::FileDeleted, this, &EditorPage::OnFileDeleted);

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

void EditorPage::OnFileRenamed(const QString& old_path, const QString& new_path)
{
    const QString old_canonical = QFileInfo(old_path).canonicalFilePath();
    const QString new_canonical = QFileInfo(new_path).canonicalFilePath();

    const auto it = openFiles.find(old_canonical);
    if(it == openFiles.end()) return;

    const auto editor = qobject_cast<CodeEditor*>(it.value());
    openFiles.erase(it);
    openFiles.insert(new_canonical, editor);

    editor->file_path = QFileInfo(new_canonical);

    if(const int tabIndex = ui->editorTabs->indexOf(editor);
        tabIndex >= 0)
    {
        QString title = QFileInfo(new_canonical).fileName();
        if(editor->document()->isModified()) title += " *";
        ui->editorTabs->setTabText(tabIndex, title);
    }
}

void EditorPage::OnFileDeleted(const QString& path)
{
    const QString canonical_path = QFileInfo(path).canonicalFilePath();
    if(const auto it = openFiles.find(canonical_path); it != openFiles.end())
    {
        if(const auto* editor = qobject_cast<CodeEditor*>(it.value()))
        {
            editor->document()->setModified(false);
            if(const int idx = ui->editorTabs->indexOf(editor);
                idx >= 0)
            {
                openFiles.erase(it);
                ui->editorTabs->removeTab(idx);
                delete editor;
            }
        }
        return;
    }

    QList<QString> to_close;
    for(auto it = openFiles.constBegin(); it != openFiles.constEnd(); ++it)
    {
        if(it.key().startsWith(canonical_path + "/") || it.key() == canonical_path)
        {
            to_close.append(it.key());
        }
    }

    for(const auto& key : to_close)
    {
        if(const auto it = openFiles.find(key); it != openFiles.end())
        {
            if(const auto* editor = qobject_cast<CodeEditor*>(it.value()))
            {
                editor->document()->setModified(false);
                if(const int idx = ui->editorTabs->indexOf(editor);
                    idx >= 0)
                {
                    ui->editorTabs->removeTab(idx);
                    delete editor;
                }
            }
            openFiles.erase(it);
        }
    }
}
