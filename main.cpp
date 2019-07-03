#include "mainwindow.h"
#include "monitor.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    //MainWindow w;
    Monitor monitor;
    monitor.show();
    // w.show();

    return a.exec();
}
