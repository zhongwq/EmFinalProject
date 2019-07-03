#include "monitor.h"
#include "ui_monitor.h"
#include "video_device.h"
#include "v4l2grab.h"

#include <QTimer>
#include <QPixmap>
#include <QImage>
#include <QPainter>
#include <QThread>
#include <QString>
#include <QFile>
#include <QDebug>
#include <QtGui>
#include <QTextEdit>
#include <QMessageBox>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/time.h>
#include <linux/videodev2.h>
#include <linux/version.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <QDesktopWidget>
#include <QDebug>

#define QStringLiteral(str) QString::fromUtf8(str, sizeof(str) - 1)

Monitor::Monitor(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Monitor) {
    ui->setupUi(this);

    server = new QTcpServer(this);
    if (!server->listen(QHostAddress::Any, 8080)) {
        QMessageBox::critical(this, "error", "Listen port 8080 failed");
        exit(0);
    }

    connect(server, SIGNAL(newConnection()), this, SLOT(new_client()));

    QDesktopWidget *desktop = QApplication::desktop();
    move((desktop->width() - this->width())/2, (desktop->height() - this->height())/2);
    setWindowTitle(QStringLiteral("SYSU视频监控"));

    save_picture_flag = 0;
    vd = new video_device(tr("/dev/video0"));

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(update()));
    qDebug() << " MainWindow initialize finish";
}

void Monitor::new_client() {
    QTcpSocket *client = server->nextPendingConnection();
    qDebug() << "New Client";
    connect(client, SIGNAL(disconnected()), this, SLOT(distconnect_client()));
    connect(client, SIGNAL(readyRead()), this, SLOT(read_data()));

    client->setUserData(0, (QObjectUserData*)new Data);
    clients.append(client);
}

void Monitor::distconnect_client() {
    QTcpSocket *client = qobject_cast<QTcpSocket *>(sender());
    Data *d = (Data*)client->userData(0);

    clients.removeOne(client);
}

void Monitor::read_data() {
    QTcpSocket *client = qobject_cast<QTcpSocket *>(sender());
    QString str = client->readAll();
    Data *d = (Data*)client->userData(0);
    QString s("newImage:%1");
    qDebug() << "Command: " << str;
    if (str == "new_request") {
        if ((d->len) && (d->stats==0)) { //图像大小不为0，表示已更新图像数据了
            d->stats = 1;
            client->write(s.arg(d->len).toUtf8());
            d->len_sent = 0;
        } else //图像数据还没有更新
            d->stats = 2; //在定时器的槽函数里发出"newImage..."
    } else if (str == "ack") {
        int len_send = P_LEN;
        if (d->len_sent >= d->len) //如果图像已传输完毕
            return;

        if ((d->len_sent + P_LEN) > d->len)
            len_send = d->len - d->len_sent;

        d->len_sent += client->write(d->data+d->len_sent, len_send);
        if (d->len_sent >= d->len) {
            d->stats = 0; //传输完毕后，把状态改为可更新
            d->len = 0;
        }
    }
}

Monitor::~Monitor() {
    delete ui;
}

void Monitor::update() {
    QPixmap pix;
    QByteArray aa ;
    BITMAPFILEHEADER   bf;
    BITMAPINFOHEADER   bi;

    //Set BITMAPINFOHEADER
    bi.biSize = 40;
    bi.biWidth = IMAGEWIDTH;
    bi.biHeight = IMAGEHEIGHT;
    bi.biPlanes = 1;
    bi.biBitCount = 24;
    bi.biCompression = 0;
    bi.biSizeImage = IMAGEWIDTH*IMAGEHEIGHT*3;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;

    //Set BITMAPFILEHEADER
    bf.bfType = 0x4d42;
    bf.bfSize = 54 + bi.biSizeImage;
    bf.bfReserved = 0;
    bf.bfOffBits = 54;

    if(-1 == vd->get_frame(&yuv_buffer_pointer, &len)) {
        qDebug() << " get_frame fail";
    }

    vd->yuyv_2_rgb888(yuv_buffer_pointer,len, rgb_frame_buffer);

    aa.append((const char *)&bf, 14);
    aa.append((const char *)&bi, 40);
    aa.append((const char *)rgb_frame_buffer, bi.biSizeImage);

    pix.loadFromData(aa);

    QString s("newImage:%1");

    for (int i = 0; i < clients.size(); i++) {
        Data *d = (Data*)clients.at(i)->userData(0);
        qDebug() << d->stats << ", sendContent outoutside: " << s.arg(d->len).toUtf8();

        if (d->stats != 1) { // 1表示传输中
            qDebug() << "1: " << d->stats << ", sendContent outside: " << s.arg(d->len).toUtf8();
            qDebug() << "ImageSize: " << aa.size();
            memcpy(d->data, aa.data(), aa.size());
            d->len = aa.size();
            qDebug() << "2: " << d->stats << ", sendContent outside: " << s.arg(d->len).toUtf8();

            if (d->stats == 2) {
                d->stats = 1; //改为传输中的状态
                d->len_sent = 0;
                for (int i = 0; i < clients.size(); i++) {
                    Data *d = (Data*)clients.at(i)->userData(0);
                    qDebug() << "status" << d->stats;

                    if (d->stats != 1) { // 1表示传输中
                        memcpy(d->data, aa.data(), aa.size());
                        d->len = aa.size();

                        if (d->stats == 2) {
                            d->stats = 1; //改为传输中的状态
                            d->len_sent = 0;
                            qDebug() << "sendContent inside: " << s.arg(d->len).toUtf8();
                            clients.at(i)->write(s.arg(d->len).toUtf8());
                        }
                    }
                }
                clients.at(i)->write(s.arg(d->len).toUtf8());
            }
        }
    }

    ui->MonitorView->setPixmap(pix);

    if(save_picture_flag == 1)
    {
        save_picture_flag = 0;
        QFile rgbfile(save_pic_name);

        if(!rgbfile.open(QIODevice::ReadWrite | QIODevice::Text))
        {
            QMessageBox msgbox(QMessageBox::Warning, "WARNING", "please enter file name");
            msgbox.exec();
            return;
        }

        rgbfile.write((const char *)&bf, 14);
        rgbfile.write((const char *)&bi, 40);

        rgbfile.write((const char *)rgb_frame_buffer, bi.biSizeImage);
        qDebug() << " save picture finish";
    }

    if(-1 == vd->unget_frame())
    {
        qDebug() << " unget_frame fail";
    }
}



void Monitor::on_playButton_released() {
    qDebug("PlayButton clicked.");

    static unsigned char count=0;
    QString _start = "Start";
    QString _stop = "Stop";

    if(count == 0)
    {
        ui->playButton->setText(_stop);
        timer->start(100);
        count = 1;
        ui->saveButton->setEnabled(1);
    }
    else
    {
        ui->playButton->setText(_start);
        timer->stop();
        count = 0;
        ui->saveButton->setEnabled(0);
    }
}

void Monitor::on_saveButton_released() {
    qDebug("SaveButton clicked.");
    save_pic_name =  ui->filePath->toPlainText();
    save_picture_flag = 1;
}
