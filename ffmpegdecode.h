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
        FFMPEG_FIND_CODEC_DECODER_ERROR,
        FFMPEG_ALOCK_CODEC_DECODER_ERROR,
        FFMPEG_OPEN_CODEC_DECODER_ERROR,
        FFMPEG_OPEN_INPUT_PATH_STREAM_ERROR,
        FFMPEG_FIND_INPUT_STREAM_ERROR,
        FFMPEG_FIND_VIDE_STREAM_ERROR,


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
    FFmpegStatus closeDecoder();
    FFmpegStatus getFrame();
    FFmpegStatus connectCamerra();
    FFmpegStatus connectToFile();
    FFmpegStatus saveCameraStream(QString outFileName);

    FFmpegStatus readFrame(uint8_t *dstFrame);
    QSize getFrameSize();

private:

    const AVCodec *codecDecode;
    AVCodecParameters *codecDecodeParameters = NULL;
    AVCodecContext *codecDecodeContext;
    AVCodecParserContext *codecDecodeParserContext;

    const AVCodec *codecEncode;
    AVCodecParameters *codecEncodeParameters = NULL;
    AVCodecContext *codecEncodeContext;
    AVCodecParserContext *codecEncodeParserContext;

    const AVInputFormat *inputFormat;
    AVDictionary *dictionaryOptions = NULL;
    AVFormatContext *formatContext = NULL;

    AVPacket *pkt = NULL;
    AVFrame *frame = NULL;

    int videoStreamInd;

    const char *cameraPath = "/dev/video0";
    const char *filePath = "/home/oleksandr/Downloads/52_jumps.avi";
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
