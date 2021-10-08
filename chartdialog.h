#ifndef CHARTDIALOG_H
#define CHARTDIALOG_H

#include <QDialog>
#include <QChartView>

using namespace QtCharts;

namespace Ui {
class chartDialog;
}

class chartDialog : public QDialog
{
    Q_OBJECT

public:
    explicit chartDialog(QList<QVector<QPointF>*>*,QStringList, QWidget *parent = nullptr);
    ~chartDialog();

    void plot(QVector<QPointF>);

private slots:
    void on_pSubStackCB_currentIndexChanged(int index);

private:
    Ui::chartDialog *ui;
    QList<QVector<QPointF>*> *pList=nullptr;
};

#endif // CHARTDIALOG_H
