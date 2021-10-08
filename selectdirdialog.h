#ifndef SELECTDIRDIALOG_H
#define SELECTDIRDIALOG_H

#include <QDialog>
#include <QFileInfoList>

namespace Ui {
class selectDirDialog;
}

class selectDirDialog : public QDialog
{
    Q_OBJECT

public:
    explicit selectDirDialog(QList<QFileInfoList>*,QString, QWidget *parent = nullptr);
    ~selectDirDialog();

    void fill();

    void accept() override;
    void reject() override;

    QList<QFileInfoList>* getList(){return _resultList;}
    QList<QFileInfoList>* reverseFileNames(QList<QFileInfoList>* _list);
    void checkReverse(bool& reverse);

private:
    Ui::selectDirDialog *ui;

    QList<QFileInfoList>* _dirList;
    QList<QFileInfoList>* _resultList;
    QString _root;
};

#endif // SELECTDIRDIALOG_H
