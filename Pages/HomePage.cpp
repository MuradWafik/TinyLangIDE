#include "HomePage.h"
#include "ui_HomePage.h"

#include <QDebug>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>

#include "TinyLangKnowledge/TinyLangUtils.h"


HomePage::HomePage(QWidget *parent) :
    QWidget(parent), ui(new Ui::HomePage)
{
    ui->setupUi(this);

    connect(ui->newProjectButton, &QPushButton::clicked, this, &HomePage::NewProject);
    connect(ui->openProjectButton, &QPushButton::clicked, this, &HomePage::OpenProject);
}

HomePage::~HomePage()
{
    delete ui;
}

void HomePage::OpenProject(bool)
{
    const auto dir_path = QFileDialog::getExistingDirectory(
        this, tr("Select Project Directory"), QDir::homePath()
    );

    if(dir_path.isEmpty()) return;

    auto info_res = TinyLangUtils::GetProjectInfo(dir_path);
    if(!info_res)
    {
        QMessageBox::warning(
            this,
            tr("Invalid Project"),
            info_res.error().isEmpty() ? tr("No valid TinyLang project found at the selected path.") : info_res.error()
        );
        return;
    }

    const QString root = info_res.value()["project_root"].toString();
    emit ProjectOpened(QDir(root.isEmpty() ? dir_path : root));
}

void HomePage::NewProject(bool)
{
    bool ok = false;
    const QString projectName = QInputDialog::getText(
        this,
        tr("New Project"),
        tr("Enter project name:"),
        QLineEdit::Normal, QString(), &ok
    );

    if(!ok || projectName.trimmed().isEmpty()) return;


    const QString project_dir = QFileDialog::getExistingDirectory(
        this, tr("Select Parent Directory for Project"), QDir::homePath()
    );

    if(project_dir.isEmpty()) return; // User cancelled the directory choice

    if(auto result = TinyLangUtils::NewProject(projectName, project_dir);
        !result)
    {
        QMessageBox::critical(
            this, tr("Project Creation Failed"),
            tr("Could not create project:\n\n%1").arg(result.error())
        );
        return;
    }

    QMessageBox::information(
        this, tr("Success"),
        tr("Project '%1' created successfully!").arg(projectName)
    );

    const QDir created_dir{QDir(project_dir).filePath(projectName)};
    emit ProjectOpened(created_dir);
}
