#pragma once

#include <QTreeView>


// basically the same as the tree view but custom context menu to decouple from the editor
class FileTree final : public QTreeView
{
    Q_OBJECT

public:
    explicit FileTree(QWidget* parent = nullptr);
    void setModel(QAbstractItemModel* model) override;

signals:
    void FileOpenRequested(const QString& path);
    void FileRenamed(const QString& old_path, const QString& new_path); // update the tracker of the open pages in the editor
    void FileDeleted(const QString& path);

protected:
    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    void ShowContextMenu(const QPoint& position);

    void NewFile(const QModelIndex& parent);
    void NewFolder(const QModelIndex& parent);
    void Rename(const QModelIndex& index);
    void Delete(const QModelIndex& index);

    QString rename_path;
};