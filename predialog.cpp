#include "predialog.h"
#include "ui_predialog.h"


#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsLineItem>
#include <QStringList>
#include <stdlib.h>
#include <QFileInfoList>

using namespace std;

Cursor::Cursor(const QString& name,float pos,QColor col,QGraphicsItem *pParent):QObject(),QGraphicsLineItem(0,0,0,20,pParent)
{
    _name=name;
    //pText= new QGraphicsSimpleTextItem(this);
   // pText->setPos(0,5);
    //pText->setPen(QPen(Qt::white));
    setPen(QPen(col,4));
    setFlags(QGraphicsItem::ItemIsMovable);
    setAcceptHoverEvents(true);
    setPos(pos,0);
}

void Cursor::mouseMoveEvent(QGraphicsSceneMouseEvent* me)
{
   QGraphicsLineItem::mouseMoveEvent(me);
   QGraphicsPixmapItem *pParentPix=dynamic_cast<QGraphicsPixmapItem*>(parentItem());
   QPointF p=pParentPix->mapFromScene(me->scenePos());
   if (p.x()<0)
       p.setX(0);
   if (pParentPix && p.x()>pParentPix->pixmap().width()-1)
       p.setX(pParentPix->pixmap().width()-1);
   p.setY(0);
   setPos(p);
   emit modified(this);
}

void Cursor::hoverEnterEvent(QGraphicsSceneHoverEvent* he)
{
    setCursor(QCursor(Qt::SizeHorCursor));
    QGraphicsLineItem::hoverEnterEvent(he);
}

void Cursor::hoverLeaveEvent(QGraphicsSceneHoverEvent* he)
{
    setCursor(QCursor(Qt::ArrowCursor));
    QGraphicsLineItem::hoverEnterEvent(he);
}

QImage preDialog::dispImage(QImage img)
{
    QImage res(img.width(),img.height(),QImage::Format_RGB888);
    float gval;
    int val;
    float hatval;
    int val2;
    int scaled=(float)img.width()*(float)ui->pSB_FMscale->value()/100.0f;
    int x=img.width()/2-scaled/2;
    int y=img.height()/2-scaled/2;
    QRect areaFm(x,y,scaled,scaled);
    QImage hat(":/imgs/FM_mask.png");
    hat=hat.copy(hat.width()/8,hat.height()/8,hat.width()-hat.width()/4,hat.height()-hat.height()/4);
    hat=hat.scaledToWidth(scaled);


    for (long ly=0;ly<img.height();++ly)
        for (long lx=0;lx<img.width();++lx)
        {
            gval=((unsigned short*)img.scanLine(ly))[lx];
            gval=(float)gval/65535.0f*(_scaleMax-_scaleMin)+_scaleMin;
            val=(gval-_min)/(_max-_min)*255.0f;
            val=max(0,val);
            val=min(255,val);

            if (gval>_bin)
                res.setPixel(lx,ly,qRgb(val,0,0));
            else if(!areaFm.contains(lx,ly))
            {

                res.setPixel(lx,ly,qRgb(0,val,0));
            }
            else
            {
                hatval=(float)qGray(hat.pixel(lx-x,ly-y))/255.0f;
                val2=(float)val*hatval;
                res.setPixel(lx,ly,qRgb(val2,val,val2));
            }
        }
    return res;
}

