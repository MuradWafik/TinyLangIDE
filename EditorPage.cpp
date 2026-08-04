#include "EditorPage.h"
#include "ui_EditorPage.h"

#include <QPainter>
#include <QTextBlock>


EditorPage::EditorPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::EditorPage)
{
    ui->setupUi(this);
}


EditorPage::~EditorPage()
{
    delete ui;
}