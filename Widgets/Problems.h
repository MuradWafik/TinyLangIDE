#ifndef PROBLEMS_H
#define PROBLEMS_H

#include <QTreeWidget>
#include <QWidget>

#include "CodeEditor.h"
#include "TinyLangUtils.h"


QT_BEGIN_NAMESPACE
namespace Ui { class Problems; }
QT_END_NAMESPACE

class Problems final : public QWidget {
Q_OBJECT

public:
    explicit Problems(QWidget *parent = nullptr);
    ~Problems() override;

    void SetDiagnostics(const QVector<TinyLangUtils::LintItem>& items) const;
    void ClearDiagnostics() const;

signals:
    void DiagnosticClicked(const QString& file_path, int line, int column);

private:
    Ui::Problems *ui;
    void OnItemClicked(QTreeWidgetItem *item, int column);

};

#endif //PROBLEMS_H
