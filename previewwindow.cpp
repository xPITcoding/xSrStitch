#include "previewwindow.h"
#include "ui_previewwindow.h"

previewWindow::previewWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::previewWindow)
{
    ui->setupUi(this);
}

previewWindow::~previewWindow()
{
    delete ui;
}
