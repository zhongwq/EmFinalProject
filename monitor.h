#ifndef MONITOR_H
#define MONITOR_H

#include <QWidget>
#include "video_device.h"
#include "v4l2grab.h"

namespace Ui {
class Monitor;
}

class Monitor : public QWidget
{
    Q_OBJECT

public:
    explicit Monitor(QWidget *parent = 0);
    unsigned char rgb_frame_buffer[IMAGEWIDTH*IMAGEHEIGHT*3];
    ~Monitor();

private slots:
    void update();
    void on_playButton_released();
    void on_saveButton_released();

private:
    Ui::Monitor *ui;
    QTimer *timer;
    video_device *vd;
    size_t len;
    unsigned char *yuv_buffer_pointer;
    QString save_pic_name;
    bool save_picture_flag;
};

#endif // MONITOR_H
