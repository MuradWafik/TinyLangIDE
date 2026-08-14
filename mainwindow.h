#pragma once

#include <qdir.h>
#include <QMainWindow>

#include "IMenuProvider.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    void OpenProject(QDir dir);
    void SetCurrentPage(QWidget* page);

    Ui::MainWindow *ui;
    QDir project_dir;
    DefaultMenuProvider default_menu_provider;
};
