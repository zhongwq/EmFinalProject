#include "monitor.h"
#include "ui_monitor.h"

#include <QDesktopWidget>
#include<QDebug>

#define QStringLiteral(str) QString::fromUtf8(str, sizeof(str) - 1)

Monitor::Monitor(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Monitor)
{
    ui->setupUi(this);
    QDesktopWidget *desktop = QApplication::desktop();
    move((desktop->width() - this->width())/2, (desktop->height() - this->height())/2);
    setWindowTitle(QStringLiteral("SYSU视频监控"));
}

Monitor::~Monitor()
{
    delete ui;
}


void Monitor::on_startButton_clicked() {
    qDebug("StartButton clicked.");
}

void Monitor::on_stopButton_clicked() {
    qDebug("StopButton clicked.");
}

void Monitor::on_saveButton_clicked() {
    qDebug("SaveButton clicked.");
}
