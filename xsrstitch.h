#ifndef XSRSTITCH_H
#define XSRSTITCH_H

#include "tools.h"
#include <QDialog>
#include <QFileInfoList>


QT_BEGIN_NAMESPACE
namespace Ui { class xSrstitch; }
QT_END_NAMESPACE





class xSrstitch : public QDialog
{
    Q_OBJECT

public:
    xSrstitch(QWidget *parent = nullptr);
    ~xSrstitch();

    bool loadDir();
    void CopyTransfer();
    void clearAll();

public slots:
    void updateSettingsUi();

    void lockUnlock();
    void updateRange(int val);
    void enableToolTips();


private slots:
    void on_pButton_InputDir_clicked();

    void on_pButton_Close_clicked();

    void on_pButton_slicePreview_clicked();

    void on_pButton_Go_clicked();

    void on_pButton_Plot_clicked();

    void on_pButton_OutputDir_clicked();

    void on_pButtonShowImgs_clicked();

    void on_pButton_Stitch_clicked();

private:
    QList<QVector<QPointF>*> maxList;
    double _maxGlobal=0.0;
    double _minGlobal=0.0;
    double _binGlobal=0.0;
    float FMscale;
    QList<QFileInfoList> _dirList;
    QList<transformData> _tData;

    QList<QList<QImage>*> showIMGs;
    QPalette pON;
    QPalette pOFF;
    QStringList subStackNames;

    datatype dt=INVALID;
    VOXInfo* vInfo=nullptr;
    bool reverse=false;

    Ui::xSrstitch *ui;
};
#endif // XSRSTITCH_H
