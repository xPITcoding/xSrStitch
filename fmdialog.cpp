#include "fmdialog.h"
#include "ui_fmdialog.h"

FMdialog::FMdialog(QList<QList<QImage>*>* in,QStringList names, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FMdialog)
{
    ui->setupUi(this);
    setFixedSize(755,570);

    for(int i=0; i<names.count()-1; i++)
    {
        ui->pSubStackCB->addItem(names[i]);
    }

    _list=in;
    _names=names;

    connect(ui->pSubStackCB,SIGNAL(currentIndexChanged(int)),this,SLOT(displayImgs(int)));

    showImgs(_list->at(0),0);




}


FMdialog::~FMdialog()
{
    delete ui;
}

void FMdialog::showImgs(QList<QImage>* list, int pos)
{
    QPixmap pix1;
    pix1.convertFromImage(list->at(0));
    pix1=pix1.scaledToWidth(ui->pLbl_1->width());
    ui->pLbl_1->setPixmap(pix1);
    ui->ptLbl_1->setText("Stack ref: "+_names[pos]);

    QPixmap pix2;
    pix2.convertFromImage(list->at(1));
    pix2=pix2.scaledToWidth(ui->pLbl_2->width());
    ui->pLbl_2->setPixmap(pix2);
    ui->ptLbl_2->setText("Stack match: "+_names[pos+1]);

    QPixmap pix3;
    pix3.convertFromImage(list->at(2));
    pix3=pix3.scaledToWidth(ui->pLbl_3->width());
    ui->pLbl_3->setPixmap(pix3);
    ui->ptLbl_3->setText("Overlay");
}

void FMdialog::displayImgs(int index)
{
    showImgs(_list->at(index),index);
}
