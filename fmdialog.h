#ifndef FMDIALOG_H
#define FMDIALOG_H

#include <QDialog>

namespace Ui {
class FMdialog;
}

class FMdialog : public QDialog
{
    Q_OBJECT

public:
    explicit FMdialog(QList<QList<QImage>*>*,QStringList, QWidget *parent = nullptr);
    ~FMdialog();

   void showImgs(QList<QImage>* list, int pos);

private slots:
    void displayImgs(int);

private:
    Ui::FMdialog *ui;
    QList<QList<QImage>*>* _list=nullptr;
    QStringList _names;
};

#endif // FMDIALOG_H
