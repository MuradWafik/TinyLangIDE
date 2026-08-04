#pragma once

#include <QWidget>
#include <QPlainTextEdit>

QT_BEGIN_NAMESPACE
namespace Ui {
class EditorPage;
}
QT_END_NAMESPACE


class EditorPage : public QWidget
{
    Q_OBJECT

public:
    explicit EditorPage(QWidget *parent = nullptr);
    ~EditorPage();

private:
    Ui::EditorPage *ui;
};


