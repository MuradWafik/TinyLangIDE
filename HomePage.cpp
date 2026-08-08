#include "HomePage.h"
#include "ui_HomePage.h"

#include <QDebug>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>

#include "TinyLangUtils.h"


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


    const QDir project_dir{dir_path};
    const QString config_file = project_dir.filePath("tinylang.json");
    if(!QFileInfo::exists(config_file))
    {
        QMessageBox::warning(
            this,
            tr("Warning"),
            tr("No tinylang.json found. Try creating a project instead."));
        return;
    }

    // TODO: Emit a signal or open the project in the editor here

    emit ProjectOpened(project_dir);
    qDebug() << "Successfully opened project at:" << dir_path;
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

    if (!ok || projectName.trimmed().isEmpty()) return;


    const QString project_dir = QFileDialog::getExistingDirectory(
        this, tr("Select Parent Directory for Project"), QDir::homePath()
    );

    if (project_dir.isEmpty()) return; // User cancelled the directory choice

    if(auto result = TinyLangUtils::NewProject(projectName, project_dir);
        !result)
    {
        QMessageBox::critical(
            this, tr("Project Creation Failed"),
            tr("Could not create project:\n\n%1").arg(result.error())
        );
        return;
    }

    const QString full_project_path = QDir(project_dir).filePath(projectName);
    QMessageBox::information(
        this, tr("Success"),
        tr("Project '%1' created successfully!").arg(projectName)
    );

    emit ProjectOpened(full_project_path);
}
