#include <QDebug>

#include "ffmpegdecode.h"

#define INBUF_SIZE 4096

FFmpegDecode::FFmpegDecode(QObject *parent) : QObject(parent)
{

    qDebug()<<"Video version"<<avutil_version();
    qDebug()<<"Video version"<<avutil_license();
    qDebug()<<"Video version"<<av_version_info();

    const AVCodec *codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    qDebug()<<"Codec name"<<codec->long_name;
}

const char *FFmpegDecode::getffmpegInfo()
{
    return av_version_info();
}

FFmpegDecode::FFmpegStatus FFmpegDecode::initDecoder(QString fileName)
{
    pkt = av_packet_alloc();
    if (!pkt) {
        return FFMPEG_ALLOC_AV_ERROR;
    }

    /* find the MPEG-1 video decoder */
    codec = avcodec_find_decoder(AV_CODEC_ID_MPEG1VIDEO);
    if (!codec) {
        return FFMPEG_FIND_DECODER_ERROR;
    }

    /*
     * Init parcer.
     * Parcer is used to parse the input video frame by frame.\
     * Explanation: sometimes we can't read the input (video) file,\
     * because the size of the file is too big. To trick this limitation,
     * we read a small piece of data and push it to the parser. After
     * completing reading the next frame, the parser is notified that the
     * new code frame is ready and we can go to the next decoding step.
     */
    parser = av_parser_init(codec->id);
    if (!parser) {
        return FFMPEG_PARSER_INIT_ERROR;
    }

    c = avcodec_alloc_context3(codec);
    if (!c) {
        return FFMPEG_ALLOC_CON3_ERROR;
    }

    /* For some codecs, such as msmpeg4 and mpeg4, width and height
       MUST be initialized there because this information is not
       available in the bitstream. */

    /* open it */
    if (avcodec_open2(c, codec, NULL) < 0) {
        return FFMPEG_OPEN2_ERROR;
    }

    file.setFileName(fileName);
    if (file.open(QIODevice::ReadOnly) == false) {
        return FFMPEG_OPEN_FILE_ERROR;
    }

    frame = av_frame_alloc();
    if (!frame) {
        return FFMPEG_ALLOC_FRAME_ERROR;
    }

    return FFMPEG_OK;
}

FFmpegDecode::FFmpegStatus FFmpegDecode::closeDecoder()
{
    file.close();

    return FFmpegDecode::FFMPEG_OK;
}

FFmpegDecode::FFmpegStatus FFmpegDecode::getFrame()
{
    return FFmpegDecode::FFMPEG_OK;
}
