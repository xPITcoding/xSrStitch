#include "chartdialog.h"
#include "ui_chartdialog.h"
#include "QLineSeries"
#include <QValueAxis>


chartDialog::chartDialog(QList<QVector<QPointF>*>* in, QStringList names, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::chartDialog)
{
    ui->setupUi(this);
    setFixedSize(745,545);
    for(int i=0; i<names.count()-1; i++)
    {
        ui->pSubStackCB->addItem(names[i]);
    }

    pList=in;
    if (pList && pList->count()>0 && pList->at(0))
        plot(*(pList->at(0)));
}

chartDialog::~chartDialog()
{
    delete ui;
}


void chartDialog::plot(QVector<QPointF> maxList)
{
    QLineSeries *pLineS= new QLineSeries;
    pLineS->setName(ui->pSubStackCB->currentText());

    for(QVector<QPointF>::iterator it=maxList.begin(); it!=maxList.end(); it++)
    {
       (*pLineS) << (*it);
    }

    QChart *pChart= new QChart;
    ui->graphicsView->setChart(pChart);
    pChart->setTheme(QChart::ChartThemeDark);
    pChart->addSeries(pLineS);
    pChart->createDefaultAxes();
    QList<QAbstractAxis*> pAxList= pChart->axes(Qt::Horizontal);
    if(pAxList.count()>0)
    {
        QValueAxis* pXA = dynamic_cast<QValueAxis*>(pAxList[0]);
        if(pXA)
        {
            pXA->setTickType(QValueAxis::TicksFixed);
            pXA->setTickInterval(1);
            pXA->setLabelFormat("%d");
        }
    }
    pChart->update();

}

void chartDialog::on_pSubStackCB_currentIndexChanged(int index)
{
    if (pList && pList->count()>index && pList->at(index)) plot(*(pList->at(index)));
}
