#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <qlistwidget.h>

QT_BEGIN_NAMESPACE
namespace Ui { class Dialog; }
QT_END_NAMESPACE

class Dialog : public QDialog
{
    Q_OBJECT

public:
    Dialog(QWidget *parent = nullptr);
    ~Dialog();

private slots:
    void on_allProcesses_itemClicked(QListWidgetItem *item);

    void on_processToKill_itemClicked(QListWidgetItem *item);

    void on_killSelectedButton_clicked();

    void on_killSameNameButton_clicked();

    void on_searchButton_clicked();

    void on_clearButton_clicked();

    void on_killNoReopenButton_clicked();

private:
    Ui::Dialog *ui;
};
#endif // DIALOG_H
