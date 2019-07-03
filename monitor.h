#ifndef MONITOR_H
#define MONITOR_H

#include <QWidget>
#include <QTcpServer>
#include <QTcpSocket>
#include <QList>
#include "video_device.h"
#include "v4l2grab.h"

#define W  640
#define H  480

#define LEN  (W*H*3 + 54)

#define P_LEN  1380

namespace Ui {
class Monitor;
}

struct Data {
    char data[LEN]; //图像数据
    int  len; //图像大小
    int  len_sent; //已发出的数据长度
    int  stats; //工作状态, 0表示空闲(图像数据可以更新), 1表示图像数据传输中(还不可以更新图像数据), 2表示需要发出图像数据的第一部分
    Data() {
        len = 0;
        len_sent = 0;
        stats = 0;
    }
};

class Monitor : public QWidget
{
    Q_OBJECT

public:
    explicit Monitor(QWidget *parent = 0);
    unsigned char rgb_frame_buffer[IMAGEWIDTH*IMAGEHEIGHT*3];
    ~Monitor();

private slots:
    void update();
    void new_client();
    void read_data();
    void distconnect_client();
    void on_playButton_released();
    void on_saveButton_released();

private:
    Ui::Monitor *ui;
    QTimer *timer;
    video_device *vd;
    size_t len;
    QTcpServer *server;
    QList<QTcpSocket *> clients;
    unsigned char *yuv_buffer_pointer;
    QString save_pic_name;
    bool save_picture_flag;
};

#endif // MONITOR_H
