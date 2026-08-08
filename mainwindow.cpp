#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // TODO: Toggleable in settings
    this->statusBar()->setVisible(false);
    this->setCentralWidget(this->ui->stackedWidget);

    const auto* home = this->ui->homePage;
    connect(home, &HomePage::ProjectOpened, this, OpenProject);

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::OpenProject(const QDir& dir) const
{
    this->ui->stackedWidget->setCurrentWidget(this->ui->editorPage);
}
