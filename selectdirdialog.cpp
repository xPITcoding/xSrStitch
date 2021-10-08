#include "selectdirdialog.h"
#include "ui_selectdirdialog.h"
#include <QMessageBox>


selectDirDialog::selectDirDialog(QList<QFileInfoList>* dirList, QString root, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::selectDirDialog)
{
    ui->setupUi(this);
    setFixedSize(300,350);


    _dirList=dirList;
    _root=root;

    fill();

    QString order("Select the order of the files inside the folder / the direction in which the VOX-file is read");
    ui->pCBorder->setToolTip(order);
    ui->pLorder->setToolTip(order);
}

selectDirDialog::~selectDirDialog()
{
    delete ui;
}

void selectDirDialog::fill()
{
    for(int i=0; i<_dirList->count(); i++)
    {
        QListWidgetItem* pItem = new QListWidgetItem(_dirList->at(i).at(0).path().remove(_root),ui->listWidget);
        pItem->setFlags(Qt:: ItemIsSelectable| Qt::ItemIsDropEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        pItem->setCheckState(Qt::Checked);
        pItem->setData(Qt::UserRole,QVariant(i));
    }
}

void selectDirDialog::reject()
{
    accept();
}

void selectDirDialog::accept()
{
    _resultList = new QList<QFileInfoList>;
    QList<QListWidgetItem*> list=ui->listWidget->findItems("",Qt::MatchContains);

    for(QList<QListWidgetItem*>::iterator it=list.begin(); it!=list.end(); ++it)
    {
        if((*it)->checkState()==Qt::Checked)
        {
            _resultList->append(_dirList->at((*it)->data(Qt::UserRole).toInt()));
        }
    }
    if (_resultList->count()<2)
    {
        QMessageBox::warning(this,"Warning!","select atleast two substacks!");
        return;
    }

    if(ui->pCBorder->currentText()=="reverse")
        _resultList=reverseFileNames(_resultList);

    QDialog::accept();
}

QList<QFileInfoList>* selectDirDialog::reverseFileNames(QList<QFileInfoList>* _list)
{
     _resultList = new QList<QFileInfoList>;
    QFileInfoList temp;
    for (int i=0;i<_list->count();i++)
    {
        for(int j=0;j<_list->at(i).count();j++)
        {
            temp.append(_list->at(i).at(_list->at(i).count()-1-j));
        }
        _resultList->append(temp);
        temp.clear();
    }
    return _resultList;
}

void selectDirDialog::checkReverse(bool& reverse)
{
    reverse=ui->pCBorder->currentText()=="reverse";
}

