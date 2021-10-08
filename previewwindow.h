#ifndef PREVIEWWINDOW_H
#define PREVIEWWINDOW_H

#include <QWidget>

namespace Ui {
class previewWindow;
}

class previewWindow : public QWidget
{
    Q_OBJECT

public:
    explicit previewWindow(QWidget *parent = nullptr);
    ~previewWindow();

private:
    Ui::previewWindow *ui;
};

#endif // PREVIEWWINDOW_H
