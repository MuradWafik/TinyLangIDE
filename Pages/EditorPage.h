#pragma once

#include <expected>
#include <QFileSystemModel>

#include "CodeEditor.h"
#include "IMenuProvider.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class EditorPage;
}
QT_END_NAMESPACE


class EditorPage final : public QWidget, public IMenuProvider
{
    Q_OBJECT

public:
    explicit EditorPage(QWidget* parent = nullptr);
    ~EditorPage() override;

    void Initialize(const QDir& dir);
    void ContributeMenus(MenuRegistry& registry) override;

    QDir project_dir;
private:
    Ui::EditorPage* ui;
    QFileSystemModel* file_model;

    QAction* close_action;
    QAction* save_action;
    QAction* save_as_action;
    QAction* build_action;
    QAction* run_action;
    QAction* clean_action;

    void OnFileOpened(const QModelIndex& index);
    void OnTabClosed(int index = -1);
    void OnSaveAs();
    void OnFileSaved();
    void OnBuild();
    void OnClean() const;
    void OnRun();
    void OnFileLinted(CodeEditor* editor, const QVector<TinyLangUtils::LintItem>& result) const;

    // open the file at the line/col
    void OnProblemClicked(CodeEditor* editor, int line, int column);


    void ConnectSignals();
    void ConnectProcessSignals();

    [[nodiscard]] CodeEditor* GetOpenEditor() const;
    CodeEditor* OpenFile(const QString& path);


    QHash<QString, QWidget*> openFiles;
    // canonical path -> tab, to prevent multiple tabs for the same file, just opens its tab
    // cant use index as on removal would have to shift all

    QProcess* running_process = nullptr; // the qprocess (possibly null) when something is running
    constexpr static auto build_dir = "build";
};
