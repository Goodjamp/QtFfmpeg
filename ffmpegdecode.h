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
        FFMPEG_FIND_STREAM_ERROR,
        FFMPEG_ALLOCATE_CONTEXT3_ERROR,
        FFMPEG_PARAMETRS_TO_CONTEXT_ERROR,
        FFMPEG_ALLOCATE_PKT_ERROR,
        FFMPEG_ALLOCATE_FRAME_ERROR,
    } FFmpegStatus;

    explicit FFmpegDecode(QObject *parent = nullptr);
    const char *getffmpegInfo();
    FFmpegStatus initDecoder(QString fileName);
    FFmpegStatus closeDecoder();
    FFmpegStatus getFrame();
    FFmpegStatus connectCamerra();

    FFmpegStatus readFrame(uint8_t *dstFrame);

private:
    const AVCodec *codec;
    AVCodecParserContext *parser;
    AVCodecContext *c= NULL;
    FILE *f;

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


    const char *cameraPath = "/dev/video0";
    const AVInputFormat *inputFormat;
    AVDictionary *options = NULL;
    AVFormatContext *pAVFormatContext = NULL;
    const AVCodec *pLocalCodec = NULL;
    const AVCodecParameters *pCodecParameters = NULL;
    const AVCodec *pCodec = NULL;
    AVPacket *pkt = NULL;
    AVFrame *frame = NULL;
    AVCodecContext *pCodecContext;
};

#endif // FFMPEGDECODE_H
