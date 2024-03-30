#include "mainwindow.h"

#include <QApplication>
#include <QDebug>

extern "C" {
    #include "libavcodec/avcodec.h"
    #include "libavdevice/avdevice.h"
    #include "libavutil/avutil.h"
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;

    qDebug()<<"Video version"<<avutil_version();
    qDebug()<<"Video version"<<avutil_license();
    qDebug()<<"Video version"<<av_version_info();

    const AVCodec *codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    qDebug()<<"Codec name"<<codec->long_name;


    w.show();
    return a.exec();
}
