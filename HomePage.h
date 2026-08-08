#ifndef HOMEPAGE_H
#define HOMEPAGE_H

#include <qdir.h>
#include <QWidget>


QT_BEGIN_NAMESPACE
namespace Ui { class HomePage; }
QT_END_NAMESPACE

class HomePage final : public QWidget {
Q_OBJECT

public:
    explicit HomePage(QWidget *parent = nullptr);
    ~HomePage() override;

signals:
    void ProjectOpened(QDir path);

private slots:
    void NewProject(bool);
    void OpenProject(bool);

private:
    Ui::HomePage *ui;
};


#endif //HOMEPAGE_H