preDialog::preDialog(QList<QFileInfoList>* dirList, float _min, float _max, float _bin, datatype dt,int pos1, int max1, int pos2, int max2, int cbPos,VOXInfo* vInf, bool rev, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::preDialog)
{
    ui->setupUi(this);
    setFixedSize(1122,634);
    preDialog::setLocale(QLocale::English);
    _dt=dt;
    _dirList=dirList;

    _pos1=pos1;
    _pos2=pos2;
    _max1=max1;
    _max2=max2;
    _cbpos=cbPos;
    _vInfo=new VOXInfo;
    _vInfo=vInf;
    _rev=rev;



    ui->pSB_i1->setRange(0,max1);
    ui->pSB_i1->setValue(pos1);
    ui->pSB_i2->setRange(0,max2);
    ui->pSB_i2->setValue(pos2);
    ui->pSlider_i1->setRange(0,max1);
    ui->pSlider_i1->setValue(pos1);
    ui->pSlider_i2->setRange(0,max2);
    ui->pSlider_i2->setValue(pos2);

    ui->pSB_FMscale->setValue(67);



    QGraphicsScene* scene= new QGraphicsScene;
    ui->pSliderView->setScene(scene);
    ui->pSliderView->setRenderHints(QPainter::Antialiasing);
    QPixmap pix(1000,20);
    pix.fill(Qt::yellow);
    pPixItem=new QGraphicsPixmapItem(pix);
    scene->addItem(pPixItem);

    _scaleMin=_min;
    _scaleMax=_max;

    pMinCur = new Cursor("min",0,Qt::white,pPixItem);
    pMaxCur = new Cursor("max",999,Qt::white,pPixItem);
    pBinCur = new Cursor("bin",499,Qt::red,pPixItem);

    connect(pMinCur,SIGNAL(modified(Cursor*)),this,SLOT(cursorMoved(Cursor*)));
    connect(pMaxCur,SIGNAL(modified(Cursor*)),this,SLOT(cursorMoved(Cursor*)));
    connect(pBinCur,SIGNAL(modified(Cursor*)),this,SLOT(cursorMoved(Cursor*)));

    connect(ui->pSB_min,SIGNAL(editingFinished()),this,SLOT(setupSliderUI()));
    connect(ui->pSB_bin,SIGNAL(editingFinished()),this,SLOT(setupSliderUI()));
    connect(ui->pSB_max,SIGNAL(editingFinished()),this,SLOT(setupSliderUI()));

    time = new QTimer;
    time->setInterval(250);
    time->setSingleShot(true);
    connect(time,SIGNAL(timeout()),this,SLOT(timeoutSlot()));

    connect(ui->pSB_i1,SIGNAL(valueChanged(int)),this,SLOT(startIt()));
    connect(ui->pSB_i2,SIGNAL(valueChanged(int)),this,SLOT(startIt()));
    connect(ui->pSB_FMscale,SIGNAL(valueChanged(int)),this,SLOT(startIt()));


    setupSliderUI(_min, _max, _bin);
    boxUpdate();
    timeoutSlot();

    enableToolTips();

}

preDialog::~preDialog()
{
    delete ui;
}

void preDialog::load1()
{
    QImage img1;
    if(_dt==VOX)
    {
        QString fname1= _dirList->at(_cbpos).at(0).filePath();
        img1=loadEverything(_dt,fname1,_scaleMin,_scaleMax,-1,_vInfo,ui->pSB_i1->maximum()-ui->pSB_i1->value(),_rev);
    }
    else
    {
        QString fname1= _dirList->at(_cbpos).at(ui->pSB_i1->maximum()-ui->pSB_i1->value()).filePath();
        img1=loadEverything(_dt,fname1,_scaleMin,_scaleMax,-1);
    }
    QPixmap pix1;
    pix1.convertFromImage(dispImage(img1));
    pix1=pix1.scaledToWidth(ui->pWindow1->width());
    ui->pWindow1->setPixmap(pix1);

}

void preDialog::load2()
{
    QImage img2;
    if(_dt==VOX)
    {
        QString fname2= _dirList->at(_cbpos+1).at(0).filePath();
        img2=loadEverything(_dt,fname2,_scaleMin,_scaleMax,-1,_vInfo,ui->pSB_i2->value(),_rev);
    }
    else
    {
        QString fname2= _dirList->at(_cbpos+1).at(ui->pSB_i2->value()).filePath();
        img2=loadEverything(_dt,fname2,_scaleMin,_scaleMax,-1);
    }
    QPixmap pix;
    pix.convertFromImage(dispImage(img2));
    pix=pix.scaledToWidth(ui->pWindow2->width());
    ui->pWindow2->setPixmap(pix);

}

void preDialog::startIt()
{
    time->start();
}

void preDialog::timeoutSlot()
{
   load1();
   load2();
}

void preDialog::cursorMoved(Cursor* pCursor)
{
    float br=1000.0/(_scaleMax-_scaleMin);
    float pos_inGVal=pCursor->pos().x()/br+_scaleMin;
    QStringList pattern;pattern << "min" << "max" << "bin";
    switch (pattern.indexOf(pCursor->name())) {
    case 0:
        if (pos_inGVal>_max*0.999) pos_inGVal=_max*0.999;
        _min=pos_inGVal;
        _bin=max(_bin,_min);
        break;
    case 1:
        if (pos_inGVal<_min*1.001) pos_inGVal=_min*1.001;
        _max=pos_inGVal;
        _bin=min(_bin,_max);
        break;
    case 2:
        if (pos_inGVal<_min*1.001) pos_inGVal=_min*1.001;
        if (pos_inGVal>_max*0.999) pos_inGVal=_max*0.999;
        _bin=pos_inGVal;
        break;
    }
    if (pCursor!=pBinCur) pBinCur->setPos(QPointF((_bin-_scaleMin)*br,0));
    pCursor->setPos(QPointF((pos_inGVal-_scaleMin)*br,0));
    //pCursor->text()->setText(QString("%1").arg(pos_inGVal,0,'g',4));
    setupSliderUI(_min,_max,_bin);
    boxUpdate();
}

