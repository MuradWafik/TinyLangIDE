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

}


MainWindow::~MainWindow()
{
    delete ui;
}
