#ifndef FFMPEGDECODE_H
#define FFMPEGDECODE_H

#include <QObject>
#include <QWidget>
#include <QString>
#include <QFile>
#include <QMap>

extern "C" {
    #include <stdint.h>
    #include "libavcodec/avcodec.h"
    #include "libavdevice/avdevice.h"
    #include "libavutil/avutil.h"
}

class FFmpegDecode : public QObject
{
    Q_OBJECT
public:
typedef enum {
        FFMPEG_OK,
        FFMPEG_ALLOC_AV_ERROR,
        FFMPEG_FIND_DECODER_ERROR,
        FFMPEG_PARSER_INIT_ERROR,
        FFMPEG_ALLOC_CON3_ERROR,
        FFMPEG_OPEN2_ERROR,
        FFMPEG_OPEN_FILE_ERROR,
        FFMPEG_ALLOC_FRAME_ERROR,
    } FFmpegStatus;

    explicit FFmpegDecode(QObject *parent = nullptr);
    const char *getffmpegInfo();
    FFmpegStatus initDecoder(QString fileName);
    FFmpegStatus closeDecoder();
    FFmpegStatus getFrame();

private:
    const AVCodec *codec;
    AVCodecParserContext *parser;
    AVCodecContext *c= NULL;
    FILE *f;
    AVFrame *frame;
    AVPacket *pkt;

    QFile file;

    const QMap<FFmpegDecode::FFmpegStatus, QString> statusToText{
        {FFMPEG_OK, "FFMPEG_OK"},
        {FFMPEG_ALLOC_AV_ERROR, "FFMPEG_ALLOC_AV_ERROR"},
        {FFMPEG_FIND_DECODER_ERROR, "FFMPEG_FIND_DECODER_ERROR"},
        {FFMPEG_PARSER_INIT_ERROR, "FFMPEG_PARSER_INIT_ERROR"},
        {FFMPEG_ALLOC_CON3_ERROR, "FFMPEG_ALLOC_CON3_ERROR"},
        {FFMPEG_OPEN2_ERROR, "FFMPEG_OPEN2_ERROR"},
        {FFMPEG_OPEN_FILE_ERROR, "FFMPEG_OPEN_FILE_ERROR"},
        {FFMPEG_ALLOC_FRAME_ERROR, "FFMPEG_ALLOC_FRAME_ERROR"},
    };
};

#endif // FFMPEGDECODE_H
