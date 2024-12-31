#ifndef FFMPEGDECODE_H
#define FFMPEGDECODE_H

#include <QObject>
#include <QWidget>
#include <QString>
#include <QFile>
#include <QMap>

extern "C" {
    #include <stdint.h>
    #include <stdio.h>
    #include "libavcodec/avcodec.h"
    #include "libavformat/avformat.h"
    #include "libavdevice/avdevice.h"
    #include "libavutil/avutil.h"
    #include "libavutil/mathematics.h"
    #include "libswscale/swscale.h"
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
        FFMPEG_FRAME_GET_BUFF_ERROR,

        FFMPEG_RTP_ALLOC_CONTEXT_ERROR,
        FFMPEG_RTP_ALLOC_CONTEXT_NULL_ERROR,
        FFMPEG_RTP_CREATE_STREAM_ERROR,
        FFMPEG_RTP_OPEN_STREAM_ERROR,
        FFMPEG_RTP_WRITE_HEADR_ERROR,

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

    typedef struct {
        QSize resolution;
        int frameRate;
        QString cameraPath;
        QString inUrl;
        QString outUrl;
    } Properties;

    explicit FFmpegDecode(QObject *parent = nullptr);
    const char *getFfmpegInfo();
    FFmpegStatus camerraPlay(Properties properties);
    FFmpegStatus filePlay();
    FFmpegStatus cameraRecord(Properties properties );
    FFmpegStatus cameraSreamNetwork(Properties properties );
    void stopVideo();


    FFmpegStatus readFrame(uint8_t *dstFrame);
    QSize getFrameSize();
    void encode(uint8_t *dstFrame);
    void streamRtp(uint8_t *dstFrame);

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
    //It is a stream from the any type of source: camera, file or any other
    AVFormatContext *rxStreamContext = NULL;

    // RTP stream context
    AVFormatContext *txRtpStreamContext = NULL;

    AVPacket *pkt = NULL;
    AVFrame *frame = NULL;

    AVFrame *frameEncode = NULL;
    AVPacket *pktEncode = NULL;

    int videoStreamInd;

    const char *cameraPath = "/dev/video0";
    const char *filePath = "/home/oleksandr/Programing/SW/FFmpeg/test.mp4";
    QFile cameraRecFile;
    FILE *f;

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

    FFmpegDecode::FFmpegStatus openEncoder(QSize frameResolution);
    FFmpegDecode::FFmpegStatus openDecoder();
    FFmpegDecode::FFmpegStatus openInputCameraStream(QSize frameResolution, int frameRate, QString cameraPath);
    FFmpegDecode::FFmpegStatus openOutputRtpStream(QString remoterUrl);
    void generateSdp();

};

#endif // FFMPEGDECODE_H
