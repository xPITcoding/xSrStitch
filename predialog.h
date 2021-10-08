#ifndef PREDIALOG_H
#define PREDIALOG_H

#include "tools.h"

#include <QDialog>
#include <QGraphicsPixmapItem>
#include <QGraphicsLineItem>
#include <QTimer>

namespace Ui {
class preDialog;
}

class Cursor:public QObject,public QGraphicsLineItem
{
Q_OBJECT
public:
    Cursor(const QString& name,float pos, QColor col,QGraphicsItem *pParent);
    QString name(){return _name;}
    QGraphicsSimpleTextItem* text(){return pText;}
protected:
    void mouseMoveEvent(QGraphicsSceneMouseEvent*) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent*) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent*) override;
signals:
    void modified(Cursor*);
protected:
    QString _name;
    QGraphicsSimpleTextItem* pText;
};

class preDialog : public QDialog
{
    Q_OBJECT

public:
    explicit preDialog(QList<QFileInfoList>* dirList, float _min, float _max, float _bin, datatype dt,int pos1, int max1, int pos2, int max2, int cbpos,VOXInfo* vInf, bool rev, QWidget *parent = nullptr);
    ~preDialog();

    void getCurrentValues(double& _mi, double& _ma, double& _bi, int& refPos, int& matchPos, float& FMscale);
    void enableToolTips();

protected slots:
    void cursorMoved(Cursor*);
    void timeoutSlot();
    void boxUpdate();
    void setupSliderUI(float _min=-1, float _max=-1, float _bin=-1);
    void load1();
    void load2();
    void startIt();

protected:
    QImage dispImage(QImage img);

private:
    Ui::preDialog *ui;

    float _min, _max, _bin;
    float _scaleMin,_scaleMax;

    QGraphicsPixmapItem *pPixItem=nullptr;
    Cursor *pMinCur,*pMaxCur,*pBinCur;
    QTimer* time= nullptr;

    QList<QFileInfoList>* _dirList;

    int _cbpos;
    int _pos1;
    int _max1;
    int _pos2;
    int _max2;
    VOXInfo* _vInfo=nullptr;
    bool _rev=false;


    datatype _dt;
};

#endif // PREDIALOG_H