void preDialog::setupSliderUI(float _mi, float _ma, float _bi)
{

    if(_mi==-1 && _bi==-1 && _ma==-1)
    {
        _mi=ui->pSB_min->value();
        _bi=ui->pSB_bin->value();
        _ma=ui->pSB_max->value();
    }
    _min=_mi;
    _max=_ma;
    _bin=_bi;

    float br=(_scaleMax-_scaleMin)/1000.0;

    QPixmap pix=pPixItem->pixmap();
    pix.fill(Qt::black);
    QPainter pain(&pix);
    pain.setPen(QPen(Qt::white,1));
    pain.setBrush(Qt::red);
    pain.drawRect(QRectF((_min-_scaleMin)/br,5,(_max-_min)/br,8));
    pain.end();
    pPixItem->setPixmap(pix);
    time->start();



    pMinCur->setPos((_min-_scaleMin)/br,0);
    pBinCur->setPos((_bin-_scaleMin)/br,0);
    pMaxCur->setPos((_max-_scaleMin)/br,0);
}

void preDialog::getCurrentValues(double& _mi, double& _ma, double& _bi, int& refPos, int& matchPos, float& FMscale)
{
    _mi=_min;
    _ma=_max;
    _bi=_bin;

    refPos=ui->pSB_i1->value();
    matchPos=ui->pSB_i2->value();
    FMscale=(float)ui->pSB_FMscale->value()/100.0f;

}

void preDialog::boxUpdate()
{
    if(_dt==TIFF32)
    {
        ui->pSB_min->setDecimals(6);
        ui->pSB_bin->setDecimals(6);
        ui->pSB_max->setDecimals(6);

        ui->pSB_min->setSingleStep(0.000001);
        ui->pSB_bin->setSingleStep(0.000001);
        ui->pSB_max->setSingleStep(0.000001);
    }
    ui->pSB_min->setRange(_min,_max);
    ui->pSB_bin->setRange(_min,_max);
    ui->pSB_max->setRange(_min,_max);

    ui->pSB_min->setValue(_min);
    ui->pSB_bin->setValue(_bin);
    ui->pSB_max->setValue(_max);

    if(ui->pSB_min->value()>= ui->pSB_max->value())
        ui->pSB_min->setValue(ui->pSB_max->value());
    if(ui->pSB_max->value()<= ui->pSB_min->value())
        ui->pSB_max->setValue(ui->pSB_min->value());

    if(ui->pSB_bin->value()>= ui->pSB_max->value())
        ui->pSB_bin->setValue(ui->pSB_max->value());
    if(ui->pSB_bin->value()<= ui->pSB_min->value())
        ui->pSB_bin->setValue(ui->pSB_min->value());

}

void preDialog::enableToolTips()
{
    QString scale("Select size of the region for matching in percent from the middle of the images.");
    ui->pL_FMscale->setToolTip(scale);
    ui->pSlider_FMscale->setToolTip(scale);
    ui->pSB_FMscale->setToolTip(scale);

    QString lslice("Select position of reference slice in the subStack.");
    ui->pSlider_i1->setToolTip(lslice);
    ui->pSB_i1->setToolTip(lslice);

    QString rslice("Select position of matching slice in the subStack.");
    ui->pSlider_i2->setToolTip(rslice);
    ui->pSB_i2->setToolTip(rslice);

    QString mins("Select global minimum gray value. Only applied to data set for 32-bit tif-images.");
    ui->pSB_min->setToolTip(mins);
    ui->pL_min->setToolTip(mins);

    QString maxs("Select global maximum gray value. Only applied to data set for 32-bit tif-images.");
    ui->pSB_max->setToolTip(maxs);
    ui->pL_max->setToolTip(maxs);

    QString bins("Select gray value for binarization threshhold. Select threshold to binarize structures of interest.");
    ui->pSB_bin->setToolTip(bins);
    ui->pL_bin->setToolTip(bins);

    QString bigSlider("Select min, max and the threshhold for binarization. min and max gray values are only applied to the data set for 32-bit tif-images.");
    ui->pSliderView->setToolTip(bigSlider);
}
