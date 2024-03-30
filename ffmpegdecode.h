#ifndef FFMPEGDECODE_H
#define FFMPEGDECODE_H

#include <QObject>
#include <QWidget>

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
    } FFmpegStatus;
    explicit FFmpegDecode(QObject *parent = nullptr);
    const char *getffmpegInfo();
    FFmpegStatus initDecoder(const char *filename);
    FFmpegStatus getFrame();

signals:

private:
    const AVCodec *codec;
    AVCodecParserContext *parser;
    AVCodecContext *c= NULL;
    FILE *f;
    AVFrame *frame;
    AVPacket *pkt;

};

#endif // FFMPEGDECODE_H
