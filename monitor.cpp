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
    QDesktopWidget *desktop = QApplication::desktop();
    move((desktop->width() - this->width())/2, (desktop->height() - this->height())/2);
    setWindowTitle(QStringLiteral("SYSU视频监控"));

    save_picture_flag = 0;
    vd = new video_device(tr("/dev/video0"));

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(update()));
    qDebug() << " MainWindow initialize finish";
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
        timer->start(30);
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
