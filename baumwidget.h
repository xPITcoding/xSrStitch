#ifndef BAUMWIDGET_H
#define BAUMWIDGET_H

#include <QListWidget>

class BaumWidget : public QListWidget
{
    Q_OBJECT
public:
    BaumWidget(QWidget* parent=nullptr);

protected:
    void dragEnterEvent(QDragEnterEvent*) override;
    void dropEvent(QDropEvent*) override;

private:
    QListWidgetItem* draggedItem=nullptr;
            ;

};

#endif // BAUMWIDGET_H
