#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setMenuBar(ui->menubar);

    // TODO: Toggleable in settings
    this->statusBar()->setVisible(false);
    this->setCentralWidget(this->ui->stackedWidget);

    const auto* home = this->ui->homePage;
    connect(home, &HomePage::ProjectOpened, this, &MainWindow::OpenProject);

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::OpenProject(QDir dir)
{
    this->ui->editorPage->Initialize(dir);
    SetCurrentPage(this->ui->editorPage);
    project_dir = std::move(dir);

}


void MainWindow::SetCurrentPage(QWidget* page)
{
    ui->stackedWidget->setCurrentWidget(page);

    MenuRegistry registry(ui->menubar);

    registry.Clear();

    default_menu_provider.ContributeMenus(registry);

    if(auto* provider = dynamic_cast<IMenuProvider*>(page);
        provider != nullptr)
    {
        provider->ContributeMenus(registry);
    }
}
