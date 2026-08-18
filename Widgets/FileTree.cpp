#include "FileTree.h"

#include <QFileDialog>
#include <QFileSystemModel>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>

FileTree::FileTree(QWidget* parent)
    : QTreeView(parent)
{
    connect(this, &QTreeView::doubleClicked, this, [this](const QModelIndex& index)
        {
            const auto* model = qobject_cast<QFileSystemModel*>(this->model());
            if(model && index.isValid() && !model->isDir(index)) emit FileOpenRequested(model->filePath(index));
        }
    );
}

void FileTree::contextMenuEvent(QContextMenuEvent* event)
{
    ShowContextMenu(event->pos());
}

void FileTree::ShowContextMenu(const QPoint& position)
{
    const QModelIndex index = indexAt(position);

    const auto* model = qobject_cast<QFileSystemModel*>(this->model());
    if(!model) return;


    QMenu menu(this);
    QModelIndex parent = index;

    if(!parent.isValid()) parent = rootIndex();
    else if(!model->isDir(parent)) parent = parent.parent();

    const QAction* new_file = menu.addAction(tr("New File"));
    const QAction* new_folder = menu.addAction(tr("New Folder"));

    menu.addSeparator();

    const QAction* rename = nullptr;
    const QAction* remove = nullptr;
    const QAction* open = nullptr;
    if(index.isValid())
    {
        open = menu.addAction(tr("Open"));
        rename = menu.addAction(tr("Rename"));
        remove = menu.addAction(tr("Delete"));
    }

    // ReSharper disable once CppTooWideScopeInitStatement
    const QAction* selected = menu.exec(viewport()->mapToGlobal(position));

    if(selected == new_file) NewFile(parent);
    else if(selected == new_folder) NewFolder(parent);
    else if(selected == open) emit FileOpenRequested(model->filePath(index));
    else if(selected == rename) Rename(index);
    else if(selected == remove) Delete(index);
}

void FileTree::NewFile(const QModelIndex& parent)
{
    const auto* model = qobject_cast<QFileSystemModel*>(this->model());
    if(!model) return;

    const QString directory = model->filePath(parent);

    QString name = QInputDialog::getText(this, tr("New File"), tr("File name:")).trimmed();
    if(name.isEmpty()) return;
    if(QFileInfo(name).suffix().isEmpty()) name += ".tl";

    const QString path = QDir(directory).filePath(name);

    if(QFileInfo::exists(path))
    {
        QMessageBox::warning(this, tr("File Already Exists"), tr("A file with that name already exists."));
        return;
    }

    QFile file(path);
    if(!file.open(QIODevice::WriteOnly))
    {
        QMessageBox::warning(this, tr("Error"), tr("Could not create the file."));
        return;
    }
    file.close();
}

void FileTree::NewFolder(const QModelIndex& parent)
{
    auto* model = qobject_cast<QFileSystemModel*>(this->model());
    if(!model) return;

    const QString name = QInputDialog::getText(
        this,
        tr("New Folder"),
        tr("Folder name:")
    );

    if(name.isEmpty()) return;
    model->mkdir(parent, name);
}

void FileTree::Rename(const QModelIndex& index)
{
    if(!index.isValid()) return;

    const auto* model = qobject_cast<QFileSystemModel*>(this->model());
    if(!model) return;

    rename_path = model->filePath(index);
    edit(index);
}

void FileTree::Delete(const QModelIndex& index)
{
    const auto* model = qobject_cast<QFileSystemModel*>(this->model());
    if(!model || !index.isValid()) return;

    const QString path = model->filePath(index);

    const auto result = QMessageBox::question(
        this,
        tr("Delete"),
        tr("Delete \"%1\"?").arg(QFileInfo(path).fileName()),
        QMessageBox::Yes | QMessageBox::No
    );

    if(result != QMessageBox::Yes) return;

    if(model->isDir(index)) QDir(path).removeRecursively();
    else QFile::remove(path);

    emit FileDeleted(path);
}

void FileTree::setModel(QAbstractItemModel* model)
{
    if(this->model()) disconnect(this->model(), nullptr, this, nullptr);

    QTreeView::setModel(model);

    auto* file_model = qobject_cast<QFileSystemModel*>(model);
    if(!file_model) return;

    connect(
        file_model,
        &QAbstractItemModel::dataChanged,
        this,
        [this, file_model](
            const QModelIndex& top_left,
            const QModelIndex& bottom_right,
            const QList<int>& roles)
        {
            if(rename_path.isEmpty()) return;
            if(!roles.isEmpty() && !roles.contains(Qt::DisplayRole)) return;
            if(top_left != bottom_right) return;

            const QString new_path = file_model->filePath(top_left);
            if(new_path != rename_path) emit FileRenamed(rename_path, new_path);
            rename_path.clear();
        }
    );
}