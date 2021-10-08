#include <stdafx.h>
#include <cstdlib>
#include "xsrstitch.h"

#include <QApplication>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setStyle(QStyleFactory::create("fusion"));
    xSrstitch w;
    QPalette pal=w.palette();
    pal.setColor(QPalette::Highlight,Qt::red);
    a.setPalette(pal);

    w.setLocale(QLocale::English);
    w.show();
    return a.exec();
}
