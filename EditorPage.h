#pragma once

#include <QFileSystemModel>
#include <QWidget>
#include <QPlainTextEdit>

QT_BEGIN_NAMESPACE
namespace Ui {
class EditorPage;
}
QT_END_NAMESPACE


class EditorPage final : public QWidget
{
    Q_OBJECT

public:
    explicit EditorPage(QWidget *parent = nullptr);
    ~EditorPage() override;

    void Initialize(const QDir* dir);

    const QDir* project_dir; // non-owning

private:
    Ui::EditorPage *ui;
    QFileSystemModel* file_model;

    void OnFileOpened(const QModelIndex &index) const;
    void OnTabClosed(int index) const;
};


