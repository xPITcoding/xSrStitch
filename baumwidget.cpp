#include "baumwidget.h"
#include <QDropEvent>
BaumWidget::BaumWidget(QWidget* parent):QListWidget(parent)
{

}

void BaumWidget::dragEnterEvent(QDragEnterEvent* e)
{
    draggedItem=currentItem();
    QListWidget::dragEnterEvent(e);
}

void BaumWidget::dropEvent(QDropEvent *d)
{
    QPoint p(d->pos());
    QListWidgetItem* pItem=itemAt(p);
    int r;
    int r2=row(draggedItem);
    if(pItem)
        r=row(pItem);
    else
        r=count()-1;

    if(r>=r2)
        r++;

    if(pItem!=draggedItem)
    {
        insertItem(r,new QListWidgetItem(*draggedItem));
        takeItem(row(draggedItem));
    }
}
